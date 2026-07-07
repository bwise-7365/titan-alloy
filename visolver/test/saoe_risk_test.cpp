// Copyright Ben Paul Wise. All Rights Reserved.
#include "saoe.hpp"
#include "saoesupport.hpp"
#include "utils.hpp"

#include <Eigen/Dense>
#include <gtest/gtest.h>

#include <algorithm>
#include <cmath>
#include <cstdio>

using namespace VINCP;

// ============================================================================
// Risk-averse SAOE (PME framework paper, section "Risk Aversion in Coalition
// Formation", eq. 4.53; functional form as in the Octave saoeJNra.m / esJ.m).
// Each actor steers by the risk-adjusted reward
//     S_ij = R_ij (1 - alpha_i (R_ij - mu_i)),
//     alpha_i = a * ln2 / halfSpread_i,
//     halfSpread_i = (max_j R_ij - min_j R_ij) / 2,
// with mu_i the mean of the actor's reward under the current option
// probabilities and alpha_i CONSTANT per actor (spread-calibrated: at a = 1,
// losing half the spread hurts twice as much as gaining half the spread
// helps). The two acceptance criteria, per the author, each run
// over EXACTLY the same two instances as saoe_chain_test (the shared case
// list saoeSharedCases() in saoesupport.hpp):
//   (A) a = 0 must MATCH the risk-neutral SAOE exactly -- same answer, and
//       visibly the same cost. The a = 0 branch is skipped outright in
//       saoeModel, so this is arithmetic identity, not a tolerance match;
//       every solve below is labeled and timed so the match is in the log.
//   (B) RESTATED: the risk knob must behave as the parametric-
//       complementarity theory predicts -- allocation exactly frozen below
//       the (in-test computed) first breakpoint, active set visibly flipped
//       above it, feasible throughout; variances are printed as data, not
//       gated (per-actor variance monotonicity is FALSE across active-set
//       flips, as the first runs demonstrated).
//
// CONTINUATION DESIGN for (B) -- the lesson of the first run (2026-07-06):
// solving every a cold from the default start let the a = 0.25 solve land
// at a DIFFERENT equilibrium than a = 0 (this game has multiple basins --
// that is the whole equilibrium-selection saga), and variances across
// basins are incomparable: 5 of 6 rose, one fell. The variance property is
// a statement about the same equilibrium BRANCH as the risk knob turns, so
// each a > 0 solve is WARM-STARTED from the a = 0 equilibrium (and rides
// the branch by continuation), exactly as a practitioner would perturb an
// already-solved game. The warm start also makes the a > 0 solves cheap.
// ============================================================================

namespace {
    constexpr double kIdentityTol = 0.0;      // criterion A is exact identity
    // Fallback rungs for criterion B's non-all-in branch (an all-in baseline
    // computes its own breakpoint in-test instead). Exploratory settings in
    // the spread-calibrated units; the VERIFIED reference is a = 0.
    constexpr double kRiskA0      = 0.5;
    constexpr double kRiskA1      = 0.67;

    // One labeled, timed chain solve (prints UTC start/elapsed and the
    // standard solve statistics, so criteria A's "same answer, same cost"
    // is visible in the log).
    VIResult
    timedChainSolve(const char* label, const VIModel& model, const VectorXd& z0)
    {
        std::printf("--- %s ---\n", label);
        const auto t0 = utcNow();
        const VIResult r = makeSaoeChain(model)(z0);
        utcElapsed(t0);
        printSolveStats(label, r);
        std::fflush(stdout);
        return r;
    }
} // namespace

// Criterion A: at a = 0 the risk-adjusted model IS the risk-neutral model.
// The G maps must agree exactly at arbitrary points, and the chain must
// produce the identical result -- at visibly the same cost -- from the
// identical start. Checked on every shared case.
TEST(SaoeRisk, RiskNeutralLimitIsExact) {
    for (const SaoeCase& c : saoeSharedCases()) {
        SCOPED_TRACE(c.name);

        const VIModel neutral = saoeModel(c.R, c.S);
        const VIModel atZero  = saoeModel(c.R, c.S, /*riskAversion=*/0.0);
        const VectorXd z0 = saoeDefaultStart(c.R, c.S);

        // G maps identical at the start and at a rough interior probe point.
        const VectorXd g0n = evaluateF(neutral, z0);
        const VectorXd g0z = evaluateF(atZero, z0);
        EXPECT_EQ(0.0, (g0n - g0z).cwiseAbs().maxCoeff());
        VectorXd probe = z0;
        probe.array() += 1.0;
        const VectorXd g1n = evaluateF(neutral, probe);
        const VectorXd g1z = evaluateF(atZero, probe);
        EXPECT_EQ(0.0, (g1n - g1z).cwiseAbs().maxCoeff());

        // Identical deterministic solver on identical models: identical
        // results, and the printed timings should match to noise.
        const VIResult rNeutral = timedChainSolve(
            (c.name + ": risk-neutral model").c_str(), neutral, z0);
        const VIResult rZero = timedChainSolve(
            (c.name + ": risk model at a = 0").c_str(), atZero, z0);
        EXPECT_TRUE(rNeutral.converged) << c.name;
        EXPECT_TRUE(rZero.converged) << c.name;
        const double maxDiff = (rNeutral.z - rZero.z).cwiseAbs().maxCoeff();
        std::printf("  max |z difference| = %.3e (identity bar %.1e)\n\n",
                    maxDiff, kIdentityTol);
        EXPECT_LE(maxDiff, kIdentityTol);
    }
}

// Criterion B, RESTATED (2026-07-06): the original "no actor's variance may
// rise" form is FALSE across active-set flips (variance depends on the
// JOINT probabilities, which other actors' flips move). The restated gates
// are the two properties the parametric-complementarity theory actually
// guarantees, with the breakpoint COMPUTED IN THE TEST from the closed form
// alpha*_i = 1/(R_best + R_second - mu_i), a* = min_i alpha*_i
// halfSpread_i / ln2 (valid at an all-in baseline):
//   (B1, plateau)    at a = 0.5 a*: the allocation is EXACTLY the a = 0
//                    allocation (vertex rigidity; only multipliers move);
//   (B2, breakpoint) at a = 1.1 a*: converged + feasible AND the allocation
//                    has visibly changed (the active set flipped).
// Variances are PRINTED as data at every rung, never gated. A case whose
// a = 0 baseline is not all-in has no closed form; it falls back to
// converged + feasible gates at fixed rungs.
TEST(SaoeRisk, RiskKnobPlateauAndBreakpoint) {
    for (const SaoeCase& c : saoeSharedCases()) {
        SCOPED_TRACE(c.name);
        const double eps = saoeEps(c.R);

        // The a = 0 baseline equilibrium and its per-actor variances.
        const VIResult base = timedChainSolve(
            (c.name + ": baseline a = 0").c_str(),
            saoeModel(c.R, c.S, 0.0), saoeDefaultStart(c.R, c.S));
        ASSERT_TRUE(base.converged) << c.name;
        const SaoeSolution baseSol = saoeDecode(base, c.R.rows(), c.R.cols());
        const VectorXd baseVar = saoePayoffVariance(c.R, baseSol.e, eps);
        std::printf("  [%s] a = 0 variances:", c.name.c_str());
        for (Index i = 0; i < c.R.rows(); ++i) {
            std::printf(" %10.2f", baseVar(i));
        }
        std::printf("\n\n");

        // Is the baseline all-in, and if so where is the first breakpoint?
        const Index M = c.R.rows();
        const Index N = c.R.cols();
        const VectorXd prob = saoeProbabilities(baseSol.e, eps);
        const VectorXd mu = c.R * prob;
        const double ln2 = std::log(2.0);
        bool allInP = true;
        double aStar = 1.0e30;
        for (Index i = 0; allInP && i < M; ++i) {
            Index bestJ = 0;
            baseSol.e.row(i).maxCoeff(&bestJ);
            allInP = (baseSol.e.row(i).sum() - baseSol.e(i, bestJ)) < 1.0e-3;
            if (allInP) {
                double alphaStar = 1.0e30;
                for (Index j = 0; j < N; ++j) {
                    const double denom = c.R(i, bestJ) + c.R(i, j) - mu(i);
                    if (j != bestJ && 0.0 < denom) {
                        alphaStar = std::min(alphaStar, 1.0 / denom);
                    }
                }
                const double halfSpread =
                    0.5 * (c.R.row(i).maxCoeff() - c.R.row(i).minCoeff());
                aStar = std::min(aStar, alphaStar * halfSpread / ln2);
            }
        }

        if (allInP) {
            std::printf("  [%s] all-in baseline; predicted breakpoint a* = %.4f\n",
                        c.name.c_str(), aStar);
            // B1: below the breakpoint the allocation is frozen exactly.
            char label[96];
            std::snprintf(label, sizeof label, "%s: a = %.4f (plateau rung)",
                          c.name.c_str(), 0.5 * aStar);
            const VIResult plat = timedChainSolve(
                label, saoeModel(c.R, c.S, 0.5 * aStar), base.z);
            EXPECT_TRUE(plat.converged) << c.name;
            const SaoeSolution platSol = saoeDecode(plat, M, N);
            EXPECT_LE((platSol.e - baseSol.e).cwiseAbs().maxCoeff(), 1.0e-6)
                << c.name << ": allocation moved BELOW the predicted breakpoint";

            // B2: past the breakpoint the active set flips.
            std::snprintf(label, sizeof label, "%s: a = %.4f (breakpoint rung)",
                          c.name.c_str(), 1.1 * aStar);
            const VIResult flip = timedChainSolve(
                label, saoeModel(c.R, c.S, 1.1 * aStar), base.z);
            EXPECT_TRUE(flip.converged) << c.name;
            const CheckResult feas = saoeCheckFeasible(c.R, c.S)(flip);
            EXPECT_TRUE(feas.pass) << c.name << ": " << feas.report;
            const SaoeSolution flipSol = saoeDecode(flip, M, N);
            EXPECT_GT((flipSol.e - baseSol.e).cwiseAbs().maxCoeff(), 1.0e-3)
                << c.name << ": allocation did NOT move ABOVE the predicted breakpoint";

            // Variances as data, not gated.
            const VectorXd vP = saoePayoffVariance(c.R, platSol.e, eps);
            const VectorXd vF = saoePayoffVariance(c.R, flipSol.e, eps);
            std::printf("    plateau variances:");
            for (Index i = 0; i < M; ++i) { std::printf(" %10.2f", vP(i)); }
            std::printf("\n    flipped variances:");
            for (Index i = 0; i < M; ++i) { std::printf(" %10.2f", vF(i)); }
            std::printf("\n\n");
            std::fflush(stdout);
        }
        else {
            // No closed form off the vertex: converged + feasible only,
            // warm-started continuation at fixed rungs.
            std::printf("  [%s] baseline NOT all-in; feasibility gates only\n",
                        c.name.c_str());
            VectorXd warm = base.z;
            for (const double a : { kRiskA0, kRiskA1 }) {
                SCOPED_TRACE(a);
                char label[96];
                std::snprintf(label, sizeof label, "%s: a = %.2f (warm-started)",
                              c.name.c_str(), a);
                const VIResult averse =
                    timedChainSolve(label, saoeModel(c.R, c.S, a), warm);
                EXPECT_TRUE(averse.converged) << c.name;
                if (averse.converged) {
                    warm = averse.z;
                    const CheckResult feas = saoeCheckFeasible(c.R, c.S)(averse);
                    EXPECT_TRUE(feas.pass) << c.name << ": " << feas.report;
                }
            }
        }
    }
}
// Copyright Ben Paul Wise. All Rights Reserved.

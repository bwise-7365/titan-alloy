// Copyright Ben Paul Wise. All Rights Reserved.
//
// "GAMS" is a registered trademark of GAMS Development Corporation. This
// code is not endorsed or certified by GAMS Development Corporation. The
// subset of the GMS parsed by this code is incompatible with most of the
// GAMS modeling language. The software is provided without warranty of any
// kind, express or implied, including without limitation for any particular
// purpose. The provider makes no guarantees about its performance, accuracy,
// or suitability for any specific application.
#include "mcpengines.hpp"
#include "vincp.hpp"

#include <Eigen/Dense>
#include <gtest/gtest.h>

#include <cmath>
#include <cstdio>

using namespace VINCP;

// ============================================================================
// Faithful translation of alloceff01cm.gms (sheridan/alloceff): Nash
// equilibrium for exertion of influence, 6 actors x 10 options, as a mixed
// nonlinear complementarity problem over K = R^n x R_+^m.
//
// GMS -> VINCP mapping (Model neInf / nfDef.nfv, sigmaDef.sigma,
// gammaDef.gamma, EInf.beta, MVInf.eff /):
//   free block x (n = 21) = [ nfv(10) | sigma(10) | gamma ], H rows are their
//     defining =e= equations. The .gms declares them Positive, but each is
//     STRICTLY positive at any solution (nfv >= epsilon > 0, sigma and gamma
//     positive sums of positives), so the GAMS lower bound never binds and the
//     free-block treatment is exact.
//   nonneg block y (m = 66) = [ beta(6) | eff(act, opt), actor-major ], G rows
//     are the =g= equations paired with them:
//       beta(a):     0 <= weight(a) - sum_p eff(a, p)               (EInf)
//       eff(a, p):   0 <= beta(a) - slope(p) * swing(a, p) / gamma^2 (MVInf)
//     with slope(p) = alpha*effR + 2*(1-alpha)*nfv(p) and
//     swing(a, p) = sum_pk sigma(pk) * (reward(a, p) - reward(a, pk)).
//   The GAMS box eff.up(a, p) = weight(a) is DROPPED: it is implied by EInf
//     feasibility (sum_p eff <= weight with eff >= 0), and at the degenerate
//     corner where one eff exhausts the budget the R_+ formulation reaches the
//     same primal point with beta absorbing the box multiplier.
//
// Same data as saoe_test's standard instance, but where saoe.hpp solves a
// reduced effort-space VI, this is the LITERAL GAMS MCP with the intermediates
// (nfv, sigma, gamma) and shadow prices (beta) as explicit variables.
//
// PASS CRITERION (upgraded 2026-07-06 after the first green run): the problem
// is nonmonotone with multiple Nash equilibria, so the test passes iff AT
// LEAST ONE engine row converges AND lands on the REFERENCE equilibrium --
// the one PATH reaches (confirmed by the author), which is also saoe_test's
// reference E (NPLEC / the earlier saoeJNrn solver): each actor puts its
// whole weight on one option (A0->P4, A1->P3, A2->P9, A3->P2, A4->P5,
// A5->P4). The gate compares the decoded option probabilities and actor
// expected values against that equilibrium (kRefProb / kRefExpVal below); a
// row that converges into a different basin fails that row (reported, not
// fatal to the case). First green run (2026-07-06): ssn (14 iters, 182 ms)
// and jn+ipm (2 outer / 16 inner, 83 ms) both reached it; the projection
// rows threw their divergence guards (expected off-monotone).
//
// CAVEAT: gamma appears squared in MVInf denominators. The start (~2.8e4) is
// far from that pole, but an engine that wanders to gamma ~ 0 will see
// evaluateF throw on the non-finite value -- inside a line search ssn treats
// that as merit +infinity and backtracks; elsewhere the throw is reported by
// runCase as that row's failure.
// ============================================================================

namespace {
    // Sets (alloceff01cm.gms lines 26-27).
    constexpr int kNumAct = 6;    // actors A0..A5
    constexpr int kNumOpt = 10;   // options P0..P9

    // Parameters (lines 32-66).
    constexpr double kAlpha = 1.0;   // linear fraction (1 = purely linear)

    const double kWeight[kNumAct] = { 68.0, 66.0, 125.0, 101.0, 127.0, 96.0 };

    const double kReward[kNumAct][kNumOpt] = {
        { 0.00,  181.42,  -50.43,  -26.32,  256.02,  -21.27, -132.68,  -65.12,  131.40,   14.54 },
        { 0.00,   31.24,  -46.53,  122.90,   39.47,  -12.94,   50.32,   70.03,    8.34, -109.78 },
        { 0.00,  -30.11,  -48.64,  -56.84,  -51.50,  -80.42, -130.30,  -54.20,   -8.75,   51.97 },
        { 0.00,   29.12,  160.04,  -27.68,   91.49,   80.93,  117.95,   27.88,   33.62,   72.34 },
        { 0.00, -199.26, -234.61,   67.80, -319.57,  270.83,  234.85,  -14.91, -236.86, -103.50 },
        { 0.00,   78.66,  -22.12,   14.25,  109.23,  -17.30,  -31.16,  -42.61,   31.46,  -46.13 },
    };

    // Derived scalars, computed exactly as the .gms computes them.
    double
    totalWeight()
    {
        double total = 0.0;
        for (int a = 0; a < kNumAct; ++a) {
            total += kWeight[a];
        }
        return total;
    }

    // epsilon = sqrt(sum_a weight^2 / numAct) / 1000 -- lowest effort.
    double
    lowestEffort()
    {
        double sumSq = 0.0;
        for (int a = 0; a < kNumAct; ++a) {
            sumSq += kWeight[a] * kWeight[a];
        }
        return std::sqrt(sumSq / kNumAct) / 1000.0;
    }

    // effR = totalW / (1 + numOpt) -- reference level of net effort.
    double
    referenceEffort()
    {
        return totalWeight() / (1.0 + kNumOpt);
    }

    // Variable packing.
    //   x (free, n = 21):   [ nfv(10) | sigma(10) | gamma ]
    //   y (nonneg, m = 66): [ beta(6) | eff(act, opt), actor-major ]
    constexpr Index kNumFree = 2 * kNumOpt + 1;              // 21
    constexpr Index kNumComp = kNumAct + kNumAct * kNumOpt;  // 66

    constexpr Index nfvAt(int p)          { return p; }
    constexpr Index sigmaAt(int p)        { return kNumOpt + p; }
    constexpr Index gammaAt()             { return 2 * kNumOpt; }
    constexpr Index betaAt(int a)         { return a; }
    constexpr Index effAt(int a, int p)   { return kNumAct + a * kNumOpt + p; }

    // H rows (the =e= definitions, paired with the free block).
    VectorXd
    alloceffH(const VectorXd& x, const VectorXd& y)
    {
        const double epsilon = lowestEffort();
        const double effR    = referenceEffort();
        VectorXd h(kNumFree);
        for (int p = 0; p < kNumOpt; ++p) {
            double sumEff = 0.0;
            for (int a = 0; a < kNumAct; ++a) {
                sumEff += y(effAt(a, p));
            }
            const double nfv = x(nfvAt(p));
            h(nfvAt(p))   = nfv - (epsilon + sumEff);                                    // nfDef
            h(sigmaAt(p)) = x(sigmaAt(p))
                            - (kAlpha * nfv * effR + (1.0 - kAlpha) * nfv * nfv);        // sigmaDef
        }
        double sumSigma = 0.0;
        for (int p = 0; p < kNumOpt; ++p) {
            sumSigma += x(sigmaAt(p));
        }
        h(gammaAt()) = x(gammaAt()) - sumSigma;                                          // gammaDef
        return h;
    }

    // G rows (the =g= equations, paired with the nonneg block).
    VectorXd
    alloceffG(const VectorXd& x, const VectorXd& y)
    {
        const double effR = referenceEffort();
        VectorXd g(kNumComp);

        // EInf(a) _|_ beta(a):  weight(a) - sum_p eff(a, p) >= 0.
        for (int a = 0; a < kNumAct; ++a) {
            double sumEff = 0.0;
            for (int p = 0; p < kNumOpt; ++p) {
                sumEff += y(effAt(a, p));
            }
            g(betaAt(a)) = kWeight[a] - sumEff;
        }

        // MVInf(a, p) _|_ eff(a, p):
        //   beta(a) - slope(p) * sum_pk sigma(pk)*(reward(a,p) - reward(a,pk)) / gamma^2 >= 0.
        const double gamma   = x(gammaAt());
        const double gammaSq = gamma * gamma;
        for (int a = 0; a < kNumAct; ++a) {
            for (int p = 0; p < kNumOpt; ++p) {
                double swing = 0.0;
                for (int pk = 0; pk < kNumOpt; ++pk) {
                    swing += x(sigmaAt(pk)) * (kReward[a][p] - kReward[a][pk]);
                }
                const double slope = kAlpha * effR + 2.0 * (1.0 - kAlpha) * x(nfvAt(p));
                g(effAt(a, p)) = y(betaAt(a)) - slope * swing / gammaSq;
            }
        }
        return g;
    }

    VIModel
    buildModel()
    {
        VIModel model;
        model.n = kNumFree;
        model.m = kNumComp;
        model.H = alloceffH;
        model.G = alloceffG;
        return model;
    }

    // The GAMS .L levels: eff at the analytic center of the budget, beta at
    // 0.5, and the intermediates at their defined values (lines 80-93).
    VectorXd
    initialPoint()
    {
        const double epsilon = lowestEffort();
        const double effR    = referenceEffort();

        VectorXd y0(kNumComp);
        for (int a = 0; a < kNumAct; ++a) {
            y0(betaAt(a)) = 0.5;
            for (int p = 0; p < kNumOpt; ++p) {
                y0(effAt(a, p)) = kWeight[a] / (1.0 + kNumOpt);
            }
        }

        VectorXd x0(kNumFree);
        double sumSigma = 0.0;
        for (int p = 0; p < kNumOpt; ++p) {
            double sumEff = 0.0;
            for (int a = 0; a < kNumAct; ++a) {
                sumEff += y0(effAt(a, p));
            }
            const double nfv = epsilon + sumEff;
            x0(nfvAt(p))   = nfv;
            x0(sigmaAt(p)) = kAlpha * nfv * effR + (1.0 - kAlpha) * nfv * nfv;
            sumSigma += x0(sigmaAt(p));
        }
        x0(gammaAt()) = sumSigma;

        VectorXd z0(kNumFree + kNumComp);
        z0 << x0, y0;
        return z0;
    }

    // Report-only check: decode and print the GAMS output parameters
    //   prob(p) = sigma(p) / gamma,  expVal(a) = sum_p prob(p) * reward(a, p)
    // (unrounded, where the .gms rounds prob to 3 decimals) so a converged run
    // can be compared to the GAMS listing by eye. Always passes.
    CheckFn
    printGamsOutputs()
    {
        return [](const VIResult& r) -> CheckResult {
            const VectorXd x     = r.z.head(kNumFree);
            const double   gamma = x(gammaAt());
            if (!std::isfinite(gamma) || !(0.0 < gamma)) {
                return CheckResult{ true, "summary skipped (gamma not positive)" };
            }
            std::printf("  prob   =");
            for (int p = 0; p < kNumOpt; ++p) {
                std::printf(" %7.3f", x(sigmaAt(p)) / gamma);
            }
            std::printf("\n  expVal =");
            for (int a = 0; a < kNumAct; ++a) {
                double val = 0.0;
                for (int p = 0; p < kNumOpt; ++p) {
                    val += (x(sigmaAt(p)) / gamma) * kReward[a][p];
                }
                std::printf(" %8.3f", val);
            }
            std::printf("\n");
            return CheckResult{ true, "GAMS output parameters printed above" };
        };
    }

    // Reference equilibrium outputs (the PATH / NPLEC / saoeJNrn solution,
    // confirmed by the author 2026-07-06): prob and expVal decoded from the
    // all-in allocation described in the header. Values are quoted to the
    // 3 decimals of the confirming run, so the tolerances below sit safely
    // above that rounding (and above solver error at kMagTol) while still
    // rejecting any other equilibrium, which shifts these by whole units.
    const double kRefProb[kNumOpt] = {
        0.000, 0.000, 0.173, 0.113, 0.281, 0.218, 0.000, 0.000, 0.000, 0.214,
    };
    const double kRefExpVal[kNumAct] = {
        58.736, -9.359, -35.733, 83.426, -86.003, 14.836,
    };
    constexpr double kProbTol   = 2.0e-3;
    constexpr double kExpValTol = 5.0e-2;

    // Gating check: the decoded outputs must match the reference equilibrium.
    CheckFn
    checkPathEquilibrium()
    {
        return [](const VIResult& r) -> CheckResult {
            const VectorXd x     = r.z.head(kNumFree);
            const double   gamma = x(gammaAt());
            if (!std::isfinite(gamma) || !(0.0 < gamma)) {
                return CheckResult{ false, "gamma not positive -- cannot decode prob" };
            }
            double maxProbErr = 0.0;
            double maxValErr  = 0.0;
            for (int p = 0; p < kNumOpt; ++p) {
                const double err = std::abs(x(sigmaAt(p)) / gamma - kRefProb[p]);
                if (maxProbErr < err) {
                    maxProbErr = err;
                }
            }
            for (int a = 0; a < kNumAct; ++a) {
                double val = 0.0;
                for (int p = 0; p < kNumOpt; ++p) {
                    val += (x(sigmaAt(p)) / gamma) * kReward[a][p];
                }
                const double err = std::abs(val - kRefExpVal[a]);
                if (maxValErr < err) {
                    maxValErr = err;
                }
            }
            char buf[160];
            std::snprintf(buf, sizeof buf,
                          "vs PATH equilibrium: max |prob err| = %.2e (tol %.1e), "
                          "max |expVal err| = %.2e (tol %.1e)",
                          maxProbErr, kProbTol, maxValErr, kExpValTol);
            const bool matchP = (maxProbErr <= kProbTol) && (maxValErr <= kExpValTol);
            return CheckResult{ matchP, string(buf) };
        };
    }

    // Engine controls (squared-norm tolerances).
    constexpr double kMagTol       = 1.0e-8;   // ssn + JN outer stop
    constexpr int    kSsnIterMax   = 500;
    constexpr int    kOuterIterMax = 100;
    constexpr double kInnerMagTol  = 1.0e-10;  // JN inner (affine-VI) stop
    constexpr int    kInnerIterMax = 20000;    // projection inner cap; dHan06 may
                                               //   honestly cap out (it refactors
                                               //   every inner iteration)
} // namespace

TEST(GmsAlloceff, AtLeastOneEngineReachesPathEquilibrium) {
    const VIModel  model = buildModel();
    const VectorXd z0    = initialPoint();

    McpEngineParams params;
    params.magTol       = kMagTol;
    params.ssnIterMax   = kSsnIterMax;
    params.outerIterMax = kOuterIterMax;
    params.innerMagTol  = kInnerMagTol;
    params.innerIterMax = kInnerIterMax;

    const vector<McpEngine> engines = {
        McpEngine::Ssn, McpEngine::JnHe, McpEngine::JnHan, McpEngine::JnIpm,
    };
    const vector<McpEngineRow> rows = makeMcpEngineRows(model, engines, params);

    // A row passes iff it converges AND matches the PATH equilibrium. Pinned
    // at TWO passing rows -- the verified 2026-07-06 run had exactly ssn and
    // jn+ipm succeed -- so a regression in either engine is caught rather
    // than masked by the other.
    const int passed =
        countConvergedRows(rows, z0, { printGamsOutputs(), checkPathEquilibrium() });
    EXPECT_GE(passed, 2)
        << "fewer than two engines reached the PATH equilibrium on the "
           "alloceff01cm.gms mixed NCP (known-good: ssn and jn+ipm)";
}
// Copyright Ben Paul Wise. All Rights Reserved.

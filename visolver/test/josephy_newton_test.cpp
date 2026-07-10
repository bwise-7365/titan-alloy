// Copyright Ben Paul Wise. All Rights Reserved.
#include "josephynewton.hpp"
#include "utils.hpp"
#include "vincp.hpp"

#include <Eigen/Dense>
#include <gtest/gtest.h>

#include <random>
#include <stdexcept>
#include <vector>

using namespace VINCP;

// Unit tests for the Josephy-Newton driver's no-progress (stall) cutoff
// (JosephyNewtonParams::stallIterMax / stallRelDecrease, added 2026-07-06
// after the deploy_v07 acceptance run burned ~22 frozen outer iterations at ~1 s
// each). The stall is driven deterministically through the InnerSolver seam
// with a FROZEN inner solver that returns its start point unchanged -- the
// outer step direction is then exactly zero, so the residual can never
// improve -- the same technique the NS1 rescue tests use (drive the protocol
// with a deliberately failing component, not a hard problem).

namespace {
    constexpr Index kDim = 2;              // tiny affine model suffices

    // Affine model F(z) = z + qValue over R_+^kDim (n = 0, pure NCP); the
    // solution is z* = max(0, -qValue) componentwise.
    VIModel
    affineModel(double qValue)
    {
        return makeVIModel(0, kDim, [qValue](const VectorXd& z) -> VectorXd {
            return z + VectorXd::Constant(kDim, qValue);
        });
    }

    // Inner "solver" frozen at the start point (see the file header).
    InnerSolver
    frozenInner()
    {
        return [](const VectorXd& x0, const MatrixXd&, const VectorXd&,
                  const Projector&) -> VIResult {
            return VIResult{ x0, 0.0, 1, true, 0 };
        };
    }

    constexpr int kStallIterMax = 3;    // cutoff under test
    constexpr int kOuterIterMax = 100;  // far above the cutoff
    constexpr int kSmallCap     = 6;    // cap for the disabled-guard control

    constexpr double kInnerMagTol  = 1.0e-14;  // healthy-run inner tolerance
    constexpr int    kInnerIterMax = 100000;
} // namespace

// Frozen loop + enabled guard: stops honestly at the cutoff, long before the
// outer cap, with converged = false and the start point's residual intact.
TEST(JosephyNewton, StallCutoffStopsFrozenLoopEarly) {
    const VIModel  model = affineModel(1.0);            // z* = 0
    const VectorXd z0    = VectorXd::Constant(kDim, 1.0);  // start away from z*

    JosephyNewtonParams params;
    params.outerIterMax = kOuterIterMax;
    params.stallIterMax = kStallIterMax;

    const VIResult r = solveVI(model, z0, frozenInner(), params);
    EXPECT_FALSE(r.converged);
    EXPECT_EQ(kStallIterMax, r.iter);
    // r(z0) = z0 here (z0 - F(z0) is negative, so the projection is 0), and
    // the frozen loop never moved: residual = ||z0||^2 exactly.
    EXPECT_NEAR(static_cast<double>(kDim), r.residual, 1.0e-12);
}

// Same frozen loop with the guard DISABLED (default): historical behavior,
// the loop honestly burns the whole outer cap.
TEST(JosephyNewton, StallCutoffDisabledRunsToOuterCap) {
    const VIModel  model = affineModel(1.0);
    const VectorXd z0    = VectorXd::Constant(kDim, 1.0);

    JosephyNewtonParams params;
    params.outerIterMax = kSmallCap;

    const VIResult r = solveVI(model, z0, frozenInner(), params);
    EXPECT_FALSE(r.converged);
    EXPECT_EQ(kSmallCap, r.iter);
}

// A healthy run with the guard enabled: every outer step improves, so the
// cutoff must not fire and the solve converges as before.
TEST(JosephyNewton, StallCutoffDoesNotTriggerOnHealthyRun) {
    const VIModel  model = affineModel(-1.0);           // z* = 1
    const VectorXd z0    = VectorXd::Zero(kDim);

    JosephyNewtonParams params;
    params.outerIterMax = kOuterIterMax;
    params.stallIterMax = 2;   // deliberately tight

    const InnerSolver inner = makeBsHe94bSolver(kInnerMagTol, kInnerIterMax, 0);
    const VIResult r = solveVI(model, z0, inner, params);
    EXPECT_TRUE(r.converged);
    EXPECT_NEAR(1.0, r.z(0), 1.0e-6);
    EXPECT_NEAR(1.0, r.z(1), 1.0e-6);
}

namespace {
    // A monotone cubic mixed VI with a known solution, for the forcing-
    // sequence and vanilla-entry tests (the han_vs_he scaffold, small).
    struct KnownCubic {
        VIModel model;
        VectorXd zStar;
    };

    KnownCubic
    makeKnownCubic()
    {
        std::mt19937 rng(20260706u);
        const Index n = 3, m = 3, d = n + m;
        VectorXd wStar, yStar;
        makeComplementaryPair(m, rng, 1, 10, wStar, yStar);
        KnownCubic k;
        k.zStar.resize(d);
        k.zStar << VectorXd::Constant(n, 1.0), yStar;
        VectorXd target(d);
        target << VectorXd::Zero(n), wStar;
        const CubicProblem prob = makeCubicProblem(d, d, rng, k.zStar, target,
                                                   /*forcePSD=*/true, -1.0, 1.0);
        k.model = makeVIModel(n, m, prob.F);
        return k;
    }

    constexpr double kCubicSolTol = 1.0e-4;   // above the 1e-10 squared stop
} // namespace

// The forcing-sequence overload solves the cubic, and the recorded inner
// tolerances show the schedule doing its job: capped (loose) while the
// residual is large, tightened far below the cap by the end.
TEST(JosephyNewton, ForcingSequenceSolvesAndLoosensEarly) {
    const KnownCubic k = makeKnownCubic();
    const VectorXd z0 = VectorXd::Zero(k.zStar.size());

    std::vector<double> requestedTols;
    const InnerSolverFactory factory = [&requestedTols](double innerTol) {
        requestedTols.push_back(innerTol);
        return makeBsHe94bSolver(innerTol, 20000, 0);
    };
    JosephyNewtonParams params;
    params.outerTol = 1.0e-10;

    const VIResult r = solveVI(k.model, z0, factory, params);
    EXPECT_TRUE(r.converged);
    EXPECT_LT((r.z - k.zStar).norm(), kCubicSolTol);

    ASSERT_GE(requestedTols.size(), 2u);
    EXPECT_EQ(params.forcingCap, requestedTols.front());   // large residual: capped
    EXPECT_LT(requestedTols.back(), 1.0e-6);               // near solution: tight
    EXPECT_GE(requestedTols.back(), params.forcingFloor);  // never below the floor
}

// The one-call vanilla entry solves the same problem with defaults only.
TEST(JosephyNewton, VanillaEntrySolvesInOneCall) {
    const KnownCubic k = makeKnownCubic();
    const VIResult r = solveVIVanilla(k.model, VectorXd::Zero(k.zStar.size()));
    EXPECT_TRUE(r.converged);
    EXPECT_LT((r.z - k.zStar).norm(), kCubicSolTol);
}

// Forcing-overload guards: unset factory, out-of-range forcing parameters,
// and a factory that returns an empty solver.
TEST(JosephyNewton, ForcingOverloadRejectsBadInputs) {
    const KnownCubic k = makeKnownCubic();
    const VectorXd z0 = VectorXd::Zero(k.zStar.size());
    const InnerSolverFactory good = [](double tol) {
        return makeBsHe94bSolver(tol, 20000, 0);
    };

    EXPECT_THROW(solveVI(k.model, z0, InnerSolverFactory{}),
                 std::invalid_argument);

    JosephyNewtonParams badRatio;
    badRatio.forcingRatio = 1.0;
    EXPECT_THROW(solveVI(k.model, z0, good, badRatio), std::invalid_argument);

    JosephyNewtonParams badFloor;
    badFloor.forcingFloor = 1.0;   // above the default cap
    EXPECT_THROW(solveVI(k.model, z0, good, badFloor), std::invalid_argument);

    const InnerSolverFactory empty = [](double) { return InnerSolver{}; };
    EXPECT_THROW(solveVI(k.model, z0, empty), std::runtime_error);
}

// Parameter guards: negative cutoff and out-of-range relative decrease throw.
TEST(JosephyNewton, RejectsBadStallParams) {
    const VIModel  model = affineModel(1.0);
    const VectorXd z0    = VectorXd::Constant(kDim, 1.0);

    JosephyNewtonParams negativeCutoff;
    negativeCutoff.stallIterMax = -1;
    EXPECT_THROW(solveVI(model, z0, frozenInner(), negativeCutoff),
                 std::invalid_argument);

    JosephyNewtonParams decreaseTooBig;
    decreaseTooBig.stallRelDecrease = 1.0;
    EXPECT_THROW(solveVI(model, z0, frozenInner(), decreaseTooBig),
                 std::invalid_argument);

    JosephyNewtonParams decreaseNegative;
    decreaseNegative.stallRelDecrease = -0.1;
    EXPECT_THROW(solveVI(model, z0, frozenInner(), decreaseNegative),
                 std::invalid_argument);
}
// Copyright Ben Paul Wise. All Rights Reserved.

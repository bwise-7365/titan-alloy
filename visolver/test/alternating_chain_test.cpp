// Copyright Ben Paul Wise. All Rights Reserved.
#include "alternatingchain.hpp"
#include "josephynewton.hpp"
#include "semismoothnewton.hpp"
#include "vincp.hpp"

#include <Eigen/Dense>
#include <gtest/gtest.h>

#include <stdexcept>

using namespace VINCP;

// Unit tests for alternatingChainSolve. The composition logic (best-point
// memory, throw-as-stall, the improvement cutoff) is driven deterministically
// through the StageSolver seam with FAKE stages returning fixed points -- the
// NS1 lesson: test a protocol with a deliberately controlled component, not a
// hard problem. One case runs the real engines end to end on a small monotone
// LCP as an integration check; the chain's real nonmonotone workout is the
// deploy_v07 acceptance test (gams_deploy_test).

namespace {
    constexpr Index kDim = 2;   // pure NCP over R_+^2 (n = 0)

    // Affine model F(z) = z + qValue: solution z* = max(0, -qValue), and the
    // squared natural residual of any point is easy to reason about.
    VIModel
    affineModel(double qValue)
    {
        return makeVIModel(0, kDim, [qValue](const VectorXd& z) -> VectorXd {
            return z + VectorXd::Constant(kDim, qValue);
        });
    }

    // A fake stage frozen at one point, with an invocation counter.
    StageSolver
    fixedStage(const VectorXd& point, int& calls)
    {
        return [point, &calls](const VectorXd&) -> VIResult {
            ++calls;
            return VIResult{ point, 0.0, 1, false, 0 };
        };
    }

    StageSolver
    throwingStage(int& calls)
    {
        return [&calls](const VectorXd&) -> VIResult {
            ++calls;
            throw std::runtime_error("deliberate stage failure");
        };
    }

    constexpr double kMagTol = 1.0e-10;   // chain acceptance (squared)
} // namespace

// Integration: real globalizer (Josephy-Newton + bsHe94b) and real finisher
// (semismooth Newton) on a monotone LCP -- converges in round one, and the
// chain reports the solution with an honest converged flag.
TEST(AlternatingChain, RealEnginesSolveMonotoneLcp) {
    const VIModel  model = affineModel(-1.0);   // z* = (1, 1)
    const VectorXd z0    = VectorXd::Zero(kDim);

    JosephyNewtonParams jnParams;
    const InnerSolver inner = makeBsHe94bSolver(1.0e-14, 100000, 0);
    const StageSolver globalizer = [model, inner, jnParams](const VectorXd& start) {
        return solveVI(model, start, inner, jnParams);
    };
    SemismoothNewtonParams ssnParams;
    const StageSolver finisher = [model, ssnParams](const VectorXd& start) {
        return semismoothNewtonSolve(model, start, ssnParams);
    };

    AlternatingChainParams params;
    params.magTol = kMagTol;
    const VIResult r = alternatingChainSolve(model, z0, globalizer, finisher, params);
    EXPECT_TRUE(r.converged);
    EXPECT_NEAR(1.0, r.z(0), 1.0e-5);
    EXPECT_NEAR(1.0, r.z(1), 1.0e-5);
}

// Best-point memory: the finisher lands on a WORSE point than the globalizer;
// the chain must return the globalizer's point, judged by its own recomputed
// natural residual, with converged = false.
TEST(AlternatingChain, ReturnsBestVisitedPoint) {
    const VIModel  model = affineModel(-1.0);            // z* = (1, 1)
    const VectorXd z0    = VectorXd::Zero(kDim);         // residual 2.0
    const VectorXd zGood = VectorXd::Constant(kDim, 0.9);   // residual 0.02
    const VectorXd zBad  = VectorXd::Constant(kDim, 0.5);   // residual 0.5

    int globalizerCalls = 0, finisherCalls = 0;
    AlternatingChainParams params;
    params.magTol    = kMagTol;
    params.roundsMax = 1;
    const VIResult r = alternatingChainSolve(model, z0,
                                             fixedStage(zGood, globalizerCalls),
                                             fixedStage(zBad, finisherCalls),
                                             params);
    EXPECT_FALSE(r.converged);
    EXPECT_NEAR(0.9, r.z(0), 1.0e-12);
    EXPECT_NEAR(0.9, r.z(1), 1.0e-12);
    EXPECT_NEAR(0.02, r.residual, 1.0e-12);   // 2 * (1 - 0.9)^2, recomputed
    EXPECT_EQ(1, globalizerCalls);
    EXPECT_EQ(1, finisherCalls);
}

// Throw-as-stall: a globalizer that always throws must not fail the chain;
// the finisher still runs (from the projected round start) and its exact
// solution converges the chain.
TEST(AlternatingChain, ThrowingStageIsAbsorbed) {
    const VIModel  model = affineModel(-1.0);
    const VectorXd z0    = VectorXd::Zero(kDim);
    const VectorXd zStar = VectorXd::Constant(kDim, 1.0);   // exact solution

    int globalizerCalls = 0, finisherCalls = 0;
    AlternatingChainParams params;
    params.magTol = kMagTol;
    const VIResult r = alternatingChainSolve(model, z0,
                                             throwingStage(globalizerCalls),
                                             fixedStage(zStar, finisherCalls),
                                             params);
    EXPECT_TRUE(r.converged);
    EXPECT_NEAR(0.0, r.residual, 1.0e-15);
    EXPECT_EQ(1, globalizerCalls);
    EXPECT_EQ(1, finisherCalls);
}

// Improvement cutoff: stages frozen at one mediocre point improve the best
// hugely in round 1 and not at all in round 2 -- the chain must stop after
// round 2, well short of the round cap.
TEST(AlternatingChain, StopsWhenRoundsStopImproving) {
    const VIModel  model = affineModel(-1.0);
    const VectorXd z0    = VectorXd::Zero(kDim);             // residual 2.0
    const VectorXd zMid  = VectorXd::Constant(kDim, 0.8);    // residual 0.08

    int globalizerCalls = 0, finisherCalls = 0;
    AlternatingChainParams params;
    params.magTol    = kMagTol;
    params.roundsMax = 5;
    const VIResult r = alternatingChainSolve(model, z0,
                                             fixedStage(zMid, globalizerCalls),
                                             fixedStage(zMid, finisherCalls),
                                             params);
    EXPECT_FALSE(r.converged);
    EXPECT_NEAR(0.08, r.residual, 1.0e-12);
    EXPECT_EQ(2, globalizerCalls);   // round 1 improves, round 2 does not, stop
    EXPECT_EQ(2, finisherCalls);
}

// Perturb-restart: an IDENTITY stage returns its start unchanged, so no round
// can improve and the chain is at a deterministic fixed point from round one.
// With perturbation disabled the chain must stop after that first stagnant
// round; with perturbation enabled the stagnant rounds restart from jiggled
// points and the chain must spend its full round budget trying.
TEST(AlternatingChain, PerturbRestartKeepsStagnantChainTrying) {
    const VIModel  model = affineModel(-1.0);
    const VectorXd z0    = VectorXd::Zero(kDim);
    const StageSolver identity = [](const VectorXd& start) -> VIResult {
        return VIResult{ start, 0.0, 1, false, 0 };
    };

    AlternatingChainParams off;
    off.magTol    = kMagTol;
    off.roundsMax = 5;
    int offGlobalizerCalls = 0;
    const StageSolver countingIdentityOff = [&](const VectorXd& start) {
        ++offGlobalizerCalls;
        return identity(start);
    };
    const VIResult rOff = alternatingChainSolve(model, z0, countingIdentityOff,
                                                identity, off);
    EXPECT_FALSE(rOff.converged);
    EXPECT_EQ(1, offGlobalizerCalls);   // one stagnant round, then stop

    AlternatingChainParams on = off;
    on.perturbScale = 0.1;
    int onGlobalizerCalls = 0;
    const StageSolver countingIdentityOn = [&](const VectorXd& start) {
        ++onGlobalizerCalls;
        return identity(start);
    };
    const VIResult rOn = alternatingChainSolve(model, z0, countingIdentityOn,
                                               identity, on);
    EXPECT_FALSE(rOn.converged);
    EXPECT_EQ(on.roundsMax, onGlobalizerCalls);   // perturbed retries to the cap
}

// Parameter and input guards.
TEST(AlternatingChain, RejectsBadInputs) {
    const VIModel  model = affineModel(-1.0);
    const VectorXd z0    = VectorXd::Zero(kDim);
    int calls = 0;
    const StageSolver stage = fixedStage(z0, calls);

    EXPECT_THROW(alternatingChainSolve(model, z0, StageSolver{}, stage),
                 std::invalid_argument);
    EXPECT_THROW(alternatingChainSolve(model, z0, stage, StageSolver{}),
                 std::invalid_argument);
    EXPECT_THROW(alternatingChainSolve(model, VectorXd::Zero(kDim + 1), stage, stage),
                 std::invalid_argument);

    AlternatingChainParams zeroRounds;
    zeroRounds.roundsMax = 0;
    EXPECT_THROW(alternatingChainSolve(model, z0, stage, stage, zeroRounds),
                 std::invalid_argument);

    AlternatingChainParams badFactor;
    badFactor.improveFactor = 1.5;
    EXPECT_THROW(alternatingChainSolve(model, z0, stage, stage, badFactor),
                 std::invalid_argument);

    AlternatingChainParams badTol;
    badTol.magTol = 0.0;
    EXPECT_THROW(alternatingChainSolve(model, z0, stage, stage, badTol),
                 std::invalid_argument);

    AlternatingChainParams badPerturb;
    badPerturb.perturbScale = -0.1;
    EXPECT_THROW(alternatingChainSolve(model, z0, stage, stage, badPerturb),
                 std::invalid_argument);
}
// Copyright Ben Paul Wise. All Rights Reserved.

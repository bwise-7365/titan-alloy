// Copyright Ben Paul Wise. All Rights Reserved.
#include "flownewton.hpp"
#include "flowplan.hpp"
#include "greedy.hpp"
#include "gentlesupport.hpp"

#include <gtest/gtest.h>

#include <cmath>
#include <cstdint>
#include <stdexcept>

using namespace VINCP;
using namespace VINCP::Network;
using namespace VINCP::Network::TestSupport;

// GoogleTest suite for the NS2 structured Newton factory (flownewton.hpp).
// The algebra is machine-verified symbolically (ns2-newton-check.mac); these
// tests verify the TRANSCRIPTION numerically: the factory's solve must agree
// with a dense LU of the same K = M + diag(sOverY) on real flow LCPs, at
// every slice shape and scaling the production path can produce.

namespace {

    const std::uint64_t kSeed = 20260703;

    // Parity bars. kBackwardTol bounds the scale-aware backward error
    // ||K d - rhs|| / (||K||_inf ||d|| + ||rhs||), the metric that stays
    // valid at any conditioning -- the sharp transcription check. kDirectTol
    // bounds ||d_flow - d_dense|| / ||d_dense|| on well-scaled systems as a
    // sanity check; each solver's FORWARD error is ~ cond(K) * eps, so this
    // bar carries the system's conditioning and is deliberately looser.
    const double kDirectTol = 1.0e-8;
    const double kBackwardTol = 1.0e-12;

    // Nondimensionalize the way solveFlowPlan does internally, so the LCP is
    // the well-scaled system the engine actually factors (the raw units mix
    // ~1e7 budget rows with ~1e-5 curvatures).
    Instance scaleInstance(Instance inst) {
        const double tonScale = inst.demand.maxCoeff();
        const double mileScale = inst.cost.maxCoeff();
        inst.supplyCap /= tonScale;
        inst.demand /= tonScale;
        inst.cost /= mileScale;
        inst.tonMileLimit /= tonScale * mileScale;
        return inst;
    }

    // A 15-node instance mirroring flowlcp_test's shape: several sinks with
    // multi-pair slices under the keep-all screen. Calibrated, then scaled.
    Instance makeMediumInstance(std::uint64_t seed) {
        InstanceProfile profile;
        profile.numSupplyOnly = 4;
        profile.numBoth = 4;
        profile.numDemandOnly = 6;
        profile.numNeither = 1;
        Instance inst = makeRandomInstance(profile, seed);
        inst.tonMileLimit = greedyPlan(inst).suggestedLimit;
        return scaleInstance(inst);
    }

    FlowLcp buildLcp(const Instance& inst, Index maxSourcesPerSink) {
        const ShortestRoutes routes = computeShortestRoutes(inst);
        const ReducedProblem reduced =
            makeReducedProblem(inst, routes, maxSourcesPerSink);
        return buildFlowLcp(inst, reduced, defaultTieBreakEpsilon(inst));
    }

    // Deterministic sOverY sweeping the given log10 range across the vector:
    // entry i gets 10^(lo + (hi - lo) * i / (dim - 1)), scrambled by stride
    // so neighboring slots differ sharply (as real complementarity diagonals
    // do near a degenerate face).
    VectorXd makeSpreadDiagonal(Index dim, double log10Lo, double log10Hi) {
        VectorXd s(dim);
        const Index stride = 7;                  // coprime with typical dims
        for (Index i = 0; i < dim; ++i) {
            const Index slot = (i * stride) % dim;
            const double frac =
                (1 == dim) ? 0.0
                           : static_cast<double>(slot) / static_cast<double>(dim - 1);
            s(i) = std::pow(10.0, log10Lo + (log10Hi - log10Lo) * frac);
        }
        return s;
    }

    // Deterministic rhs with mixed signs and scales.
    VectorXd makeRhs(Index dim) {
        VectorXd rhs(dim);
        for (Index i = 0; i < dim; ++i) {
            const double sign = (0 == i % 2) ? 1.0 : -1.0;
            rhs(i) = sign * (1.0 + static_cast<double>(i % 13));
        }
        return rhs;
    }

    double backwardError(const MatrixXd& K, const VectorXd& d,
                         const VectorXd& rhs) {
        const double scale = K.cwiseAbs().rowwise().sum().maxCoeff() * d.norm()
                             + rhs.norm();
        return (K * d - rhs).norm() / scale;
    }

    // Assert factory-vs-dense parity on one (lcp, sOverY, rhs) triple.
    void expectParity(const FlowLcp& lcp, const VectorXd& sOverY,
                      bool directCompareP) {
        const Index dim = lcp.numPairs + lcp.numSources + 1;
        MatrixXd K = lcp.M;
        K.diagonal() += sOverY;
        const VectorXd rhs = makeRhs(dim);

        const VectorXd dDense = PartialPivLU<MatrixXd>(K).solve(rhs);
        const NewtonSolve solve = makeFlowNewtonFactory(lcp)(sOverY, 0.0);
        const VectorXd dFlow = solve(rhs);

        ASSERT_TRUE(dFlow.allFinite());
        EXPECT_LT(backwardError(K, dFlow, rhs), kBackwardTol);
        if (directCompareP) {
            EXPECT_LT((dFlow - dDense).norm() / dDense.norm(), kDirectTol);
        }
    }

} // namespace

// Keep-all screen (multi-pair slices, the last slice ending exactly at the
// end of the t block): the structured solve must match a dense LU of the
// identical K, both directly and in backward error, on a moderate diagonal.
TEST(NetworkFlowNewton, ParityKeepAllModerateDiagonal) {
    const Instance inst = makeMediumInstance(kSeed);
    const FlowLcp lcp = buildLcp(inst, 0);
    const Index dim = lcp.numPairs + lcp.numSources + 1;
    expectParity(lcp, makeSpreadDiagonal(dim, -2.0, 2.0), true);
}

// Late-iteration regime: the complementarity diagonal spans 1e-6..1e6 (the
// central path pushes s/y to both extremes as mu -> 0). Direct d-vs-d
// comparison is no longer meaningful at this conditioning, so the claim is
// the scale-aware backward error.
TEST(NetworkFlowNewton, ParityKeepAllExtremeDiagonal) {
    const Instance inst = makeMediumInstance(kSeed);
    const FlowLcp lcp = buildLcp(inst, 0);
    const Index dim = lcp.numPairs + lcp.numSources + 1;
    expectParity(lcp, makeSpreadDiagonal(dim, -6.0, 6.0), false);
}

// k_n = 1 everywhere: the count screen at 1 makes every sink slice a single
// pair, the Sherman-Morrison edge case (rank-one correction on a 1-slice).
TEST(NetworkFlowNewton, ParitySingletonSlices) {
    const Instance inst = makeMediumInstance(kSeed);
    const FlowLcp lcp = buildLcp(inst, 1);
    ASSERT_EQ(lcp.numPairs,
              static_cast<Index>(10));           // one pair per sink (10 sinks)
    const Index dim = lcp.numPairs + lcp.numSources + 1;
    expectParity(lcp, makeSpreadDiagonal(dim, -2.0, 2.0), true);
}

// Extreme Q_n spread: priorities spanning 1e-2..1e2 against single-digit
// demands push Q_n = 2 P_n / D_n^2 across ~6 orders of magnitude, stressing
// the per-sink coefficient c_n = Q_n / (1 + Q_n sigma_n) at both ends.
TEST(NetworkFlowNewton, ParityExtremeQuadSpread) {
    InstanceProfile profile;
    profile.numSupplyOnly = 4;
    profile.numBoth = 4;
    profile.numDemandOnly = 6;
    profile.numNeither = 1;
    profile.demandLo = 2.0;
    profile.demandHi = 10.0;
    profile.priorityLo = 0.01;
    profile.priorityHi = 100.0;
    Instance inst = makeRandomInstance(profile, kSeed);
    inst.tonMileLimit = greedyPlan(inst).suggestedLimit;
    inst = scaleInstance(inst);

    const FlowLcp lcp = buildLcp(inst, 0);
    const Index dim = lcp.numPairs + lcp.numSources + 1;
    expectParity(lcp, makeSpreadDiagonal(dim, -2.0, 2.0), false);
}

// End-to-end through the production path: solveFlowPlan with engine "ipm"
// must produce the same certified optimum under "dense" and "flow" Newton
// linear algebra on the gentle 5-node instance -- with the engine's
// newtonCheckTol drift guard ON for the structured run, so every predictor
// and corrector solve is verified against M inside the engine itself.
TEST(NetworkFlowNewton, EndToEndIpmParity) {
    const double kShortfallTol = 1.0e-8;
    const double kZTol = 1.0e-5;
    const double kCheckTol = 1.0e-8;             // squared drift bar

    const Instance inst = makeGentleInstance(kSeed);

    FlowPlanParams dense;
    dense.engine = "ipm";
    dense.iterMax = 100;
    const FlowPlanResult viaDense = solveFlowPlan(inst, dense);

    FlowPlanParams flow = dense;
    flow.ipmNewton = "flow";
    flow.newtonCheckTol = kCheckTol;
    const FlowPlanResult viaFlow = solveFlowPlan(inst, flow);

    ASSERT_TRUE(viaDense.vi.converged);
    ASSERT_TRUE(viaFlow.vi.converged);
    EXPECT_TRUE(viaDense.certifiedP);
    EXPECT_TRUE(viaFlow.certifiedP);
    EXPECT_NEAR(viaDense.shortfall, viaFlow.shortfall, kShortfallTol);
    ASSERT_EQ(viaDense.vi.z.size(), viaFlow.vi.z.size());
    EXPECT_LT((viaDense.vi.z - viaFlow.vi.z).norm(), kZTol);
}

// Guards: the factory validates its inputs and its lcp rather than proceed.
TEST(NetworkFlowNewton, RejectsBadInputs) {
    const Instance inst = makeMediumInstance(kSeed);
    const FlowLcp lcp = buildLcp(inst, 0);
    const Index dim = lcp.numPairs + lcp.numSources + 1;
    const NewtonSolverFactory factory = makeFlowNewtonFactory(lcp);
    const VectorXd good = VectorXd::Constant(dim, 1.0);

    // The flow LCP has no free block: nonzero freeRegularization is misuse.
    EXPECT_THROW(factory(good, 1.0e-8), std::invalid_argument);

    // Wrong-sized or non-positive complementarity diagonal.
    EXPECT_THROW(factory(VectorXd::Constant(dim + 1, 1.0), 0.0),
                 std::invalid_argument);
    VectorXd nonPositive = good;
    nonPositive(0) = 0.0;
    EXPECT_THROW(factory(nonPositive, 0.0), std::invalid_argument);

    // A wrong-sized rhs is refused by the returned solver.
    const NewtonSolve solve = factory(good, 0.0);
    EXPECT_THROW(solve(VectorXd::Constant(dim - 1, 1.0)),
                 std::invalid_argument);

    // An inconsistent lcp is refused at factory construction.
    EXPECT_THROW(makeFlowNewtonFactory(FlowLcp{}), std::invalid_argument);

    // The ipmNewton param is validated by solveFlowPlan.
    FlowPlanParams bad;
    bad.engine = "ipm";
    bad.ipmNewton = "sparse";
    EXPECT_THROW(solveFlowPlan(inst, bad), std::invalid_argument);
}
// Copyright Ben Paul Wise. All Rights Reserved.

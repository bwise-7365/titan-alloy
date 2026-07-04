// Copyright Ben Paul Wise. All Rights Reserved.
#include "oracle.hpp"

#include "bshe94b.hpp"
#include "flowlcp.hpp"
#include "gentlesupport.hpp"
#include "greedy.hpp"
#include "lanesupport.hpp"

#include <gtest/gtest.h>

#include <cstdint>
#include <stdexcept>

using namespace VINCP;
using namespace VINCP::Network;
using namespace VINCP::Network::TestSupport;

namespace {

    const std::uint64_t kSeed = 20260703;
    const double kBigBudget = 1.0e5;

    // Comparison tolerances. Solver iterates carry natural-map residuals of
    // ~sqrt(magTol), so plans are feasible and optimal only to that scale.
    const double kSolverFeasTol = 1.0e-4;
    const double kResupplyTol = 1.0e-2;
    const double kShortfallTol = 1.0e-3;

} // namespace

// Assembly sanity on the hand lane: block offsets, the q slots, and
// monotonicity (PSD symmetric part) of the full-formulation KKT matrix.
TEST(NetworkOracle, KktLayoutAndMonotonicity) {
    const Instance inst = makeLaneInstance(kBigBudget);
    const OracleKkt kkt = buildOracleKkt(inst);

    // 2 nu + 4 f + 2 S + 1 R + 2 delta + 2 mu + 1 lambda = 14.
    const Index kDim = 14;
    ASSERT_EQ(kkt.q.size(), kDim);
    ASSERT_EQ(kkt.M.rows(), kDim);
    ASSERT_EQ(static_cast<Index>(kkt.demandNodes.size()), 1);
    EXPECT_EQ(kkt.demandNodes[0], 1);

    EXPECT_EQ(kkt.q(kkt.muBase + 0), laneCap);
    EXPECT_EQ(kkt.q(kkt.muBase + 1), 0.0);
    EXPECT_EQ(kkt.q(kkt.lambdaIndex), kBigBudget);
    EXPECT_DOUBLE_EQ(kkt.q(kkt.rBase), -2.0 * lanePriority / laneDemand);

    const MatrixXd symmetric = 0.5 * (kkt.M + kkt.M.transpose());
    SelfAdjointEigenSolver<MatrixXd> eigs(symmetric, EigenvaluesOnly);
    ASSERT_EQ(eigs.info(), Success);
    EXPECT_GE(eigs.eigenvalues().minCoeff(), -1.0e-10);
}

// Slack budget: the oracle must deliver the full demand (R* = D, theta* = 0).
// The flow PATTERN is not asserted -- without a tie-break it may licitly
// wander the optimal face (that degeneracy is why the tie-break exists).
TEST(NetworkOracle, SolvesLaneUnconstrained) {
    const Instance inst = makeLaneInstance(kBigBudget);
    const OracleResult result = solveOracle(inst);

    ASSERT_TRUE(result.vi.converged);
    EXPECT_NEAR(result.plan.resupply(1), laneDemand, kResupplyTol);
    EXPECT_LE(result.shortfall, 1.0e-6);
    EXPECT_LE(maxViolation(checkPlan(inst, result.plan)), kSolverFeasTol);
}

// Binding budget L = 2000: only t = L/laneMiles = 4 of the 8 wanted tons can
// move, so R* = 4 and theta* = P (1 - 4/8)^2 = 0.5 exactly.
TEST(NetworkOracle, SolvesLaneBudgetBound) {
    const double kTightBudget = 2000.0;
    const Instance inst = makeLaneInstance(kTightBudget);
    const OracleResult result = solveOracle(inst);

    ASSERT_TRUE(result.vi.converged);
    const double kBoundTons = kTightBudget / laneMiles;
    EXPECT_NEAR(result.plan.resupply(1), kBoundTons, kResupplyTol);
    const double kExpectedShortfall =
        lanePriority * (1.0 - kBoundTons / laneDemand)
        * (1.0 - kBoundTons / laneDemand);
    EXPECT_NEAR(result.shortfall, kExpectedShortfall, kShortfallTol);
    EXPECT_LE(maxViolation(checkPlan(inst, result.plan)), kSolverFeasTol);
}

// THE cross-check (task C3's purpose): on a random instance the production
// path (reduce -> buildFlowLcp -> bsHe94b -> unpack) and the oracle (full
// formulation, dHan06) solve DIFFERENT optimization problems that Lemma R1
// says share their optimum. theta and the unique R* must agree; f need not.
TEST(NetworkOracle, AgreesWithReducedPipeline) {
    const Instance inst = makeGentleInstance(kSeed);

    // Oracle side.
    const OracleResult oracle = solveOracle(inst);
    ASSERT_TRUE(oracle.vi.converged);
    EXPECT_LE(maxViolation(checkPlan(inst, oracle.plan)), kSolverFeasTol);

    // Production side, on the NONDIMENSIONALIZED system (exact unit change;
    // task C4's solveFlowPlan will fold this scaling in — this is a preview).
    const double kMagTol = 1.0e-12;
    const int kIterMax = 200000;
    const double tonScale = inst.demand.maxCoeff();
    const double mileScale = inst.cost.maxCoeff();
    Instance scaled = inst;
    scaled.supplyCap /= tonScale;
    scaled.demand /= tonScale;
    scaled.cost /= mileScale;
    scaled.tonMileLimit /= tonScale * mileScale;

    const ShortestRoutes routes = computeShortestRoutes(scaled);
    const ReducedProblem reduced = makeReducedProblem(scaled, routes);
    const FlowLcp lcp = buildFlowLcp(scaled, reduced,
                                     defaultTieBreakEpsilon(scaled));
    const VectorXd z0 = VectorXd::Zero(lcp.numPairs + lcp.numSources + 1);
    const VIResult vi = bsHe94b(z0, lcp.M, lcp.q, projectNonnegative,
                                kMagTol, kIterMax, 0);
    ASSERT_TRUE(vi.converged);
    Plan produced = unpackFlowLcp(scaled, routes, reduced, lcp, vi.z);
    produced.flow *= tonScale;             // back to real tons
    produced.supplied *= tonScale;
    produced.resupply *= tonScale;
    EXPECT_LE(maxViolation(checkPlan(inst, produced)), kSolverFeasTol);

    // Agreement: optimal value and the unique optimal resupply.
    const double thetaOracle = oracle.shortfall;
    const double thetaProduced = shortfallObjective(inst, produced);
    EXPECT_NEAR(thetaProduced, thetaOracle, kShortfallTol);
    for (Index i = 0; i < inst.numNodes; ++i) {
        if (0.0 < inst.demand(i)) {
            EXPECT_NEAR(produced.resupply(i), oracle.plan.resupply(i),
                        kResupplyTol);
        }
    }

    // Both sit above the budget-unconstrained rationing bound (sandwich).
    // The bound is exact only for exactly-feasible plans; solver iterates are
    // feasible to ~residual scale, and dtheta/dR ~ 2P/D ~ O(1) per ton here,
    // so allow the bound to be undercut by that much.
    const double kSandwichSlack = 1.0e-4;
    const double thetaRation =
        shortfallOfResupply(inst, rationTargets(inst));
    EXPECT_GE(thetaProduced, thetaRation - kSandwichSlack);
    EXPECT_GE(thetaOracle, thetaRation - kSandwichSlack);
}

// The oracle refuses what it is not for: big instances (the dense full
// formulation would be huge) and uncalibrated budgets.
TEST(NetworkOracle, RejectsBigOrUncalibrated) {
    const InstanceProfile bigProfile;   // the 70-node default
    Instance big = makeRandomInstance(bigProfile, kSeed);
    big.tonMileLimit = 1.0;
    EXPECT_THROW(solveOracle(big), std::invalid_argument);

    const Instance uncalibrated = makeLaneInstance(0.0);
    EXPECT_THROW(buildOracleKkt(uncalibrated), std::invalid_argument);
}
// Copyright Ben Paul Wise. All Rights Reserved.

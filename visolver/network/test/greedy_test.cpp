// Copyright Ben Paul Wise. All Rights Reserved.
#include "greedy.hpp"

#include <gtest/gtest.h>

#include <cstdint>

using namespace VIMCP;
using namespace VIMCP::Network;

namespace {

    const std::uint64_t kSeed = 20260703;
    const double kTol = 1.0e-9;

    // Feasibility slack for plans BUILT in floating point: summing ~70 flows
    // of magnitude ~5e3 leaves balance/capacity residuals of order 1e-12
    // (association-order differences between the builder's and the checker's
    // Eigen sums). Hand-built plans still assert exact zero.
    const double kFeasTol = 1.0e-9;

    // One source node (index 0) plus two demand nodes, with every cost
    // positive; capacities/demands/priorities are the test's knobs.
    Instance makeThreeNodeInstance(double cap0,
                                   double demand1, double priority1,
                                   double demand2, double priority2) {
        Instance inst;
        inst.numNodes = 3;
        inst.supplyCap = VectorXd(3);
        inst.supplyCap << cap0, 0.0, 0.0;
        inst.demand = VectorXd(3);
        inst.demand << 0.0, demand1, demand2;
        inst.priority = VectorXd(3);
        inst.priority << 1.0, priority1, priority2;
        inst.cost = MatrixXd(3, 3);
        inst.cost << 2.0,   150.0, 250.0,
                     160.0,   2.0, 400.0,
                     260.0, 410.0,   2.0;
        validateInstance(inst);
        return inst;
    }

} // namespace

// No shortfall (total demand <= total capacity): targets are exactly D,
// including the exact-boundary case sum D == sum C (the G1 boundary).
TEST(NetworkGreedy, RationingAllMeetable) {
    const double kCap = 100.0;
    const Instance slack = makeThreeNodeInstance(kCap, 60.0, 1.0, 30.0, 2.0);
    const VectorXd slackTargets = rationTargets(slack);
    EXPECT_EQ(slackTargets(1), 60.0);
    EXPECT_EQ(slackTargets(2), 30.0);

    const Instance boundary = makeThreeNodeInstance(kCap, 60.0, 1.0, 40.0, 2.0);
    const VectorXd boundaryTargets = rationTargets(boundary);
    EXPECT_EQ(boundaryTargets(1), 60.0);
    EXPECT_EQ(boundaryTargets(2), 40.0);
}

// Symmetric shortfall: two identical demand nodes sharing half the capacity
// split it evenly (lambda = (200 - 100)/20000, R_i = 50).
TEST(NetworkGreedy, RationingSplitsSymmetricShortfall) {
    const Instance inst = makeThreeNodeInstance(100.0, 100.0, 1.0, 100.0, 1.0);
    const VectorXd targets = rationTargets(inst);
    EXPECT_NEAR(targets(1), 50.0, kTol);
    EXPECT_NEAR(targets(2), 50.0, kTol);
    EXPECT_NEAR(targets.sum(), 100.0, kTol);   // rations to exactly MR
}

// Clamp case: node 1 (big demand, low priority => huge weight D^2/P) goes
// negative in the interior solve and must be excluded; the re-solve gives all
// 5 tons to node 2. Optimality is hand-checkable: node 2's marginal gain
// dominates node 1's over the whole range.
TEST(NetworkGreedy, RationingClampExcludesAndResolves) {
    const Instance inst = makeThreeNodeInstance(5.0, 100.0, 1.0, 10.0, 10.0);
    const VectorXd targets = rationTargets(inst);
    EXPECT_NEAR(targets(1), 0.0, kTol);
    EXPECT_NEAR(targets(2), 5.0, kTol);
}

// Greedy serves the sole demand node entirely from the cheaper of two sources.
TEST(NetworkGreedy, GreedyUsesCheapestSource) {
    const double kDemand = 5.0;
    Instance inst;
    inst.numNodes = 3;
    inst.supplyCap = VectorXd(3);
    inst.supplyCap << 10.0, 10.0, 0.0;
    inst.demand = VectorXd(3);
    inst.demand << 0.0, 0.0, kDemand;
    inst.priority = VectorXd::Ones(3);
    inst.cost = MatrixXd(3, 3);
    inst.cost << 2.0,   300.0, 100.0,
                 300.0,   2.0, 200.0,
                 110.0, 210.0,   2.0;
    validateInstance(inst);

    const GreedyParams params;
    const GreedyResult result = greedyPlan(inst, params);

    const double kServed = (1.0 - params.demandScaleDown) * kDemand;
    EXPECT_DOUBLE_EQ(result.plan.flow(0, 2), kServed);   // cheap source used
    EXPECT_EQ(result.plan.flow(1, 2), 0.0);              // dear source unused
    EXPECT_EQ(maxViolation(checkPlan(inst, result.plan)), 0.0);
    EXPECT_DOUBLE_EQ(result.tonMilesUsed, kServed * inst.cost(0, 2));
}

// On the full random 70-node profile: the plan is feasible, resupply hits the
// scaled targets, calibration is the requested fraction of usage, and the
// objective ordering theta_greedy >= theta_ration holds (scale-down makes the
// greedy plan deliver slightly less than the unconstrained optimum).
TEST(NetworkGreedy, GreedyPlanServesTargetsOnRandomInstance) {
    const InstanceProfile profile;
    const Instance inst = makeRandomInstance(profile, kSeed);
    const GreedyParams params;
    const GreedyResult result = greedyPlan(inst, params);

    EXPECT_LE(maxViolation(checkPlan(inst, result.plan)), kFeasTol);
    EXPECT_GT(result.tonMilesUsed, 0.0);
    EXPECT_DOUBLE_EQ(result.suggestedLimit,
                     params.budgetFraction * result.tonMilesUsed);
    EXPECT_GT(result.iterations, 0);

    // Every sink receives its scaled target (up to accumulation round-off).
    const VectorXd scaled = (1.0 - params.demandScaleDown) * result.targets;
    const double worstMiss =
        (result.plan.resupply - scaled).cwiseAbs().maxCoeff();
    const double kAccumTol = 1.0e-9 * totalDemand(inst);
    EXPECT_LE(worstMiss, kAccumTol);

    // Sandwich ordering (formulation.md section 7).
    const double thetaRation = shortfallOfResupply(inst, result.targets);
    const double thetaGreedy = shortfallObjective(inst, result.plan);
    EXPECT_GE(thetaGreedy, thetaRation);

    // Targets never exceed what is meetable.
    const double meetable = std::min(totalDemand(inst), totalSupplyCap(inst));
    EXPECT_LE(result.targets.sum(), meetable + kAccumTol);
}
// Copyright Ben Paul Wise. All Rights Reserved.

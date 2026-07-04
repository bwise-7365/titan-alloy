// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
// Tests for the transportation 2-exchange (swap) engine.
// ----------------------------------------------
#include "swap.hpp"

#include <gtest/gtest.h>

using namespace VINCP;
using namespace VINCP::Network;

namespace {

  // Five nodes: 0,1 supply-only; 2,3 demand-only; 4 transit. Costs make the
  // "crossed" assignment 0->2, 1->3 expensive (10 each) and the uncrossed
  // 0->3, 1->2 cheap (1 each), so one swap saves 90 ton-miles.
  const double kCap = 10.0, kDem = 5.0, kBig = 50.0, kSelf = 2.0;

  Instance makeCrossedInstance() {
    Instance inst;
    inst.numNodes = 5;
    inst.supplyCap = VectorXd(5);
    inst.supplyCap << kCap, kCap, 0.0, 0.0, 0.0;
    inst.demand = VectorXd(5);
    inst.demand << 0.0, 0.0, kDem, kDem, 0.0;
    inst.priority = VectorXd::Ones(5);
    inst.cost = MatrixXd::Constant(5, 5, kBig);
    for (Index i = 0; i < 5; ++i) {
      inst.cost(i, i) = kSelf;
    }
    inst.cost(0, 2) = 10.0;
    inst.cost(0, 3) = 1.0;
    inst.cost(1, 2) = 1.0;
    inst.cost(1, 3) = 10.0;
    inst.tonMileLimit = 1000.0;   // generous, so the plan is budget-feasible
    validateInstance(inst);
    return inst;
  }

  // The crossed plan: 0->2 and 1->3, each carrying kDem tons.
  Plan makeCrossedPlan(const Instance& inst) {
    Plan plan = makeZeroPlan(inst);
    plan.flow(0, 2) = kDem;
    plan.flow(1, 3) = kDem;
    plan.supplied = plan.flow.rowwise().sum();
    plan.resupply = plan.flow.colwise().sum().transpose();
    return plan;
  }

} // namespace

// The global best swap uncrosses the assignment, saving exactly 90 ton-miles,
// and leaves every node's delivered/supplied totals unchanged.
TEST(NetworkSwap, GlobalSwapUncrossesAssignment) {
  const Instance inst = makeCrossedInstance();
  Plan plan = makeCrossedPlan(inst);
  ASSERT_EQ(maxViolation(checkPlan(inst, plan)), 0.0);
  EXPECT_DOUBLE_EQ(tonMiles(inst, plan), 100.0);

  const SwapMove move = bestSwap(inst, plan);
  EXPECT_TRUE(move.improvingP);
  EXPECT_DOUBLE_EQ(move.amount, kDem);
  EXPECT_DOUBLE_EQ(move.saving, 90.0);

  const VectorXd resupplyBefore = plan.resupply;
  const VectorXd suppliedBefore = plan.supplied;
  applySwap(plan, move);

  EXPECT_DOUBLE_EQ(tonMiles(inst, plan), 10.0);
  EXPECT_TRUE(plan.resupply.isApprox(resupplyBefore));   // 2-exchange invariant
  EXPECT_TRUE(plan.supplied.isApprox(suppliedBefore));
  EXPECT_EQ(maxViolation(checkPlan(inst, plan)), 0.0);

  // Uncrossed plan is a local optimum: no further improving swap.
  EXPECT_FALSE(bestSwap(inst, plan).improvingP);
}

// Version 1 only considers swaps incident to the clicked node.
TEST(NetworkSwap, NodeLocalSwapRespectsIncidence) {
  const Instance inst = makeCrossedInstance();
  const Plan plan = makeCrossedPlan(inst);

  // Node 0 is a sender of arc (0,2): it finds the uncrossing swap.
  const SwapMove atZero = bestSwapAtNode(inst, plan, 0);
  EXPECT_TRUE(atZero.improvingP);
  EXPECT_DOUBLE_EQ(atZero.saving, 90.0);

  // The transit node 4 touches no flow arc: nothing to swap.
  const SwapMove atTransit = bestSwapAtNode(inst, plan, 4);
  EXPECT_FALSE(atTransit.improvingP);
}

// Version 3 iterates to the local optimum: here exactly one swap, saving 90.
TEST(NetworkSwap, LoopToLocalOptimum) {
  const Instance inst = makeCrossedInstance();
  Plan plan = makeCrossedPlan(inst);

  const SwapSummary summary = swapToLocalOptimum(inst, plan);
  EXPECT_EQ(summary.swaps, 1);
  EXPECT_DOUBLE_EQ(summary.totalSaving, 90.0);
  EXPECT_DOUBLE_EQ(tonMiles(inst, plan), 10.0);
}

namespace {

  // Source 0 sends to sinks 3 and 5; each of those arcs is in its own crossed
  // pair (0-3 with 1-4, 0-5 with 2-6). Driving node 0 to its optimum therefore
  // needs TWO swaps -- more than one, fewer than a global pass.
  Instance makeDoubleCrossInstance() {
    Instance inst;
    inst.numNodes = 7;
    inst.supplyCap = VectorXd(7);
    inst.supplyCap << 10.0, 5.0, 5.0, 0.0, 0.0, 0.0, 0.0;
    inst.demand = VectorXd(7);
    inst.demand << 0.0, 0.0, 0.0, 5.0, 5.0, 5.0, 5.0;
    inst.priority = VectorXd::Ones(7);
    inst.cost = MatrixXd::Constant(7, 7, 50.0);
    for (Index i = 0; i < 7; ++i) {
      inst.cost(i, i) = 2.0;
    }
    inst.cost(0, 3) = 10.0;   // pair A: crossed 0->3, 1->4 ...
    inst.cost(1, 4) = 10.0;
    inst.cost(0, 4) = 1.0;    // ... cheap uncrossed 0->4, 1->3
    inst.cost(1, 3) = 1.0;
    inst.cost(0, 5) = 10.0;   // pair B: crossed 0->5, 2->6 ...
    inst.cost(2, 6) = 10.0;
    inst.cost(0, 6) = 1.0;    // ... cheap uncrossed 0->6, 2->5
    inst.cost(2, 5) = 1.0;
    inst.tonMileLimit = 1000.0;
    validateInstance(inst);
    return inst;
  }

  Plan makeDoubleCrossPlan(const Instance& inst) {
    Plan plan = makeZeroPlan(inst);
    plan.flow(0, 3) = 5.0;
    plan.flow(0, 5) = 5.0;
    plan.flow(1, 4) = 5.0;
    plan.flow(2, 6) = 5.0;
    plan.supplied = plan.flow.rowwise().sum();
    plan.resupply = plan.flow.colwise().sum().transpose();
    return plan;
  }

} // namespace

// The intermediate: driving one node to its swap optimum applies both of its
// improving swaps (and no more), leaving the rest of the plan's totals intact.
TEST(NetworkSwap, NodeToLocalOptimumIterates) {
  const Instance inst = makeDoubleCrossInstance();
  Plan plan = makeDoubleCrossPlan(inst);
  EXPECT_DOUBLE_EQ(tonMiles(inst, plan), 200.0);

  const VectorXd resupplyBefore = plan.resupply;
  const SwapSummary summary = swapNodeToLocalOptimum(inst, plan, 0);

  EXPECT_EQ(summary.swaps, 2);
  EXPECT_DOUBLE_EQ(summary.totalSaving, 180.0);
  EXPECT_DOUBLE_EQ(tonMiles(inst, plan), 20.0);
  EXPECT_TRUE(plan.resupply.isApprox(resupplyBefore));
  EXPECT_FALSE(bestSwapAtNode(inst, plan, 0).improvingP);
}
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------

// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
// Tests for the gravity (proportional all-to-all) planner.
// ----------------------------------------------
#include "gravity.hpp"

#include <gtest/gtest.h>

using namespace VINCP;
using namespace VINCP::Network;

namespace {

  // Two supply-only nodes (0,1) and two demand-only nodes (2,3). Costs are
  // arbitrary positive values -- gravity ignores them.
  Instance makeGravityInstance(double cap0, double cap1, double dem2,
                               double dem3) {
    Instance inst;
    inst.numNodes = 4;
    inst.supplyCap = VectorXd(4);
    inst.supplyCap << cap0, cap1, 0.0, 0.0;
    inst.demand = VectorXd(4);
    inst.demand << 0.0, 0.0, dem2, dem3;
    inst.priority = VectorXd::Ones(4);
    inst.cost = MatrixXd::Constant(4, 4, 100.0);
    for (Index i = 0; i < 4; ++i) {
      inst.cost(i, i) = 2.0;
    }
    inst.tonMileLimit = 1.0e9;   // huge: the cost-blind plan is budget-feasible
    validateInstance(inst);
    return inst;
  }

} // namespace

// total C < total D: suppliers give ALL their capacity; sinks are proportionally
// rationed to totalC/totalD of their demand.
TEST(NetworkGravity, ShortSupplyRationsSinks) {
  const Instance inst = makeGravityInstance(4.0, 6.0, 10.0, 10.0);   // C=10, D=20
  const Plan plan = gravityPlan(inst);

  EXPECT_EQ(maxViolation(checkPlan(inst, plan)), 0.0);
  EXPECT_DOUBLE_EQ(plan.supplied(0), 4.0);   // full capacity
  EXPECT_DOUBLE_EQ(plan.supplied(1), 6.0);
  EXPECT_DOUBLE_EQ(plan.resupply(2), 5.0);   // 10 * 10/20 -> half
  EXPECT_DOUBLE_EQ(plan.resupply(3), 5.0);
}

// total C > total D: sinks get ALL their demand; suppliers give a fixed fraction
// (totalD/totalC) of their capacity.
TEST(NetworkGravity, ShortDemandFractionsSuppliers) {
  const Instance inst = makeGravityInstance(10.0, 10.0, 4.0, 6.0);   // C=20, D=10
  const Plan plan = gravityPlan(inst);

  EXPECT_EQ(maxViolation(checkPlan(inst, plan)), 0.0);
  EXPECT_DOUBLE_EQ(plan.resupply(2), 4.0);   // full demand
  EXPECT_DOUBLE_EQ(plan.resupply(3), 6.0);
  EXPECT_DOUBLE_EQ(plan.supplied(0), 5.0);   // 10 * 10/20 -> half
  EXPECT_DOUBLE_EQ(plan.supplied(1), 5.0);
}
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------

// Copyright Ben Paul Wise. All Rights Reserved.
#include "plan.hpp"

#include <gtest/gtest.h>

#include <cstdint>

using namespace VINCP;
using namespace VINCP::Network;

namespace {

    const std::uint64_t kSeed = 20260703;

    // Hand-checkable two-node instance: node 0 is supply-only, node 1 has both
    // capacity and demand (so it can self-supply through f_11).
    const double kCap0 = 10.0, kCap1 = 6.0;
    const double kDemand1 = 8.0;
    const double kPriority1 = 2.0;
    const double kSelfCost0 = 2.0, kSelfCost1 = 3.0;
    const double kCost01 = 500.0, kCost10 = 520.0;

    Instance makeTwoNodeInstance() {
        Instance inst;
        inst.numNodes = 2;
        inst.supplyCap = VectorXd(2);
        inst.supplyCap << kCap0, kCap1;
        inst.demand = VectorXd(2);
        inst.demand << 0.0, kDemand1;
        inst.priority = VectorXd(2);
        inst.priority << 1.0, kPriority1;
        inst.cost = MatrixXd(2, 2);
        inst.cost << kSelfCost0, kCost01,
                     kCost10,    kSelfCost1;
        validateInstance(inst);
        return inst;
    }

} // namespace

// The zero plan is feasible, moves nothing, and scores the full-scale
// shortfall: every demand node contributes exactly its priority.
TEST(NetworkPlan, ZeroPlanFeasibleWithFullShortfall) {
    const InstanceProfile profile;
    const Instance inst = makeRandomInstance(profile, kSeed);
    const Plan zero = makeZeroPlan(inst);

    EXPECT_EQ(maxViolation(checkPlan(inst, zero)), 0.0);
    EXPECT_EQ(tonMiles(inst, zero), 0.0);

    double fullScale = 0.0;
    for (Index i = 0; i < inst.numNodes; ++i) {
        if (0.0 < inst.demand(i)) {
            fullScale += inst.priority(i);
        }
    }
    const double relTol = 1.0e-12 * fullScale;
    EXPECT_NEAR(shortfallObjective(inst, zero), fullScale, relTol);
}

// A hand-built plan: node 0 ships 5 tons to node 1, node 1 self-supplies 3
// tons through f_11. Feasible; ton-miles and objective are hand-computable.
TEST(NetworkPlan, HandBuiltPlanEvaluates) {
    const Instance inst = makeTwoNodeInstance();
    const double kShipped = 5.0, kSelfSupplied = 3.0;

    Plan plan = makeZeroPlan(inst);
    plan.supplied << kShipped, kSelfSupplied;
    plan.resupply << 0.0, kShipped + kSelfSupplied;
    plan.flow(0, 1) = kShipped;
    plan.flow(1, 1) = kSelfSupplied;

    EXPECT_EQ(maxViolation(checkPlan(inst, plan)), 0.0);
    EXPECT_DOUBLE_EQ(tonMiles(inst, plan),
                     kShipped * kCost01 + kSelfSupplied * kSelfCost1);

    // R_1 = 8 = D_1: zero shortfall at the only demand node.
    EXPECT_DOUBLE_EQ(shortfallObjective(inst, plan), 0.0);
}

// shortfallVsTarget separates "did we hit the (rationed) target" from "how far
// short of the original demand we are" -- the two numbers the viewer reports.
TEST(NetworkPlan, ShortfallVsTargetSeparatesRationedFromOriginal) {
    const Instance inst = makeTwoNodeInstance();   // node 1: D = 8, P = 2
    VectorXd resupply(2);
    resupply << 0.0, 6.0;                          // delivered 6 to node 1
    VectorXd rationed(2);
    rationed << 0.0, 6.0;                          // rationed target was also 6

    // Meeting the rationed target exactly -> zero shortfall against it...
    EXPECT_DOUBLE_EQ(shortfallVsTarget(inst, rationed, resupply), 0.0);
    // ...but a real gap against the original demand of 8.
    const double fraction = (kDemand1 - 6.0) / kDemand1;
    EXPECT_DOUBLE_EQ(shortfallOfResupply(inst, resupply),
                     kPriority1 * fraction * fraction);
    // shortfallOfResupply is exactly shortfallVsTarget with target = demand.
    EXPECT_DOUBLE_EQ(shortfallOfResupply(inst, resupply),
                     shortfallVsTarget(inst, inst.demand, resupply));
}

// The F1 fix: supplying one's own demand WITHOUT routing it through the
// self-arc satisfies (balance) but must be caught by (delivery).
TEST(NetworkPlan, SelfSupplyShortcutIsCaught) {
    const Instance inst = makeTwoNodeInstance();
    const double kShortcut = 5.0;

    Plan shortcut = makeZeroPlan(inst);
    shortcut.supplied(1) = kShortcut;
    shortcut.resupply(1) = kShortcut;

    const PlanViolations violations = checkPlan(inst, shortcut);
    EXPECT_EQ(violations.balance, 0.0);           // balance alone cannot see it
    EXPECT_EQ(violations.delivery, kShortcut);    // (delivery) does

    // Routing the same tons through the self-arc f_11 makes the plan legal.
    Plan routed = shortcut;
    routed.flow(1, 1) = kShortcut;
    EXPECT_EQ(maxViolation(checkPlan(inst, routed)), 0.0);
    EXPECT_DOUBLE_EQ(tonMiles(inst, routed), kShortcut * kSelfCost1);
}

// Each remaining violation family is detected: balance, capacity, negativity,
// idle resupply, and the ton-mile budget (only once L is calibrated).
TEST(NetworkPlan, ViolationFamiliesDetected) {
    const Instance inst = makeTwoNodeInstance();
    const double kAmount = 4.0;

    Plan unbalanced = makeZeroPlan(inst);
    unbalanced.resupply(1) = kAmount;             // demanded from thin air
    EXPECT_EQ(checkPlan(inst, unbalanced).balance, kAmount);

    Plan overCap = makeZeroPlan(inst);
    overCap.supplied(0) = kCap0 + kAmount;
    overCap.flow(0, 1) = kCap0 + kAmount;
    overCap.resupply(1) = kCap0 + kAmount;
    EXPECT_EQ(checkPlan(inst, overCap).capacity, kAmount);

    Plan negative = makeZeroPlan(inst);
    negative.flow(1, 0) = -kAmount;
    EXPECT_EQ(checkPlan(inst, negative).negativity, kAmount);

    Plan idle = makeZeroPlan(inst);
    idle.supplied(1) = kAmount;                   // ship to the no-demand node 0
    idle.flow(1, 0) = kAmount;
    idle.resupply(0) = kAmount;
    EXPECT_EQ(checkPlan(inst, idle).idleResupply, kAmount);

    Instance budgeted = inst;
    budgeted.tonMileLimit = 1.0;                  // far below one shipped ton
    Plan busy = makeZeroPlan(budgeted);
    busy.supplied(0) = kAmount;
    busy.flow(0, 1) = kAmount;
    busy.resupply(1) = kAmount;
    EXPECT_DOUBLE_EQ(checkPlan(budgeted, busy).budget,
                     kAmount * kCost01 - budgeted.tonMileLimit);

    // The identical plan on the UNcalibrated instance reports no budget issue.
    EXPECT_EQ(checkPlan(inst, busy).budget, 0.0);
}
// Copyright Ben Paul Wise. All Rights Reserved.

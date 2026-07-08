// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
// Tests for fleet plan evaluation and the nine-family feasibility checker.
// ----------------------------------------------
#include "fleetplan.hpp"

#include <gtest/gtest.h>

using namespace VINCP;
using namespace VINCP::Network;

namespace {

    // Two nodes (source 0, sink 1), one asset (2 tons + 4 sqft per unit), one
    // vehicle type (10 tons + 100 sqft per vehicle, budget 4*50*24 = 4800
    // vehicle-miles). Distances: 0->1 is 100, 1->0 is 120.
    FleetInstance makeTinyFleetInstance() {
        FleetInstance inst;
        inst.numNodes = 2;
        inst.assets = {{"crate", 2.0, 4.0}};
        inst.vehicles = {{"truck", 10.0, 100.0, 4.0, 50.0}};
        inst.supplyCap = MatrixXd(2, 1);
        inst.supplyCap << 100.0, 0.0;
        inst.demand = MatrixXd(2, 1);
        inst.demand << 0.0, 60.0;
        inst.priority = MatrixXd(2, 1);
        inst.priority << 1.0, 2.0;
        inst.distance = MatrixXd(2, 2);
        inst.distance << 2.0, 100.0,
                         120.0,  3.0;
        inst.horizonHours = 24.0;
        validateFleetInstance(inst);
        return inst;
    }

    // Hand-feasible plan on the tiny instance: 5 units 0 -> 1 (10 tons /
    // 20 sqft) on exactly one truck out-and-back. Every family is exactly 0.
    FleetPlan makeHandPlan(const FleetInstance& inst) {
        FleetPlan plan = makeZeroFleetPlan(inst);
        plan.flow[0](0, 1) = 5.0;
        plan.vehicles[0](0, 1) = 1.0;
        plan.vehicles[0](1, 0) = 1.0;      // deadhead return leg
        plan.supplied(0, 0) = 5.0;
        plan.resupply(1, 0) = 5.0;
        return plan;
    }

} // namespace

TEST(NetworkFleetPlan, ZeroPlanFeasibleWithFullShortfall) {
    const FleetInstance inst = makeTinyFleetInstance();
    const FleetPlan plan = makeZeroFleetPlan(inst);
    EXPECT_EQ(maxViolation(checkFleetPlan(inst, plan)), 0.0);
    // Zero resupply forfeits the whole objective: sum of P over demand cells.
    EXPECT_DOUBLE_EQ(fleetShortfallObjective(inst, plan), 2.0);
}

TEST(NetworkFleetPlan, HandPlanFeasibleAndEvaluated) {
    const FleetInstance inst = makeTinyFleetInstance();
    const FleetPlan plan = makeHandPlan(inst);
    EXPECT_EQ(maxViolation(checkFleetPlan(inst, plan)), 0.0);
    EXPECT_DOUBLE_EQ(vehicleMiles(inst, plan, 0), 100.0 + 120.0);
    // ((60 - 5)/60)^2 * 2.
    EXPECT_DOUBLE_EQ(fleetShortfallObjective(inst, plan),
                     2.0 * (55.0 / 60.0) * (55.0 / 60.0));
}

// Perturb the feasible hand plan one family at a time and check the exact
// violation magnitude of the TARGETED family (a perturbation may also nudge
// families upstream of it, e.g. balance).
TEST(NetworkFleetPlan, ViolationFamiliesExact) {
    const FleetInstance inst = makeTinyFleetInstance();

    {
        FleetPlan plan = makeHandPlan(inst);
        plan.supplied(0, 0) = 6.0;         // injects 6, ships 5
        EXPECT_DOUBLE_EQ(checkFleetPlan(inst, plan).assetBalance, 1.0);
    }
    {
        FleetPlan plan = makeHandPlan(inst);
        plan.resupply(1, 0) = 6.0;         // claims 6 delivered, 5 arrived
        EXPECT_DOUBLE_EQ(checkFleetPlan(inst, plan).delivery, 1.0);
    }
    {
        FleetPlan plan = makeHandPlan(inst);
        plan.supplied(0, 0) = 101.0;       // cap is 100
        EXPECT_DOUBLE_EQ(checkFleetPlan(inst, plan).capacity, 1.0);
    }
    {
        FleetPlan plan = makeHandPlan(inst);
        plan.flow[0](1, 0) = -2.0;
        EXPECT_DOUBLE_EQ(checkFleetPlan(inst, plan).negativity, 2.0);
    }
    {
        FleetPlan plan = makeHandPlan(inst);
        plan.resupply(0, 0) = 3.0;         // node 0 has no demand
        EXPECT_DOUBLE_EQ(checkFleetPlan(inst, plan).idleResupply, 3.0);
    }
    {
        FleetPlan plan = makeHandPlan(inst);
        plan.flow[0](0, 1) = 6.0;          // 12 tons on a 10-ton allocation
        EXPECT_DOUBLE_EQ(checkFleetPlan(inst, plan).linkWeight, 2.0);
        EXPECT_EQ(checkFleetPlan(inst, plan).linkArea, 0.0);   // 24 <= 100
    }
    {
        FleetPlan plan = makeHandPlan(inst);
        plan.vehicles[0](0, 1) = 0.0;      // cargo with no vehicles at all
        EXPECT_DOUBLE_EQ(checkFleetPlan(inst, plan).linkWeight, 10.0);
        EXPECT_DOUBLE_EQ(checkFleetPlan(inst, plan).linkArea, 20.0);
    }
    {
        FleetPlan plan = makeHandPlan(inst);
        plan.vehicles[0](1, 0) = 0.0;      // outbound truck never comes back
        EXPECT_DOUBLE_EQ(checkFleetPlan(inst, plan).vehicleBalance, 1.0);
    }
    {
        FleetPlan plan = makeHandPlan(inst);
        plan.vehicles[0](0, 1) = 30.0;     // 30 round trips: 6600 miles
        plan.vehicles[0](1, 0) = 30.0;     // vs budget 4800
        EXPECT_DOUBLE_EQ(checkFleetPlan(inst, plan).budget, 1800.0);
        EXPECT_EQ(checkFleetPlan(inst, plan).vehicleBalance, 0.0);
    }
}

// Out-and-back and diagonal-only vehicle moves circulate EXACTLY (Lemma FL2):
// the same stored values enter the row sum and the column sum.
TEST(NetworkFleetPlan, OutAndBackCirculatesExactly) {
    const FleetInstance inst = makeTinyFleetInstance();
    FleetPlan plan = makeZeroFleetPlan(inst);
    plan.vehicles[0](0, 1) = 0.371;
    plan.vehicles[0](1, 0) = 0.371;
    plan.vehicles[0](0, 0) = 2.25;        // diagonal closes its own loop
    EXPECT_EQ(checkFleetPlan(inst, plan).vehicleBalance, 0.0);
}

TEST(NetworkFleetPlan, RejectsMismatchedShapes) {
    const FleetInstance inst = makeTinyFleetInstance();
    FleetPlan plan = makeZeroFleetPlan(inst);
    plan.flow.clear();
    EXPECT_THROW(checkFleetPlan(inst, plan), std::invalid_argument);
}
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------

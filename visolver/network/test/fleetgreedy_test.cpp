// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
// Tests for the greedy fleet planner: per-asset rationing, exact small
// cases, vehicle-type mixing, starvation, and the random-profile smoke test.
// ----------------------------------------------
#include "fleetgreedy.hpp"

#include <gtest/gtest.h>

#include <algorithm>
#include <cstdint>

using namespace VINCP;
using namespace VINCP::Network;

namespace {

    const std::uint64_t kSeed = 20260703;
    const double kTol = 1.0e-9;
    const double kFeasTol = 1.0e-9;

    // Node 0 supplies, node 2 demands; node 1 is idle. Distances chosen so
    // node 0 is the obvious source; asset/vehicle lists are each test's knob.
    FleetInstance makeThreeNodeSkeleton() {
        FleetInstance inst;
        inst.numNodes = 3;
        inst.distance = MatrixXd(3, 3);
        inst.distance << 2.0,   300.0, 100.0,
                         300.0,   2.0, 200.0,
                         110.0, 210.0,   2.0;
        inst.horizonHours = 1.0;
        return inst;
    }

} // namespace

// Rationing decomposes per asset (Lemma FL1): a slack asset keeps its full
// demand while a scarce asset is rationed EXACTLY as the single-commodity
// water-fill of that column.
TEST(NetworkFleetGreedy, RationingPerAssetIndependent) {
    FleetInstance inst = makeThreeNodeSkeleton();
    inst.assets = {{"a0", 1.0, 1.0}, {"a1", 1.0, 1.0}};
    inst.vehicles = {{"truck", 10.0, 10.0, 100.0, 50.0}};
    inst.supplyCap = MatrixXd(3, 2);
    inst.supplyCap << 100.0, 100.0,
                        0.0,   0.0,
                        0.0,   0.0;
    inst.demand = MatrixXd(3, 2);
    inst.demand <<  0.0,   0.0,
                   60.0, 100.0,
                   30.0, 100.0;
    inst.priority = MatrixXd(3, 2);
    inst.priority << 1.0, 1.0,
                     1.0, 1.0,
                     2.0, 1.0;
    validateFleetInstance(inst);

    const MatrixXd targets = rationFleetTargets(inst);

    // Asset 0 is slack (90 <= 100): targets are the demands.
    EXPECT_EQ(targets(1, 0), 60.0);
    EXPECT_EQ(targets(2, 0), 30.0);

    // Asset 1 is scarce (200 > 100): symmetric split, and identical to the
    // single-commodity water-fill of the column -- the separability claim
    // made executable.
    EXPECT_NEAR(targets(1, 1), 50.0, kTol);
    EXPECT_NEAR(targets(2, 1), 50.0, kTol);
    const VectorXd column = waterFillTargets(inst.demand.col(1),
                                             inst.priority.col(1), 100.0);
    EXPECT_EQ((targets.col(1) - column).cwiseAbs().maxCoeff(), 0.0);
}

// One asset, one vehicle type, ample budget: exact flow, weight-bound
// vehicle count, deadhead symmetry, exact mileage, and a scale hint <= 1.
TEST(NetworkFleetGreedy, GreedyExactSmallCase) {
    const double kDemand = 5.0;
    FleetInstance inst = makeThreeNodeSkeleton();
    inst.assets = {{"crate", 2.0, 4.0}};              // 2 tons, 4 sqft per unit
    inst.vehicles = {{"truck", 10.0, 100.0, 100.0, 50.0}};   // kappa = 10/2 = 5
    inst.supplyCap = MatrixXd::Zero(3, 1);
    inst.supplyCap(0, 0) = 100.0;
    inst.demand = MatrixXd::Zero(3, 1);
    inst.demand(2, 0) = kDemand;
    inst.priority = MatrixXd::Ones(3, 1);
    validateFleetInstance(inst);

    const FleetGreedyParams params;
    const FleetGreedyResult result = greedyFleetPlan(inst, params);

    const double kServed = (1.0 - params.demandScaleDown) * kDemand;
    const double kKappa = 5.0;                        // weight binds: 10/2 < 100/4
    const double kVehicles = kServed / kKappa;
    const double kRoundTrip = inst.distance(0, 2) + inst.distance(2, 0);

    EXPECT_DOUBLE_EQ(result.plan.flow[0](0, 2), kServed);
    EXPECT_DOUBLE_EQ(result.plan.vehicles[0](0, 2), kVehicles);
    EXPECT_EQ(result.plan.vehicles[0](0, 2), result.plan.vehicles[0](2, 0));
    EXPECT_DOUBLE_EQ(result.milesUsed(0), kVehicles * kRoundTrip);
    EXPECT_DOUBLE_EQ(result.plan.resupply(2, 0), kServed);

    const FleetPlanViolations violations = checkFleetPlan(inst, result.plan);
    EXPECT_EQ(violations.vehicleBalance, 0.0);        // FL2: exact
    EXPECT_LE(maxViolation(violations), kFeasTol);
    EXPECT_LE(result.fleetScaleHint, 1.0);            // ample budget sufficed
    EXPECT_EQ(result.unserved.maxCoeff(), 0.0);
}

// Two assets against one vehicle type: the dense asset is weight-bound and
// the bulky asset area-bound, each with its own exact vehicle count.
TEST(NetworkFleetGreedy, AreaBindsVsWeightBinds) {
    const double kDemand = 5.0;
    FleetInstance inst = makeThreeNodeSkeleton();
    inst.assets = {{"dense", 2.0, 4.0},     // kappa = min(10/2, 100/4)  = 5
                   {"bulky", 0.1, 50.0}};   // kappa = min(10/0.1, 100/50) = 2
    inst.vehicles = {{"truck", 10.0, 100.0, 100.0, 50.0}};
    inst.supplyCap = MatrixXd::Zero(3, 2);
    inst.supplyCap.row(0) << 100.0, 100.0;
    inst.demand = MatrixXd::Zero(3, 2);
    inst.demand.row(2) << kDemand, kDemand;
    inst.priority = MatrixXd::Ones(3, 2);
    validateFleetInstance(inst);

    const FleetGreedyParams params;
    const FleetGreedyResult result = greedyFleetPlan(inst, params);

    const double kServed = (1.0 - params.demandScaleDown) * kDemand;
    EXPECT_DOUBLE_EQ(result.plan.flow[0](0, 2), kServed);
    EXPECT_DOUBLE_EQ(result.plan.flow[1](0, 2), kServed);
    // Vehicles accumulate: weight-bound share then area-bound share (the
    // dense cell is served first: equal scores, row-major tie-break).
    EXPECT_DOUBLE_EQ(result.plan.vehicles[0](0, 2),
                     kServed / 5.0 + kServed / 2.0);
    EXPECT_LE(maxViolation(checkFleetPlan(inst, result.plan)), kFeasTol);
}

// Two vehicle types, the better one budget-starved: exact split across
// types, and the drained type's utilization is EXACTLY 1 (the
// assign-not-subtract discipline).
TEST(NetworkFleetGreedy, MixesVehicleTypes) {
    const double kDemand = 10.0;
    FleetInstance inst;
    inst.numNodes = 2;
    inst.assets = {{"box", 1.0, 1.0}};
    // Type 0: kappa = 10, budget 1 * 64 * 1 = 64 vehicle-miles (starved).
    // Type 1: kappa = 2, budget 100 * 100 * 1 = 10000 (ample).
    inst.vehicles = {{"big", 10.0, 10.0, 1.0, 64.0},
                     {"small", 2.0, 2.0, 100.0, 100.0}};
    inst.supplyCap = MatrixXd(2, 1);
    inst.supplyCap << 100.0, 0.0;
    inst.demand = MatrixXd(2, 1);
    inst.demand << 0.0, kDemand;
    inst.priority = MatrixXd::Ones(2, 1);
    inst.distance = MatrixXd(2, 2);
    inst.distance << 1.0, 128.0,
                     128.0, 1.0;             // round trip 256: exact binary
    inst.horizonHours = 1.0;
    validateFleetInstance(inst);

    const FleetGreedyParams params;
    const FleetGreedyResult result = greedyFleetPlan(inst, params);

    // Type 0 moves (64/256) * 10 = 2.5 units on 0.25 vehicles, exactly
    // draining its budget; type 1 carries the remainder.
    const double kServed = (1.0 - params.demandScaleDown) * kDemand;
    const double kBigUnits = 2.5;
    EXPECT_EQ(result.plan.vehicles[0](0, 1), 0.25);
    EXPECT_EQ(result.milesUsed(0), 64.0);
    EXPECT_EQ(result.utilization(0), 1.0);              // drained EXACTLY
    EXPECT_DOUBLE_EQ(result.plan.vehicles[1](0, 1), (kServed - kBigUnits) / 2.0);
    EXPECT_DOUBLE_EQ(result.plan.flow[0](0, 1), kServed);
    EXPECT_DOUBLE_EQ(result.plan.resupply(1, 0), kServed);
    EXPECT_LE(maxViolation(checkFleetPlan(inst, result.plan)), kFeasTol);
    EXPECT_GT(result.fleetScaleHint, 1.0);              // big type alone: short
}

// Starved fleet: the loop gives up gracefully (no throw), the plan stays
// feasible, the shortfall is honest, and the scale hint says how much more
// fleet this heuristic wanted.
TEST(NetworkFleetGreedy, TransportStarvedGraceful) {
    FleetInstance inst;
    inst.numNodes = 3;
    inst.assets = {{"box", 1.0, 1.0}};
    // kappa = 10; budget 100 vehicle-miles; round trips cost 200: at most
    // (100/200) * 10 = 5 units can ever move.
    inst.vehicles = {{"truck", 10.0, 10.0, 1.0, 100.0}};
    inst.supplyCap = MatrixXd(3, 1);
    inst.supplyCap << 100.0, 0.0, 0.0;
    inst.demand = MatrixXd(3, 1);
    inst.demand << 0.0, 10.0, 10.0;
    inst.priority = MatrixXd::Ones(3, 1);
    inst.distance = MatrixXd(3, 3);
    inst.distance << 1.0, 100.0, 100.0,
                     100.0, 1.0, 100.0,
                     100.0, 100.0, 1.0;
    inst.horizonHours = 1.0;
    validateFleetInstance(inst);

    const FleetGreedyResult result = greedyFleetPlan(inst);

    EXPECT_EQ(result.utilization(0), 1.0);              // everything spent
    EXPECT_GT(result.unserved.maxCoeff(), 0.0);         // and it wasn't enough
    EXPECT_GT(result.fleetScaleHint, 1.0);
    EXPECT_LE(result.iterations,
              static_cast<int>(3 * inst.numNodes * numAssets(inst)
                               + numVehicleTypes(inst) + 2));
    EXPECT_LE(maxViolation(checkFleetPlan(inst, result.plan)), kFeasTol);
    // Only 5 of the ~20 rationed units moved.
    EXPECT_DOUBLE_EQ(result.plan.resupply.sum(), 5.0);
}

// The full default random profile: feasible within accumulation tolerances,
// budgets respected, utilizations sane, and the shortfall ordering
// theta_plan >= theta_ration (the sandwich analog) holds.
TEST(NetworkFleetGreedy, RandomFleetProfileFeasible) {
    const FleetProfile profile;
    const FleetInstance inst = makeRandomFleetInstance(profile, kSeed);
    const FleetGreedyResult result = greedyFleetPlan(inst);

    double totalUnits = 0.0;
    for (Index a = 0; a < numAssets(inst); ++a) {
        totalUnits += totalFleetDemand(inst, a);
    }
    const double kAccumTol = 1.0e-9 * totalUnits;

    const FleetPlanViolations violations = checkFleetPlan(inst, result.plan);
    EXPECT_LE(violations.assetBalance, kAccumTol);
    EXPECT_LE(violations.delivery, kAccumTol);
    EXPECT_LE(violations.capacity, kAccumTol);
    EXPECT_EQ(violations.negativity, 0.0);
    EXPECT_EQ(violations.idleResupply, 0.0);
    EXPECT_LE(violations.linkWeight, kAccumTol);
    EXPECT_LE(violations.linkArea, kAccumTol);
    EXPECT_EQ(violations.vehicleBalance, 0.0);          // FL2: exact
    // Budget residuals scale with the budgets, not with tonnage.
    EXPECT_LE(violations.budget, 1.0e-9 * result.budget.maxCoeff());

    EXPECT_GT(result.iterations, 0);
    for (Index k = 0; k < numVehicleTypes(inst); ++k) {
        EXPECT_GE(result.utilization(k), 0.0);
        EXPECT_LE(result.utilization(k), 1.0 + 1.0e-9);
        EXPECT_DOUBLE_EQ(result.budget(k), vehicleBudget(inst, k));
    }
    EXPECT_GE(result.unserved.minCoeff(), 0.0);
    EXPECT_GT(result.fleetScaleHint, 0.0);

    // Shortfall ordering: the plan cannot beat the transport-unconstrained
    // rationing bound.
    const double thetaRation = fleetShortfallOfResupply(inst, result.targets);
    EXPECT_GE(result.shortfallValue, thetaRation);

    // Targets never exceed what is meetable, per asset.
    for (Index a = 0; a < numAssets(inst); ++a) {
        const double meetable = std::min(totalFleetDemand(inst, a),
                                         totalFleetSupplyCap(inst, a));
        EXPECT_LE(result.targets.col(a).sum(), meetable + kAccumTol);
    }
}
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------

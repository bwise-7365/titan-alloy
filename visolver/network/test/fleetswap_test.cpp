// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
// Tests for the fleet swap improvement: per-asset 2-exchange local optima
// with vehicle reallocation.
// ----------------------------------------------
#include "fleetgreedy.hpp"
#include "fleetswap.hpp"

#include <gtest/gtest.h>

#include <cstdint>

using namespace VIMCP;
using namespace VIMCP::Network;

namespace {

  const std::uint64_t kSeed = 20260703;
  const double kFeasTol = 1.0e-9;

  // Two sources (0, 1), two sinks (2, 3); the crossed pairing 0->2, 1->3 is
  // ten times longer than the uncrossed 0->3, 1->2. One asset (1 t + 1 sqft
  // per unit), one vehicle type (kappa = 10), generous budget.
  FleetInstance makeCrossedFleetInstance(Index numAssetTypes) {
    FleetInstance inst;
    inst.numNodes = 4;
    for (Index a = 0; a < numAssetTypes; ++a) {
      inst.assets.push_back({"box" + std::to_string(a), 1.0, 1.0});
    }
    inst.vehicles = {{"truck", 10.0, 10.0, 1.0, 100.0}};   // budget 100 v-mi
    inst.supplyCap = MatrixXd::Zero(4, numAssetTypes);
    inst.supplyCap.row(0).setConstant(10.0);
    inst.supplyCap.row(1).setConstant(10.0);
    inst.demand = MatrixXd::Zero(4, numAssetTypes);
    inst.demand.row(2).setConstant(5.0);
    inst.demand.row(3).setConstant(5.0);
    inst.priority = MatrixXd::Ones(4, numAssetTypes);
    inst.distance = MatrixXd::Constant(4, 4, 50.0);
    inst.distance.diagonal().setConstant(2.0);
    inst.distance(0, 2) = 10.0;
    inst.distance(2, 0) = 10.0;
    inst.distance(1, 3) = 10.0;
    inst.distance(3, 1) = 10.0;
    inst.distance(0, 3) = 1.0;
    inst.distance(3, 0) = 1.0;
    inst.distance(1, 2) = 1.0;
    inst.distance(2, 1) = 1.0;
    inst.horizonHours = 1.0;
    validateFleetInstance(inst);
    return inst;
  }

  // The crossed plan for every asset: 5 units on (0,2) and (1,3), carried
  // out-and-back on 0.5 trucks each way per asset.
  FleetPlan makeCrossedFleetPlan(const FleetInstance& inst) {
    FleetPlan plan = makeZeroFleetPlan(inst);
    for (Index a = 0; a < numAssets(inst); ++a) {
      plan.flow[static_cast<size_t>(a)](0, 2) = 5.0;
      plan.flow[static_cast<size_t>(a)](1, 3) = 5.0;
      plan.supplied(0, a) = 5.0;
      plan.supplied(1, a) = 5.0;
      plan.resupply(2, a) = 5.0;
      plan.resupply(3, a) = 5.0;
      plan.vehicles[0](0, 2) += 0.5;
      plan.vehicles[0](2, 0) += 0.5;
      plan.vehicles[0](1, 3) += 0.5;
      plan.vehicles[0](3, 1) += 0.5;
    }
    return plan;
  }

} // namespace

// One asset: the crossed assignment uncrosses in one swap (round-trip saving
// (20 + 20) - (2 + 2) = 36 per unit x 5 units), the vehicles are rebuilt on
// the short links, and every constraint family stays exactly zero.
TEST(NetworkFleetSwap, UncrossesSingleAssetExactly) {
  const FleetInstance inst = makeCrossedFleetInstance(1);
  FleetPlan plan = makeCrossedFleetPlan(inst);
  ASSERT_EQ(maxViolation(checkFleetPlan(inst, plan)), 0.0);

  const MatrixXd resupplyBefore = plan.resupply;
  const FleetSwapSummary summary = swapFleetToLocalOptimum(inst, plan);

  EXPECT_EQ(summary.totalSwaps, 1);
  EXPECT_EQ(summary.swapsPerAsset[0], 1);
  EXPECT_DOUBLE_EQ(summary.unitMileSaving, 5.0 * 36.0);
  EXPECT_DOUBLE_EQ(summary.milesUsedBefore(0), 20.0);   // 0.5 x 20 twice
  EXPECT_DOUBLE_EQ(summary.milesUsedAfter(0), 2.0);     // 0.5 x 2 twice

  EXPECT_EQ(plan.flow[0](0, 2), 0.0);
  EXPECT_EQ(plan.flow[0](1, 3), 0.0);
  EXPECT_DOUBLE_EQ(plan.flow[0](0, 3), 5.0);
  EXPECT_DOUBLE_EQ(plan.flow[0](1, 2), 5.0);
  EXPECT_DOUBLE_EQ(plan.vehicles[0](0, 3), 0.5);
  EXPECT_EQ(plan.vehicles[0](0, 3), plan.vehicles[0](3, 0));
  EXPECT_DOUBLE_EQ(plan.vehicles[0](1, 2), 0.5);
  EXPECT_EQ(plan.vehicles[0](1, 2), plan.vehicles[0](2, 1));

  EXPECT_EQ((plan.resupply - resupplyBefore).cwiseAbs().maxCoeff(), 0.0);
  EXPECT_EQ(maxViolation(checkFleetPlan(inst, plan)), 0.0);
}

// Two assets, both crossed: one swap per asset class, applied in turn.
TEST(NetworkFleetSwap, SwapsEachAssetInTurn) {
  const FleetInstance inst = makeCrossedFleetInstance(2);
  FleetPlan plan = makeCrossedFleetPlan(inst);
  ASSERT_EQ(maxViolation(checkFleetPlan(inst, plan)), 0.0);

  const FleetSwapSummary summary = swapFleetToLocalOptimum(inst, plan);

  EXPECT_EQ(summary.totalSwaps, 2);
  EXPECT_EQ(summary.swapsPerAsset[0], 1);
  EXPECT_EQ(summary.swapsPerAsset[1], 1);
  EXPECT_DOUBLE_EQ(summary.milesUsedAfter(0), 4.0);   // both assets rebuilt
  EXPECT_EQ(maxViolation(checkFleetPlan(inst, plan)), 0.0);
}

// An already-uncrossed plan is a local optimum: no swaps, and the vehicle
// reallocation reproduces the same mileage.
TEST(NetworkFleetSwap, LocalOptimumIsFixedPoint) {
  const FleetInstance inst = makeCrossedFleetInstance(1);
  FleetPlan plan = makeCrossedFleetPlan(inst);
  ASSERT_EQ(swapFleetToLocalOptimum(inst, plan).totalSwaps, 1);

  const FleetSwapSummary again = swapFleetToLocalOptimum(inst, plan);
  EXPECT_EQ(again.totalSwaps, 0);
  EXPECT_DOUBLE_EQ(again.milesUsedBefore(0), again.milesUsedAfter(0));
}

// Purification on a fleet plan with TIED routing: the 4-arc spread
// consolidates to 2 arcs at zero saving, deliveries bitwise unchanged,
// vehicles rebuilt, everything exactly feasible. (The distances are
// symmetric here, so the round-trip tie mirrors the single-commodity
// tied-routing case.)
TEST(NetworkFleetSwap, PurifyFleetConsolidatesTiedRouting) {
  FleetInstance inst;
  inst.numNodes = 4;
  inst.assets = {{"box", 1.0, 1.0}};
  inst.vehicles = {{"truck", 10.0, 10.0, 10.0, 100.0}};   // budget 1000
  inst.horizonHours = 1.0;
  inst.supplyCap = MatrixXd::Zero(4, 1);
  inst.supplyCap(0, 0) = 10.0;
  inst.supplyCap(1, 0) = 10.0;
  inst.demand = MatrixXd::Zero(4, 1);
  inst.demand(2, 0) = 5.0;
  inst.demand(3, 0) = 5.0;
  inst.priority = MatrixXd::Ones(4, 1);
  // Symmetric distances with TIED pairings: d(0,2)+d(1,3) = 2+4 = 6 equals
  // d(0,3)+d(1,2) = 3+3; every mix of the two matchings costs the same.
  inst.distance = MatrixXd::Constant(4, 4, 50.0);
  inst.distance.diagonal().setConstant(1.0);
  inst.distance(0, 2) = 2.0;  inst.distance(2, 0) = 2.0;
  inst.distance(1, 3) = 4.0;  inst.distance(3, 1) = 4.0;
  inst.distance(0, 3) = 3.0;  inst.distance(3, 0) = 3.0;
  inst.distance(1, 2) = 3.0;  inst.distance(2, 1) = 3.0;
  validateFleetInstance(inst);

  FleetPlan plan = makeZeroFleetPlan(inst);
  for (const auto& arc : {std::pair<Index, Index>{0, 2}, {0, 3}, {1, 2},
                          {1, 3}}) {
    plan.flow[0](arc.first, arc.second) = 2.5;               // the spread
    plan.vehicles[0](arc.first, arc.second) += 0.25;         // 2.5 / kappa 10
    plan.vehicles[0](arc.second, arc.first) += 0.25;
  }
  plan.supplied.col(0) = plan.flow[0].rowwise().sum();
  plan.resupply.col(0) = plan.flow[0].colwise().sum().transpose();
  ASSERT_EQ(maxViolation(checkFleetPlan(inst, plan)), 0.0);

  const MatrixXd resupplyBefore = plan.resupply;
  const FleetPurifySummary summary = purifyFleetPlan(inst, plan);

  EXPECT_EQ(summary.consolidatingSwaps, 1);
  EXPECT_EQ(summary.arcsBeforePerAsset[0], 4);
  EXPECT_EQ(summary.arcsAfterPerAsset[0], 2);
  EXPECT_EQ((plan.resupply - resupplyBefore).cwiseAbs().maxCoeff(), 0.0);
  EXPECT_NEAR(summary.milesUsedAfter(0), summary.milesUsedBefore(0), 1.0e-9);
  EXPECT_LE(maxViolation(checkFleetPlan(inst, plan)), 1.0e-9);
}

// The full random profile: swap the greedy fleet plan. Deliveries and the
// objective are bitwise unchanged, the plan stays feasible, budgets hold,
// and total vehicle-miles do not increase.
TEST(NetworkFleetSwap, ImprovesGreedyPlanOnRandomInstance) {
  const FleetProfile profile;
  const FleetInstance inst = makeRandomFleetInstance(profile, kSeed);
  FleetGreedyResult greedy = greedyFleetPlan(inst);
  const MatrixXd resupplyBefore = greedy.plan.resupply;
  const double thetaBefore = fleetShortfallObjective(inst, greedy.plan);

  const FleetSwapSummary summary =
      swapFleetToLocalOptimum(inst, greedy.plan);

  EXPECT_GT(summary.totalSwaps, 0);
  EXPECT_GT(summary.unitMileSaving, 0.0);
  EXPECT_LE(summary.milesUsedAfter.sum(),
            summary.milesUsedBefore.sum() + kFeasTol);
  EXPECT_EQ((greedy.plan.resupply - resupplyBefore).cwiseAbs().maxCoeff(),
            0.0);
  EXPECT_EQ(fleetShortfallObjective(inst, greedy.plan), thetaBefore);

  const FleetPlanViolations violations = checkFleetPlan(inst, greedy.plan);
  EXPECT_EQ(violations.vehicleBalance, 0.0);            // FL2: exact
  EXPECT_LE(violations.budget, 1.0e-9 * vehicleBudget(inst, 0));
  double totalUnits = 0.0;
  for (Index a = 0; a < numAssets(inst); ++a) {
    totalUnits += totalFleetDemand(inst, a);
  }
  const double kAccumTol = 1.0e-9 * totalUnits;
  EXPECT_LE(violations.assetBalance, kAccumTol);
  EXPECT_LE(violations.delivery, kAccumTol);
  EXPECT_LE(violations.linkWeight, kAccumTol);
  EXPECT_LE(violations.linkArea, kAccumTol);
  EXPECT_EQ(violations.negativity, 0.0);
  EXPECT_EQ(violations.idleResupply, 0.0);
}
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------

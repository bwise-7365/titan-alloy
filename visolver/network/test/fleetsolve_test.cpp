// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
// Tests for the fleet optimizer driver. The load-bearing check is the
// A=1/K=1 equivalence with the oracle-validated single-commodity pipeline.
// ----------------------------------------------
#include "fleetsolve.hpp"

#include "fleetgreedy.hpp"
#include "flowplan.hpp"

#include <gtest/gtest.h>

#include <cstdint>
#include <stdexcept>

using namespace VINCP;
using namespace VINCP::Network;

namespace {

  const std::uint64_t kSeed = 20260703;

  // Symmetric, metric line geometry: d(i, j) = 10 |i - j|, diagonal 1. No
  // strict multi-hop shortcut exists, so d-hat = d and the fleet round trip
  // is exactly 2 d. Sources 0, 1; sinks 2, 3.
  MatrixXd makeLineDistances() {
    MatrixXd d(4, 4);
    for (Index i = 0; i < 4; ++i) {
      for (Index j = 0; j < 4; ++j) {
        d(i, j) = (i == j) ? 1.0 : 10.0 * std::abs(static_cast<double>(i - j));
      }
    }
    return d;
  }

  // One asset (1 ton, no area), one vehicle type with kappa = 2: the fleet
  // per-unit round-trip mileage is 2 d / kappa = d, so the equivalent
  // single-commodity instance has cost = d and budget L = B.
  FleetInstance makeEquivalenceFleetInstance(double budgetMiles) {
    FleetInstance inst;
    inst.numNodes = 4;
    inst.assets = {{"box", 1.0, 0.0}};
    inst.vehicles = {{"cart", 2.0, 0.0, budgetMiles, 1.0}};   // B = budget
    inst.horizonHours = 1.0;
    inst.supplyCap = MatrixXd::Zero(4, 1);
    inst.supplyCap(0, 0) = 10.0;
    inst.supplyCap(1, 0) = 10.0;
    inst.demand = MatrixXd::Zero(4, 1);
    inst.demand(2, 0) = 8.0;
    inst.demand(3, 0) = 6.0;
    inst.priority = MatrixXd::Ones(4, 1);
    inst.priority(2, 0) = 2.0;
    inst.priority(3, 0) = 3.0;
    inst.distance = makeLineDistances();
    validateFleetInstance(inst);
    return inst;
  }

  Instance makeEquivalenceFlowInstance(double budgetMiles) {
    Instance inst;
    inst.numNodes = 4;
    inst.supplyCap = VectorXd(4);
    inst.supplyCap << 10.0, 10.0, 0.0, 0.0;
    inst.demand = VectorXd(4);
    inst.demand << 0.0, 0.0, 8.0, 6.0;
    inst.priority = VectorXd(4);
    inst.priority << 1.0, 1.0, 2.0, 3.0;
    inst.cost = makeLineDistances();
    inst.tonMileLimit = budgetMiles;
    validateInstance(inst);
    return inst;
  }

} // namespace

// With one asset and one vehicle type the fleet LCP is the single-commodity
// LCP under cost = 2 d / kappa: the two oracle-validated pipelines must
// agree on shortfall, deliveries, mileage, and the budget shadow price.
// Exercised at a BINDING budget so the lambda comparison is nontrivial.
TEST(NetworkFleetSolve, MatchesFlowPipelineWhenSingleAssetSingleType) {
  const double kBudget = 150.0;   // cheapest full service needs ~200 miles
  const FleetInstance fleet = makeEquivalenceFleetInstance(kBudget);
  const Instance flow = makeEquivalenceFlowInstance(kBudget);

  FleetSolveParams fleetParams;
  fleetParams.maxSourcesPerSink = 0;              // keep-all on both sides
  const FleetSolveResult fleetResult = solveFleetPlan(fleet, fleetParams);

  FlowPlanParams flowParams;
  flowParams.engine = "ipm";
  const FlowPlanResult flowResult = solveFlowPlan(flow, flowParams);

  ASSERT_TRUE(fleetResult.vi.converged);
  ASSERT_TRUE(flowResult.vi.converged);
  EXPECT_TRUE(fleetResult.certifiedP);
  EXPECT_TRUE(flowResult.certifiedP);

  EXPECT_NEAR(fleetResult.shortfall, flowResult.shortfall, 1.0e-6);
  for (Index i = 0; i < 4; ++i) {
    EXPECT_NEAR(fleetResult.plan.resupply(i, 0), flowResult.plan.resupply(i),
                1.0e-5);
  }
  EXPECT_NEAR(fleetResult.milesUsed(0), flowResult.tonMilesUsed, 1.0e-4);
  EXPECT_NEAR(fleetResult.budgetShadowPrice(0), flowResult.budgetShadowPrice,
              1.0e-6);
  EXPECT_LE(fleetResult.milesUsed(0), kBudget * (1.0 + 1.0e-9));
  EXPECT_LE(maxViolation(checkFleetPlan(fleet, fleetResult.plan)), 1.0e-6);
}

// The certificate loop recovers a screened-out source that the optimum
// needs: the near source is capacity-starved, the far one was excluded by
// the k = 1 screen.
TEST(NetworkFleetSolve, ScreenCertificateRecoversExcludedSource) {
  FleetInstance inst;
  inst.numNodes = 3;
  inst.assets = {{"box", 1.0, 0.0}};
  inst.vehicles = {{"cart", 1.0, 0.0, 10000.0, 1.0}};   // ample budget
  inst.horizonHours = 1.0;
  inst.supplyCap = MatrixXd::Zero(3, 1);
  inst.supplyCap(0, 0) = 2.0;                    // near but starved
  inst.supplyCap(1, 0) = 10.0;                   // far but ample
  inst.demand = MatrixXd::Zero(3, 1);
  inst.demand(2, 0) = 8.0;
  inst.priority = MatrixXd::Ones(3, 1);
  inst.distance = MatrixXd(3, 3);
  inst.distance << 1.0,  40.0, 10.0,
                   40.0,  1.0, 50.0,
                   10.0, 50.0,  1.0;
  validateFleetInstance(inst);

  FleetSolveParams params;
  params.maxSourcesPerSink = 1;                  // screens out source 1
  const FleetSolveResult result = solveFleetPlan(inst, params);

  ASSERT_TRUE(result.vi.converged);
  EXPECT_TRUE(result.certifiedP);
  EXPECT_GE(result.certificateRounds, 1);        // the far source was added
  EXPECT_EQ(result.keptPairs, result.totalPairs);
  EXPECT_NEAR(result.plan.resupply(2, 0), 8.0, 1.0e-3);
}

// Small random profile: the optimizer sits inside the validation sandwich,
// respects every budget, and the plan is feasible.
TEST(NetworkFleetSolve, SandwichAndFeasibilityOnRandomProfile) {
  FleetProfile profile;
  profile.geometry.numSupplyOnly = 8;
  profile.geometry.numBoth = 6;
  profile.geometry.numDemandOnly = 10;
  const FleetInstance inst = makeRandomFleetInstance(profile, kSeed);

  const FleetSolveResult result = solveFleetPlan(inst);
  ASSERT_TRUE(result.vi.converged);
  EXPECT_TRUE(result.certifiedP);

  // Feasibility of the unpacked plan (accumulation tolerances; circulation
  // cancels only to rounding on multi-hop loops).
  EXPECT_LE(maxViolation(checkFleetPlan(inst, result.plan)), 1.0e-3);
  for (Index k = 0; k < numVehicleTypes(inst); ++k) {
    EXPECT_LE(result.milesUsed(k),
              vehicleBudget(inst, k) * (1.0 + 1.0e-6));
    EXPECT_GE(result.budgetShadowPrice(k), 0.0);
  }

  // Sandwich: theta_ration <= theta* <= theta_greedy.
  const MatrixXd targets = rationFleetTargets(inst);
  const double thetaRation = fleetShortfallOfResupply(inst, targets);
  const FleetGreedyResult greedy = greedyFleetPlan(inst);
  EXPECT_LE(thetaRation, result.shortfall + 1.0e-6);
  EXPECT_LE(result.shortfall, greedy.shortfallValue + 1.0e-6);
}

TEST(NetworkFleetSolve, RejectsBadInputs) {
  const FleetInstance inst = makeEquivalenceFleetInstance(100.0);
  FleetSolveParams params;
  params.engine = "simplex";
  EXPECT_THROW(solveFleetPlan(inst, params), std::invalid_argument);
}

// The observability hooks (FP0, 2026-07-08 performance plan) fire in matched
// start/end pairs, one pair per certificate round, with consistent round
// numbers and growing kept sets on the certificate-recovery instance; the
// engine heartbeat logger fires at iterFreq 1; and the solution is bitwise
// identical to a hook-free run.
TEST(NetworkFleetSolve, RoundHooksObserveWithoutChangingTheSolve) {
  FleetInstance inst;
  inst.numNodes = 3;
  inst.assets = {{"box", 1.0, 0.0}};
  inst.vehicles = {{"cart", 1.0, 0.0, 10000.0, 1.0}};
  inst.horizonHours = 1.0;
  inst.supplyCap = MatrixXd::Zero(3, 1);
  inst.supplyCap(0, 0) = 2.0;
  inst.supplyCap(1, 0) = 10.0;
  inst.demand = MatrixXd::Zero(3, 1);
  inst.demand(2, 0) = 8.0;
  inst.priority = MatrixXd::Ones(3, 1);
  inst.distance = MatrixXd(3, 3);
  inst.distance << 1.0,  40.0, 10.0,
                   40.0,  1.0, 50.0,
                   10.0, 50.0,  1.0;
  validateFleetInstance(inst);

  FleetSolveParams plainParams;
  plainParams.maxSourcesPerSink = 1;             // forces >= 1 recovery round
  const FleetSolveResult plain = solveFleetPlan(inst, plainParams);

  vector<int> startRounds;
  vector<Index> startKept;
  vector<Index> startDims;
  vector<int> endRounds;
  int heartbeats = 0;

  FleetSolveParams hookedParams = plainParams;
  hookedParams.iterFreq = 1;
  hookedParams.logger = [&heartbeats](int, int, double, double) {
    ++heartbeats;
  };
  hookedParams.roundStartLogger = [&](int round, Index kept, Index dim) {
    startRounds.push_back(round);
    startKept.push_back(kept);
    startDims.push_back(dim);
  };
  hookedParams.roundEndLogger = [&endRounds](int round, const VIResult&,
                                             double milliseconds) {
    EXPECT_LE(0.0, milliseconds);
    endRounds.push_back(round);
  };
  const FleetSolveResult hooked = solveFleetPlan(inst, hookedParams);

  // Matched pairs, consecutive round numbers starting at 0, one pair per
  // round actually solved.
  ASSERT_EQ(startRounds, endRounds);
  ASSERT_EQ(static_cast<int>(startRounds.size()),
            hooked.certificateRounds + 1);
  for (size_t i = 0; i < startRounds.size(); ++i) {
    EXPECT_EQ(static_cast<int>(i), startRounds[i]);
    EXPECT_LT(0, startKept[i]);
    EXPECT_LE(startKept[i], startDims[i]);
  }
  // The certificate added a source between round 0 and round 1.
  ASSERT_GE(hooked.certificateRounds, 1);
  EXPECT_LT(startKept.front(), startKept.back());
  EXPECT_LT(0, heartbeats);

  // Hooks observe; they must not perturb the solve.
  ASSERT_TRUE(hooked.vi.converged);
  EXPECT_EQ(plain.vi.iter, hooked.vi.iter);
  ASSERT_EQ(plain.vi.z.size(), hooked.vi.z.size());
  EXPECT_EQ(0.0, (plain.vi.z - hooked.vi.z).cwiseAbs().maxCoeff());
}
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------

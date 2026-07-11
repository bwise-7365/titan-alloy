// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
// Tests for the fleet reduction: shared shortest routes, per-asset
// round-trip pair mileage, screens, and the capability matrix.
// ----------------------------------------------
#include "fleetreduction.hpp"

#include <gtest/gtest.h>

#include <algorithm>
#include <cstdint>
#include <stdexcept>

using namespace VIMCP;
using namespace VIMCP::Network;

namespace {

  const std::uint64_t kSeed = 20260703;
  const double kTol = 1.0e-9;

  // Three nodes with a DOMINATED direct arc: 0 -> 2 direct costs 100 but the
  // two-hop 0 -> 1 -> 2 costs 30 + 20 = 50 (and the reverse likewise). Node 0
  // supplies asset 0; node 2 demands it; node 1 is transit for the asset.
  FleetInstance makeDominatedArcInstance() {
    FleetInstance inst;
    inst.numNodes = 3;
    inst.assets = {{"box", 1.0, 1.0}};
    inst.vehicles = {{"truck", 10.0, 10.0, 4.0, 50.0}};
    inst.supplyCap = MatrixXd::Zero(3, 1);
    inst.supplyCap(0, 0) = 100.0;
    inst.demand = MatrixXd::Zero(3, 1);
    inst.demand(2, 0) = 60.0;
    inst.priority = MatrixXd::Ones(3, 1);
    inst.distance = MatrixXd(3, 3);
    inst.distance << 2.0,  30.0, 100.0,
                     30.0,  2.0,  20.0,
                     100.0, 20.0,  2.0;
    inst.horizonHours = 24.0;
    validateFleetInstance(inst);
    return inst;
  }

} // namespace

// The pair mileage is the ROUND TRIP over one-way shortest routes: the
// dominated direct arc is bypassed in both directions.
TEST(NetworkFleetReduction, RoundTripMileageUsesShortestRoutes) {
  const FleetInstance inst = makeDominatedArcInstance();
  const FleetReducedProblem reduced = makeFleetReducedProblem(inst);

  ASSERT_EQ(reduced.perAsset.size(), 1u);
  const ReducedProblem& asset = reduced.perAsset[0];
  ASSERT_EQ(asset.sources, (std::vector<Index>{0}));
  ASSERT_EQ(asset.sinks, (std::vector<Index>{2}));

  // One-way d-hat(0,2) = 50 via node 1, both directions: rho-hat = 100.
  EXPECT_NEAR(reduced.routes.distance(0, 2), 50.0, kTol);
  EXPECT_NEAR(reduced.routes.distance(2, 0), 50.0, kTol);
  EXPECT_NEAR(asset.shipCost(0, 0), 100.0, kTol);

  // The unpack path walk sees the one-way successors: 0 -> 1 -> 2.
  EXPECT_EQ(routeNodes(reduced.routes, 0, 2), (std::vector<Index>{0, 1, 2}));

  // kappa: min(10/1, 10/1) = 10.
  EXPECT_DOUBLE_EQ(reduced.kappa(0, 0), 10.0);
}

// A self pair (a node that both supplies and demands the asset) is priced by
// the at-least-one-arc self loop, not zero.
TEST(NetworkFleetReduction, SelfPairPricedBySelfLoop) {
  FleetInstance inst = makeDominatedArcInstance();
  inst.demand(0, 0) = 10.0;          // node 0 now demands too
  validateFleetInstance(inst);

  const FleetReducedProblem reduced = makeFleetReducedProblem(inst);
  const ReducedProblem& asset = reduced.perAsset[0];
  ASSERT_EQ(asset.sinks, (std::vector<Index>{0, 2}));
  // Self loop at node 0: the self-arc d(0,0) = 2 beats any round trip.
  EXPECT_NEAR(asset.shipCost(0, 0), 2.0, kTol);
}

// Per-asset count screen keeps each sink's k cheapest sources, ordered by
// ROUND-TRIP mileage; the default profile exercises several assets.
TEST(NetworkFleetReduction, CountScreenKeepsCheapestPrefix) {
  const FleetProfile profile;
  const FleetInstance inst = makeRandomFleetInstance(profile, kSeed);
  ScreenParams screen;
  screen.maxSourcesPerSink = 3;
  const FleetReducedProblem reduced = makeFleetReducedProblem(inst, screen);

  ASSERT_EQ(reduced.perAsset.size(),
            static_cast<size_t>(numAssets(inst)));
  for (const ReducedProblem& asset : reduced.perAsset) {
    const Index numSources = static_cast<Index>(asset.sources.size());
    for (size_t t = 0; t < asset.sinks.size(); ++t) {
      const vector<Index>& kept = asset.kept[t];
      EXPECT_EQ(static_cast<Index>(kept.size()),
                std::min<Index>(3, numSources));
      for (size_t i = 1; i < kept.size(); ++i) {   // ascending round trips
        EXPECT_LE(asset.shipCost(kept[i - 1], static_cast<Index>(t)),
                  asset.shipCost(kept[i], static_cast<Index>(t)) + kTol);
      }
    }
  }
}

// Modeling errors are rejected: an asset with demand but no supply, and an
// asset with demand but no capable vehicle type.
TEST(NetworkFleetReduction, RejectsUnservableAssets) {
  {
    FleetInstance bad = makeDominatedArcInstance();
    bad.supplyCap(0, 0) = 0.0;       // demand at node 2, no supply anywhere
    validateFleetInstance(bad);
    EXPECT_THROW(makeFleetReducedProblem(bad), std::invalid_argument);
  }
  {
    FleetInstance bad = makeDominatedArcInstance();
    bad.assets[0].unitWeight = 5.0;  // heavier than any weight capacity...
    bad.vehicles[0].weightCap = 4.0; // ...kappa = min(4/5, 10/1) < 1? No:
    bad.vehicles[0].areaCap = 0.0;   // pure weight carrier, kappa = 4/5 > 0.
    bad.assets[0].unitArea = 1.0;    // area needed but areaCap = 0: kappa = 0.
    validateFleetInstance(bad);
    EXPECT_THROW(makeFleetReducedProblem(bad), std::invalid_argument);
  }
}
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------

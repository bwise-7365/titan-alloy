// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
// Tests for the Fleet app class (headless, no Qt): the QP solve converges
// feasibly, greedy/swap/sparsify behave, and an unsupported engine throws.
// ----------------------------------------------
#include "fleetproblem.hpp"

#include <gtest/gtest.h>

#include <cmath>
#include <cstdint>
#include <stdexcept>

using namespace VIMCP;
using namespace VIMCP::App;

namespace {

  constexpr std::uint64_t kSeed = 20260709ULL;

  // A small instance (10 nodes, the default 2 assets x 2 vehicle types) so the
  // QP solves quickly in Debug.
  Network::FleetProfile
  smallProfile()
  {
    Network::FleetProfile profile;
    profile.geometry.numSupplyOnly = 3;
    profile.geometry.numBoth       = 3;
    profile.geometry.numDemandOnly = 4;
    profile.geometry.numNeither    = 0;
    return profile;
  }

  Fleet
  smallFleet()
  {
    return Fleet(Fleet::generate(smallProfile(), kSeed));
  }

} // namespace

TEST(FleetProblem, OptimalSolveConvergesFeasibly)
{
  const Fleet problem = smallFleet();
  const auto [vi, res] = problem.solve(FleetParams{});   // Default -> Ipm
  (void)vi;

  EXPECT_TRUE(res.converged || res.certifiedP);
  EXPECT_TRUE(std::isfinite(res.shortfall));
  EXPECT_GE(res.shortfall, 0.0);
  EXPECT_GT(res.milesUsed.size(), 0);
  EXPECT_EQ(res.budgetShadowPrice.size(), res.milesUsed.size());
}

TEST(FleetProblem, GreedyPlanProducesFinitePlan)
{
  const Fleet problem = smallFleet();
  const Network::FleetGreedyResult greedy = problem.greedyPlan();

  EXPECT_TRUE(std::isfinite(greedy.shortfallValue));
  EXPECT_GE(greedy.shortfallValue, 0.0);
  EXPECT_GT(greedy.milesUsed.size(), 0);
}

TEST(FleetProblem, SwapDoesNotWorsenMiles)
{
  const Fleet problem = smallFleet();
  Network::FleetPlan plan = problem.greedyPlan().plan;
  const Network::FleetSwapSummary summary = problem.swapToLocalOptimum(plan);

  EXPECT_LE(summary.milesUsedAfter.sum(), summary.milesUsedBefore.sum() + 1.0e-6);
}

TEST(FleetProblem, SparsifyDoesNotIncreaseArcs)
{
  const Fleet problem = smallFleet();
  Network::FleetPlan plan = problem.greedyPlan().plan;
  const Network::FleetPurifySummary summary = problem.sparsify(plan);

  long before = 0;
  long after  = 0;
  for (const int arcs : summary.arcsBeforePerAsset) {
    before += arcs;
  }
  for (const int arcs : summary.arcsAfterPerAsset) {
    after += arcs;
  }
  EXPECT_LE(after, before);
}

TEST(FleetProblem, UnsupportedEngineThrows)
{
  const Fleet problem = smallFleet();
  FleetParams params;
  params.engine = ProblemBase::Engine::Chain;
  EXPECT_THROW(problem.solve(params), std::invalid_argument);
}

TEST(FleetProblem, SparsifyHookPreservesShortfallAndLowersMiles)
{
  // The framework hook: sparsify a whole FleetResult. Deliveries (hence
  // shortfall) are invariant; vehicle-miles do not increase.
  const Fleet problem = smallFleet();
  const auto [vi, res] = problem.solve(FleetParams{});
  (void)vi;
  const FleetResult sparse = problem.sparsify(res);
  EXPECT_NEAR(sparse.shortfall, res.shortfall, 1.0e-9);
  EXPECT_LE(sparse.milesUsed.sum(), res.milesUsed.sum() + 1.0e-6);
  EXPECT_EQ(sparse.milesUsed.size(), res.milesUsed.size());
}
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------

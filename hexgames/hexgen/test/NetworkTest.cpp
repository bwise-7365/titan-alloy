// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
// The road network between the cities, and the roads of the map edges.
// ----------------------------------------------
#include "HexGenTest.h"

#include "hexgen/Chains.h"
#include "hexgen/CostGraph.h"
#include "hexgen/Generate.h"
#include "hexgen/Network.h"
#include "hexgen/Random.h"

#include <gtest/gtest.h>

#include <algorithm>
#include <cstdlib>
#include <set>

namespace {

  using HexGen::kNoPath;
  using HexGen::Method;
  using HexGen::Role;
  using HexGen::Side;

  std::vector<int>
  hexesOf(const HexGen::GenMap& m, const HexGen::Settings& s, Role role)
  {
    std::vector<int> out;
    for (int hex = 0; hex < m.size(); ++hex) {
      if (s.terrainIndex(role) == m.terrain[static_cast<std::size_t>(hex)]) {
        out.push_back(hex);
      }
    }
    return out;
  }

  HexGen::Settings
  withSides(int roads, int cities)
  {
    HexGen::Settings s = HexGen::Settings::hexkrieg();
    s.cols = 20;
    s.rows = 15;
    s.roads->sides.clear();
    for (const Side side : {Side::Top, Side::Bottom, Side::Left, Side::Right}) {
      s.roads->sides.push_back({side, roads, cities});
    }
    return s;
  }

  TEST(HexGenNetwork, SpanningForestJoinsEachGroupAlone)
  {
    // Nodes 0, 1, 2 joined; 3 and 4 joined; no path between the two groups.
    const std::vector<std::vector<std::int64_t>> cost{
        {0, 5, 2, kNoPath, kNoPath},
        {5, 0, 1, kNoPath, kNoPath},
        {2, 1, 0, kNoPath, kNoPath},
        {kNoPath, kNoPath, kNoPath, 0, 7},
        {kNoPath, kNoPath, kNoPath, 7, 0}};
    const std::vector<std::pair<int, int>> forest = HexGen::spanningForest(cost);
    EXPECT_EQ((std::vector<std::pair<int, int>>{{0, 2}, {1, 2}, {3, 4}}), forest);
    return;
  }

  TEST(HexGenNetwork, TheTourVisitsEveryMemberOnce)
  {
    // Four points on a line at 0, 1, 5, 9: the tour starts with the farthest pair (0 and 9).
    const std::vector<int> at{0, 1, 5, 9};
    std::vector<std::vector<std::int64_t>> cost(4, std::vector<std::int64_t>(4));
    for (std::size_t i = 0; i < 4; ++i) {
      for (std::size_t j = 0; j < 4; ++j) {
        cost[i][j] = std::abs(at[i] - at[j]);
      }
    }
    const std::vector<int> tour = HexGen::furthestInsertionTour(cost, {0, 1, 2, 3});
    EXPECT_EQ(0, tour.front());
    EXPECT_EQ((std::set<int>{0, 1, 2, 3}), std::set<int>(tour.begin(), tour.end()));
    EXPECT_EQ(4u, tour.size());
    return;
  }

  TEST(HexGenNetwork, CitiesALandPathJoinsAreJoinedByRoad)
  {
    for (const Method method : {Method::Fractal, Method::Perlin}) {
      HexGen::Settings s = HexGen::Settings::hexkrieg();
      s.method = method;
      s.roads->sides.clear();
      const HexGen::GenMap m = HexGen::generateMap(s);
      const std::vector<int> cities = hexesOf(m, s, Role::City);
      ASSERT_LE(2u, cities.size());
      ASSERT_FALSE(m.roads().empty());
      const HexGen::CostGraph graph(m, s);
      HexSearch::SearchScratch scratch;
      for (const int a : cities) {
        const std::set<int> byRoad = HexGenTest::byRoadFrom(m, a);
        const HexSearch::Field<HexSearch::CostHalves> byLand = graph.searchFrom(a, scratch);
        for (const int b : cities) {
          EXPECT_EQ(byLand.reachedP(static_cast<HexSearch::NodeIndex>(b)), byRoad.contains(b)) << a << " " << b;
        }
      }
      for (const auto& [a, b] : m.roads()) {
        EXPECT_TRUE(graph.roadsEnterP(a) && graph.roadsEnterP(b));  // never into water
      }
    }
    return;
  }

  TEST(HexGenNetwork, NoCitiesNoRoads)
  {
    HexGen::Settings s = HexGen::Settings::hexkrieg();
    HexGenTest::terrainOf(s, Role::City).percent = 0;
    s.roads->sides.clear();
    EXPECT_TRUE(HexGen::generateMap(s).roads().empty());
    return;
  }

  TEST(HexGenEdges, EndsAreSpacedOnTheirSideAndJoinTheNetwork)
  {
    HexGen::Settings s = withSides(3, 2);
    HexGenTest::terrainOf(s, Role::Low).percent = 0;  // one land mass, so every end can be joined
    HexGen::Settings interior = s;
    interior.roads->sides.clear();
    HexGen::GenMap m = HexGen::generateMap(interior);
    for (const HexGen::SideSetting& side : s.roads->sides) {
      std::mt19937_64 gen(77u);
      const std::vector<int> ends = HexGen::addEdgeRoads(m, s, side, gen);
      EXPECT_EQ(3u, ends.size()) << HexGen::sideName(side.at);
      const bool across = Side::Top == side.at || Side::Bottom == side.at;
      const int length = across ? m.cols() : m.rows();
      std::vector<int> along;
      int cities = 0;
      for (const int hex : ends) {
        const int col = hex % m.cols();
        const int row = hex / m.cols();
        const int fixed = Side::Top == side.at ? 0 : Side::Bottom == side.at ? m.rows() - 1 : Side::Left == side.at ? 0 : m.cols() - 1;
        EXPECT_EQ(fixed, across ? row : col);
        const int p = across ? col : row;
        EXPECT_LT(0, p);
        EXPECT_GT(length - 1, p);
        along.push_back(p);
        cities += s.terrainIndex(Role::City) == m.terrain[static_cast<std::size_t>(hex)] ? 1 : 0;
        EXPECT_TRUE(m.onRoadP(hex));
        const std::set<int> byRoad = HexGenTest::byRoadFrom(m, hex);
        const std::vector<int> all = hexesOf(m, s, Role::City);
        EXPECT_TRUE(std::any_of(all.begin(), all.end(), [&](int c) { return c != hex && byRoad.contains(c); }));
      }
      EXPECT_LE(2, cities);
      EXPECT_TRUE(std::is_sorted(along.begin(), along.end()));
      for (std::size_t k = 1; k < along.size(); ++k) {
        EXPECT_LE(length, 2 * 3 * (along[k] - along[k - 1]));
      }
    }
    return;
  }

  TEST(HexGenEdges, EndsAreNeverWaterAndTheSameSeedGivesTheSameEnds)
  {
    const HexGen::Settings s = withSides(3, 1);
    const HexGen::SideSetting left{Side::Left, 3, 1};
    HexGen::Settings interior = s;
    interior.roads->sides.clear();
    HexGen::GenMap a = HexGen::generateMap(interior);
    HexGen::GenMap b = HexGen::generateMap(interior);
    std::mt19937_64 ga(5u);
    std::mt19937_64 gb(5u);
    const std::vector<int> ends = HexGen::addEdgeRoads(a, s, left, ga);
    EXPECT_EQ(ends, HexGen::addEdgeRoads(b, s, left, gb));
    for (const int hex : ends) {
      EXPECT_NE(s.terrainIndex(Role::Low), a.terrain[static_cast<std::size_t>(hex)]);
    }
    return;
  }

  TEST(HexGenEdges, NoRoadsOnASideLaysNothingThere)
  {
    HexGen::Settings s = withSides(0, 0);
    const HexGen::GenMap with = HexGen::generateMap(s);
    s.roads->sides.clear();
    EXPECT_EQ(with.roads(), HexGen::generateMap(s).roads());
    return;
  }

  TEST(HexGenChains, ChainsCutWhereTheNetworkBranchesAndCloseLoops)
  {
    // A Y: 0-1, 1-2, 1-3; and a triangle 4-5, 5-6, 4-6 (a loop with no branch).
    const std::set<std::pair<int, int>> steps{{0, 1}, {1, 2}, {1, 3}, {4, 5}, {5, 6}, {4, 6}};
    const std::vector<std::vector<int>> chains = HexGen::chainsOf(steps, 7);
    EXPECT_EQ((std::vector<std::vector<int>>{{0, 1}, {1, 2}, {1, 3}, {4, 5, 6, 4}}), chains);
    return;
  }

}  // namespace
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------

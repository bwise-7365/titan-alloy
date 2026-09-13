// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
#include "ToyBoard.h"

#include <gtest/gtest.h>

#include <cstdint>
#include <functional>
#include <optional>
#include <random>
#include <span>
#include <vector>

namespace {

  using HexSearch::Algorithms;
  using HexSearch::CostHalves;
  using HexSearch::NodeIndex;
  using HexSearch::SearchScratch;
  using HexSearch::Source;

  // A costing that varies from arc to arc but never below one whole point, so that 2 * hexDist is
  // an admissible heuristic. Drawn once from a literal seed, so the board is the same every run.
  std::vector<int>
  arcCosts(const HexModel::Board& board, std::uint64_t seed)
  {
    std::mt19937_64 stream(seed);
    std::uniform_int_distribution<int> pick(1, 3);
    std::vector<int> costs(board.hexCount() * 6, 2);
    for (int& cost : costs) {
      cost = 2 * pick(stream);
    }
    return costs;
  }

  int
  pathCost(const HexModel::Board& board, const std::vector<int>& costs, const std::vector<NodeIndex>& path)
  {
    int total = 0;
    for (std::size_t i = 1; i < path.size(); ++i) {
      const HexModel::HexIndex from{path[i - 1]};
      bool foundP = false;
      for (int d = 0; d < HexCoord::kDirections; ++d) {
        const std::optional<HexModel::HexIndex> next = board.neighbour(from, static_cast<HexModel::Direction>(d));
        if (next && next->value == path[i]) {
          total += costs[from.value * 6 + static_cast<std::size_t>(d)];
          foundP = true;
          break;
        }
      }
      if (!foundP) {
        throw std::invalid_argument("AStarTest: path step is not an adjacency");
      }
    }
    return total;
  }

}  // namespace

TEST(AStarTest, FindsTheSamePathAsDijkstra)
{
  const HexModel::Board board = ToyBoard::build();
  const HexSearch::HexAdjacencyGraph graph = ToyBoard::openGraph(board);

  for (std::uint64_t seed : {20260913ull, 20260914ull, 20260915ull}) {
    const std::vector<int> costs = arcCosts(board, seed);
    const std::function<std::optional<CostHalves>(NodeIndex, std::size_t)> arcCost =
        [&costs](NodeIndex from, std::size_t arcIndex) {
          return std::optional<CostHalves>(CostHalves{costs[from * 6 + arcIndex]});
        };

    const HexModel::HexIndex from = ToyBoard::at(board, "A1");
    const HexModel::HexIndex to = ToyBoard::at(board, "E5");

    SearchScratch scratch;
    const Source source{from.value, CostHalves::zero()};
    const std::span<const Source> sources(&source, 1);
    const HexSearch::Field<CostHalves> field =
        Algorithms::dijkstraBounded(graph, scratch, sources, arcCost, CostHalves{9999}, nullptr);
    ASSERT_TRUE(field.reachedP(to.value));
    const int best = field.distance(to.value).halves;

    const std::function<CostHalves(NodeIndex)> heuristic = [&board, to](NodeIndex n) {
      return CostHalves{2 * board.distance(HexModel::HexIndex{n}, to)};
    };
    const std::optional<std::vector<NodeIndex>> path =
        Algorithms::aStar(graph, scratch, from.value, to.value, arcCost, heuristic);
    ASSERT_TRUE(path.has_value());
    EXPECT_EQ(from.value, path->front());
    EXPECT_EQ(to.value, path->back());
    EXPECT_EQ(best, pathCost(board, costs, *path));
  }
}

TEST(AStarTest, NoPathWhenEveryArcIsRefused)
{
  const HexModel::Board board = ToyBoard::build();
  const HexSearch::HexAdjacencyGraph graph = ToyBoard::openGraph(board);
  SearchScratch scratch;

  const std::function<std::optional<CostHalves>(NodeIndex, std::size_t)> refuse =
      [](NodeIndex, std::size_t) { return std::optional<CostHalves>(); };
  const std::function<CostHalves(NodeIndex)> zero = [](NodeIndex) { return CostHalves::zero(); };

  EXPECT_FALSE(Algorithms::aStar(graph, scratch, ToyBoard::node(board, "A1"), ToyBoard::node(board, "E5"),
                                  refuse, zero)
                   .has_value());
}
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------

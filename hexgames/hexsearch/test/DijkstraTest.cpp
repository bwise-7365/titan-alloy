// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
#include "ToyBoard.h"

#include <gtest/gtest.h>

#include <algorithm>
#include <functional>
#include <optional>
#include <span>
#include <vector>

namespace {

  using HexSearch::Algorithms;
  using HexSearch::CostHalves;
  using HexSearch::NodeIndex;
  using HexSearch::SearchScratch;
  using HexSearch::Source;

  std::optional<CostHalves>
  oneEach(NodeIndex, std::size_t)
  {
    return CostHalves{1};
  }

}  // namespace

TEST(DijkstraTest, UnitCostsOnAToyBoard)
{
  const HexModel::Board board = ToyBoard::build();
  const HexSearch::HexAdjacencyGraph graph = ToyBoard::openGraph(board);
  SearchScratch scratch;

  const HexModel::HexIndex seed = ToyBoard::at(board, "C3");
  const Source source{seed.value, CostHalves::zero()};
  const std::span<const Source> sources(&source, 1);
  const HexSearch::Field<CostHalves> field =
      Algorithms::dijkstraBounded(graph, scratch, sources, &oneEach, CostHalves{99}, nullptr);

  EXPECT_EQ(board.hexCount(), field.reached().size());
  for (NodeIndex reached : field.reached()) {
    EXPECT_EQ(board.distance(seed, HexModel::HexIndex{reached}), field.distance(reached).halves)
        << board.id(HexModel::HexIndex{reached}).text;
  }

  // A path is a walk of adjacent hexes from the source, one hex longer than its cost.
  const NodeIndex corner = ToyBoard::node(board, "E1");
  const std::vector<NodeIndex> path = field.pathTo(corner);
  ASSERT_FALSE(path.empty());
  EXPECT_EQ(seed.value, path.front());
  EXPECT_EQ(corner, path.back());
  EXPECT_EQ(static_cast<std::size_t>(field.distance(corner).halves) + 1, path.size());
  for (std::size_t i = 1; i < path.size(); ++i) {
    EXPECT_EQ(1, board.distance(HexModel::HexIndex{path[i - 1]}, HexModel::HexIndex{path[i]}));
  }
}

TEST(DijkstraTest, CeilingAndTerminals)
{
  const HexModel::Board board = ToyBoard::build();
  const HexSearch::HexAdjacencyGraph graph = ToyBoard::openGraph(board);
  SearchScratch scratch;

  const HexModel::HexIndex seed = ToyBoard::at(board, "C3");
  const Source source{seed.value, CostHalves::zero()};
  const std::span<const Source> sources(&source, 1);

  {
    const HexSearch::Field<CostHalves> bounded =
        Algorithms::dijkstraBounded(graph, scratch, sources, &oneEach, CostHalves{1}, nullptr);
    for (std::size_t h = 0; h < board.hexCount(); ++h) {
      const HexModel::HexIndex hex{static_cast<std::uint32_t>(h)};
      const bool withinP = 1 >= board.distance(seed, hex);
      EXPECT_EQ(withinP, bounded.reachedP(hex.value)) << board.id(hex).text;
    }
  }

  {
    // Every hex but the source is a "must stop on entry" hex: reached, never expanded.
    const std::function<bool(NodeIndex)> terminalP = [&](NodeIndex n) { return n != seed.value; };
    const HexSearch::Field<CostHalves> stopped =
        Algorithms::dijkstraBounded(graph, scratch, sources, &oneEach, CostHalves{99}, terminalP);
    for (NodeIndex reached : stopped.reached()) {
      EXPECT_GE(1, board.distance(seed, HexModel::HexIndex{reached}));
    }
    EXPECT_FALSE(stopped.reachedP(ToyBoard::node(board, "A1")));
    EXPECT_TRUE(stopped.reachedP(ToyBoard::node(board, "D3")));
  }
}

TEST(DijkstraTest, MultiSourceWithIndividualBudgets)
{
  const HexModel::Board board = ToyBoard::build();
  const HexSearch::HexAdjacencyGraph graph = ToyBoard::openGraph(board);
  SearchScratch scratch;

  const HexModel::HexIndex west = ToyBoard::at(board, "A1");
  const HexModel::HexIndex east = ToyBoard::at(board, "E5");
  const std::vector<Source> sources{Source{west.value, CostHalves::zero()}, Source{east.value, CostHalves{2}}};
  const HexSearch::Field<CostHalves> field = Algorithms::dijkstraBounded(
      graph, scratch, std::span<const Source>(sources), &oneEach, CostHalves{3}, nullptr);

  EXPECT_EQ(0, field.distance(west.value).halves);
  EXPECT_EQ(west.value, field.origin(west.value));
  EXPECT_EQ(2, field.distance(east.value).halves);
  EXPECT_EQ(east.value, field.origin(east.value));

  // Each hex is claimed by whichever source spends least reaching it, ties going to the earlier one.
  for (NodeIndex reached : field.reached()) {
    const HexModel::HexIndex hex{reached};
    const int fromWest = board.distance(west, hex);
    const int fromEast = 2 + board.distance(east, hex);
    EXPECT_EQ(std::min(fromWest, fromEast), field.distance(reached).halves) << board.id(hex).text;
    const NodeIndex expected = fromWest <= fromEast ? west.value : east.value;
    EXPECT_EQ(expected, field.origin(reached)) << board.id(hex).text;
  }
}
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------

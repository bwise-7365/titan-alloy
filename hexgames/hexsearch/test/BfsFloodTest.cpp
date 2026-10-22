// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
#include "ToyBoard.h"

#include <gtest/gtest.h>

#include <functional>
#include <span>

TEST(BfsFloodTest, DepthBounded)
{
  const HexModel::Board board = ToyBoard::build();
  const HexSearch::HexAdjacencyGraph graph = ToyBoard::openGraph(board);
  HexSearch::SearchScratch scratch;

  const HexModel::HexIndex seed = ToyBoard::at(board, "C3");
  const HexSearch::NodeIndex source = seed.value;
  const std::function<bool(HexSearch::NodeIndex, std::size_t)> anyArc = [](HexSearch::NodeIndex, std::size_t) {
    return true;
  };

  const HexSearch::Field<HexSearch::CostHalves> field = HexSearch::Algorithms::bfsFlood(
      graph, scratch, std::span<const HexSearch::NodeIndex>(&source, 1), anyArc, 2);

  for (std::size_t h = 0; h < board.hexCount(); ++h) {
    const HexModel::HexIndex hex{static_cast<std::uint32_t>(h)};
    const bool withinP = 2 >= board.distance(seed, hex);
    EXPECT_EQ(withinP, field.reachedP(hex.value)) << board.id(hex).text;
    if (withinP) {
      EXPECT_EQ(board.distance(seed, hex), field.distance(hex.value).halves) << board.id(hex).text;
    }
  }
}

TEST(BfsFloodTest, WalledArcsAreNotCrossed)
{
  const HexModel::Board board = ToyBoard::build();
  const HexSearch::HexAdjacencyGraph graph = ToyBoard::openGraph(board);
  HexSearch::SearchScratch scratch;

  // The toy sheet's wall runs along every hexside between row 3 and row 4, so a flood that refuses
  // walled hexsides never leaves the northern three rows, however deep it is allowed to go.
  const HexSearch::NodeIndex source = ToyBoard::node(board, "A1");
  const std::function<bool(HexSearch::NodeIndex, std::size_t)> unwalled =
      [&board](HexSearch::NodeIndex from, std::size_t arcIndex) {
        return !ToyBoard::walledP(board, HexModel::HexIndex{from}, static_cast<HexModel::Direction>(arcIndex));
      };

  const HexSearch::Field<HexSearch::CostHalves> field = HexSearch::Algorithms::bfsFlood(
      graph, scratch, std::span<const HexSearch::NodeIndex>(&source, 1), unwalled, 99);

  EXPECT_EQ(15u, field.reached().size());
  EXPECT_TRUE(field.reachedP(ToyBoard::node(board, "E3")));
  EXPECT_FALSE(field.reachedP(ToyBoard::node(board, "A4")));
  EXPECT_FALSE(field.reachedP(ToyBoard::node(board, "E5")));
}
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------

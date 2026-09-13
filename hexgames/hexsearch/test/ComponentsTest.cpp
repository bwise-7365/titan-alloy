// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
#include "ToyBoard.h"

#include <gtest/gtest.h>

#include <cstdint>
#include <functional>
#include <set>
#include <vector>

TEST(ComponentsTest, EdgeBlockedComponents)
{
  const HexModel::Board board = ToyBoard::build();
  const HexSearch::HexAdjacencyGraph walled = ToyBoard::walledGraph(board);
  HexSearch::SearchScratch scratch;

  const std::function<bool(HexSearch::NodeIndex, std::size_t)> anyArc = [](HexSearch::NodeIndex, std::size_t) {
    return true;
  };
  const std::vector<std::uint32_t> component = HexSearch::Algorithms::components(walled, scratch, anyArc);
  ASSERT_EQ(board.hexCount(), component.size());

  const std::set<std::uint32_t> labels(component.begin(), component.end());
  EXPECT_EQ(2u, labels.size());

  const std::uint32_t north = component[ToyBoard::node(board, "A1")];
  const std::uint32_t south = component[ToyBoard::node(board, "A4")];
  EXPECT_NE(north, south);
  for (const char* id : {"A1", "B1", "C1", "D1", "E1", "A2", "E2", "A3", "C3", "E3"}) {
    EXPECT_EQ(north, component[ToyBoard::node(board, id)]) << id;
  }
  for (const char* id : {"A4", "C4", "E4", "A5", "C5", "E5"}) {
    EXPECT_EQ(south, component[ToyBoard::node(board, id)]) << id;
  }
}

TEST(ComponentsTest, NoWallIsOneComponent)
{
  const HexModel::Board board = ToyBoard::build();
  const HexSearch::HexAdjacencyGraph open = ToyBoard::openGraph(board);
  HexSearch::SearchScratch scratch;

  const std::function<bool(HexSearch::NodeIndex, std::size_t)> anyArc = [](HexSearch::NodeIndex, std::size_t) {
    return true;
  };
  const std::vector<std::uint32_t> component = HexSearch::Algorithms::components(open, scratch, anyArc);
  const std::set<std::uint32_t> labels(component.begin(), component.end());
  EXPECT_EQ(1u, labels.size());
}
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------

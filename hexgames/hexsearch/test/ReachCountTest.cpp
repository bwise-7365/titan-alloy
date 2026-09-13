// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
#include "ToyBoard.h"

#include <gtest/gtest.h>

#include <functional>
#include <span>
#include <vector>

TEST(ReachCountTest, StopsAtThreshold)
{
  const HexModel::Board board = ToyBoard::build();
  const HexSearch::HexAdjacencyGraph graph = ToyBoard::openGraph(board);
  HexSearch::SearchScratch scratch;

  const HexSearch::NodeIndex from = ToyBoard::node(board, "A1");
  const std::vector<HexSearch::NodeIndex> targets{ToyBoard::node(board, "E1"), ToyBoard::node(board, "E5")};

  const std::function<bool(HexSearch::NodeIndex)> nothingBlocked = [](HexSearch::NodeIndex) { return false; };
  EXPECT_EQ(2, HexSearch::Algorithms::reachCount(graph, scratch, from,
                                                  std::span<const HexSearch::NodeIndex>(targets),
                                                  nothingBlocked, 2));

  // Row 3 is the only way from the northern rows to the southern ones, so walling it off leaves
  // exactly one of the two targets reachable -- Tarawa's "communication needs two" answering one.
  const std::function<bool(HexSearch::NodeIndex)> rowThreeBlocked = [&board](HexSearch::NodeIndex n) {
    const std::string& id = board.id(HexModel::HexIndex{n}).text;
    return '3' == id.back();
  };
  EXPECT_EQ(1, HexSearch::Algorithms::reachCount(graph, scratch, from,
                                                  std::span<const HexSearch::NodeIndex>(targets),
                                                  rowThreeBlocked, 2));
}
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------

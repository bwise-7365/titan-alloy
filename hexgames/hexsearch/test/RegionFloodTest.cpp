// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
#include "ToyBoard.h"

#include <gtest/gtest.h>

#include <algorithm>
#include <functional>
#include <vector>

TEST(RegionFloodTest, BoundedByWalls)
{
  const HexModel::Board board = ToyBoard::build();

  const std::function<bool(HexModel::HexIndex, HexModel::Direction)> wallP =
      [&board](HexModel::HexIndex hex, HexModel::Direction d) { return ToyBoard::walledP(board, hex, d); };

  const std::vector<HexModel::HexIndex> north =
      HexSearch::regionFlood(board, ToyBoard::at(board, "A1"), wallP);
  EXPECT_EQ(15u, north.size());
  EXPECT_TRUE(north.end() != std::find(north.begin(), north.end(), ToyBoard::at(board, "E3")));
  EXPECT_TRUE(north.end() == std::find(north.begin(), north.end(), ToyBoard::at(board, "A4")));

  const std::vector<HexModel::HexIndex> south =
      HexSearch::regionFlood(board, ToyBoard::at(board, "E5"), wallP);
  EXPECT_EQ(10u, south.size());
  EXPECT_TRUE(south.end() == std::find(south.begin(), south.end(), ToyBoard::at(board, "C3")));
}

TEST(RegionFloodTest, NoWallsReachTheWholeBoard)
{
  const HexModel::Board board = ToyBoard::build();
  const std::function<bool(HexModel::HexIndex, HexModel::Direction)> open =
      [](HexModel::HexIndex, HexModel::Direction) { return false; };
  EXPECT_EQ(board.hexCount(), HexSearch::regionFlood(board, ToyBoard::at(board, "C3"), open).size());
}
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------

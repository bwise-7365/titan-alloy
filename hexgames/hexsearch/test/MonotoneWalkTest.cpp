// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
#include "ToyBoard.h"

#include <gtest/gtest.h>

#include <functional>
#include <vector>

TEST(MonotoneWalkTest, EachStepStrictlyFarther)
{
  const HexModel::Board board = ToyBoard::build();

  const HexModel::HexIndex origin = ToyBoard::at(board, "C3");
  const std::function<bool(HexModel::HexIndex)> anywhere = [](HexModel::HexIndex) { return true; };

  const std::vector<HexModel::HexIndex> ends = HexSearch::monotoneWalk(board, origin, origin, 2, anywhere);
  EXPECT_FALSE(ends.empty());
  for (HexModel::HexIndex end : ends) {
    EXPECT_EQ(2, board.distance(origin, end)) << board.id(end).text;
  }

  // No step may stand still or come back: a walk of zero steps is where it started.
  const std::vector<HexModel::HexIndex> none = HexSearch::monotoneWalk(board, origin, origin, 0, anywhere);
  ASSERT_EQ(1u, none.size());
  EXPECT_EQ(origin, none.front());
}

TEST(MonotoneWalkTest, DisallowedHexesAreNeverEntered)
{
  const HexModel::Board board = ToyBoard::build();

  const HexModel::HexIndex origin = ToyBoard::at(board, "C3");
  const HexModel::HexIndex forbidden = ToyBoard::at(board, "D3");
  const std::function<bool(HexModel::HexIndex)> exceptOne = [forbidden](HexModel::HexIndex h) {
    return h != forbidden;
  };

  const std::vector<HexModel::HexIndex> ends = HexSearch::monotoneWalk(board, origin, origin, 1, exceptOne);
  for (HexModel::HexIndex end : ends) {
    EXPECT_NE(forbidden, end);
    EXPECT_EQ(1, board.distance(origin, end));
  }
}
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------

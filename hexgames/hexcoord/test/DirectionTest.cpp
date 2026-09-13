// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
// The six directions: one cycle, two sets of compass names, and no direction at all for anything
// that is not a unit step.
// ----------------------------------------------
#include "hexcoord/Direction.h"

#include <gtest/gtest.h>

#include <array>
#include <stdexcept>
#include <string_view>

namespace {

  using HexCoord::Direction;
  using HexCoord::kDirections;
  using HexCoord::Orientation;
  using HexCoord::Qrs;

  constexpr std::array<Direction, 6> kCycle = {Direction::D0, Direction::D1, Direction::D2,
                                               Direction::D3, Direction::D4, Direction::D5};

}  // namespace

TEST(DirectionTest, RotateIsCyclicAndOppositeIsThree)
{
  for (const Direction d : kCycle) {
    EXPECT_EQ(d, HexCoord::rotate(d, kDirections));
    EXPECT_EQ(d, HexCoord::rotate(d, -kDirections));
    EXPECT_EQ(d, HexCoord::rotate(HexCoord::rotate(d, 1), -1));
    EXPECT_EQ(HexCoord::opposite(d), HexCoord::rotate(d, 3));
    EXPECT_EQ(d, HexCoord::opposite(HexCoord::opposite(d)));
    EXPECT_EQ(HexCoord::step(d) * -1, HexCoord::step(HexCoord::opposite(d)));
  }
}

TEST(DirectionTest, CompassNamesPerOrientation)
{
  const std::array<std::string_view, 6> flat = {"n", "ne", "se", "s", "sw", "nw"};
  const std::array<std::string_view, 6> pointy = {"ne", "e", "se", "sw", "w", "nw"};
  for (int i = 0; i < kDirections; ++i) {
    const Direction d = kCycle[static_cast<std::size_t>(i)];
    EXPECT_EQ(flat[static_cast<std::size_t>(i)], HexCoord::compassName(d, Orientation::Flat));
    EXPECT_EQ(pointy[static_cast<std::size_t>(i)], HexCoord::compassName(d, Orientation::Pointy));
    EXPECT_EQ(d, HexCoord::fromCompass(HexCoord::compassName(d, Orientation::Flat),
                                       Orientation::Flat));
    EXPECT_EQ(d, HexCoord::fromCompass(HexCoord::compassName(d, Orientation::Pointy),
                                       Orientation::Pointy));
  }
  // "e" and "w" are hexsides of a pointy-topped hex only; "n" and "s" of a flat-topped one only.
  EXPECT_THROW(HexCoord::fromCompass("e", Orientation::Flat), std::invalid_argument);
  EXPECT_THROW(HexCoord::fromCompass("w", Orientation::Flat), std::invalid_argument);
  EXPECT_THROW(HexCoord::fromCompass("n", Orientation::Pointy), std::invalid_argument);
  EXPECT_THROW(HexCoord::fromCompass("north", Orientation::Flat), std::invalid_argument);
}

TEST(DirectionTest, DirectionOfUnitStepsOnly)
{
  for (const Direction d : kCycle) {
    EXPECT_EQ(d, HexCoord::directionOf(HexCoord::step(d)));
  }
  EXPECT_THROW(HexCoord::directionOf(HexCoord::QVec * 2), std::invalid_argument);
  EXPECT_THROW(HexCoord::directionOf(Qrs{}), std::invalid_argument);
  EXPECT_THROW(HexCoord::directionOf(HexCoord::QVec + HexCoord::RVec * 2), std::invalid_argument);
}

TEST(DirectionTest, EdgeAnglesAreTheRenderersEdgeOrder)
{
  const std::array<double, 6> flat = {270.0, 330.0, 30.0, 90.0, 150.0, 210.0};
  const std::array<double, 6> pointy = {300.0, 0.0, 60.0, 120.0, 180.0, 240.0};
  for (int i = 0; i < kDirections; ++i) {
    const Direction d = kCycle[static_cast<std::size_t>(i)];
    EXPECT_DOUBLE_EQ(flat[static_cast<std::size_t>(i)],
                     HexCoord::edgeAngleDegrees(d, Orientation::Flat));
    EXPECT_DOUBLE_EQ(pointy[static_cast<std::size_t>(i)],
                     HexCoord::edgeAngleDegrees(d, Orientation::Pointy));
  }
}
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------

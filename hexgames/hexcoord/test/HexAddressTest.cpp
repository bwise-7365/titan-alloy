// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
// Centres, vertices, hexsides and neighbourhoods. The first three cases are tricoord's
// testHexVertex and testHexCenter, case for case.
// ----------------------------------------------
#include "hexcoord/HexAddress.h"

#include <gtest/gtest.h>

#include <algorithm>
#include <random>
#include <stdexcept>

namespace {

  using HexCoord::Abc;
  using HexCoord::AVec;
  using HexCoord::BVec;
  using HexCoord::CVec;
  using HexCoord::Direction;
  using HexCoord::edgeDist;
  using HexCoord::HexCentre;
  using HexCoord::HexEdge;
  using HexCoord::HexVertex;
  using HexCoord::Qrs;

  constexpr int kIterations = 400;

  std::mt19937_64
  seeded()
  {
    return std::mt19937_64(20260912u);
  }

  std::uniform_int_distribution<int>
  spread()
  {
    return std::uniform_int_distribution<int>(-10, 20);
  }

  bool
  holdsP(const std::array<HexCentre, 3>& hexes, HexCentre wanted)
  {
    return hexes.end() != std::find(hexes.begin(), hexes.end(), wanted);
  }

}  // namespace

TEST(HexAddressTest, VertexClass1And4HaveCentresAtPlusABC)
{
  std::mt19937_64 rng = seeded();
  std::uniform_int_distribution<int> draw = spread();
  int seen = 0;
  for (int i = 0; i < kIterations; ++i) {
    const Abc v{draw(rng), draw(rng), draw(rng)};
    const int code = v.hvCode();
    if (1 != code && 4 != code) {
      continue;
    }
    ++seen;
    // tricoord's case table: class 1 opens right to an odd column, class 4 to an even one.
    const int atA = 1 == code ? 3 : 0;
    const int atBc = 1 == code ? 0 : 3;
    EXPECT_EQ(atA, (v + AVec).hvCode());
    EXPECT_EQ(atBc, (v + BVec).hvCode());
    EXPECT_EQ(atBc, (v + CVec).hvCode());
    EXPECT_EQ(1, edgeDist(v, v + AVec));

    const HexVertex vertex{v};
    const std::array<HexCentre, 3> hexes = vertex.hexes();
    EXPECT_TRUE(holdsP(hexes, HexCentre{v + AVec}));
    EXPECT_TRUE(holdsP(hexes, HexCentre{v + BVec}));
    EXPECT_TRUE(holdsP(hexes, HexCentre{v + CVec}));
    for (const HexVertex& near : vertex.neighbours()) {
      EXPECT_EQ(1, edgeDist(v, near.abc()));
    }
  }
  EXPECT_LT(0, seen);
}

TEST(HexAddressTest, VertexClass2And5HaveCentresAtMinusABC)
{
  std::mt19937_64 rng = seeded();
  std::uniform_int_distribution<int> draw = spread();
  int seen = 0;
  for (int i = 0; i < kIterations; ++i) {
    const Abc v{draw(rng), draw(rng), draw(rng)};
    const int code = v.hvCode();
    if (2 != code && 5 != code) {
      continue;
    }
    ++seen;
    const int atA = 2 == code ? 0 : 3;
    const int atBc = 2 == code ? 3 : 0;
    EXPECT_EQ(atA, (v - AVec).hvCode());
    EXPECT_EQ(atBc, (v - BVec).hvCode());
    EXPECT_EQ(atBc, (v - CVec).hvCode());
    EXPECT_EQ(1, edgeDist(v, v - AVec));

    const std::array<HexCentre, 3> hexes = HexVertex{v}.hexes();
    EXPECT_TRUE(holdsP(hexes, HexCentre{v - AVec}));
    EXPECT_TRUE(holdsP(hexes, HexCentre{v - BVec}));
    EXPECT_TRUE(holdsP(hexes, HexCentre{v - CVec}));
  }
  EXPECT_LT(0, seen);
}

TEST(HexAddressTest, EveryQrsIsACentre)
{
  std::mt19937_64 rng = seeded();
  std::uniform_int_distribution<int> draw = spread();
  for (int i = 0; i < kIterations; ++i) {
    const Qrs w{draw(rng), draw(rng), draw(rng)};
    const Abc v = w.toAbc();
    EXPECT_TRUE(0 == v.hvCode() || 3 == v.hvCode());
    const HexCentre centre = HexCentre::fromQrs(w);
    EXPECT_EQ(v, centre.abc());
    EXPECT_EQ(w, centre.qrs());  // and back again, exactly
    EXPECT_NO_THROW(HexCentre{v});
    EXPECT_THROW(HexVertex{v}, std::invalid_argument);
  }
}

TEST(HexAddressTest, EdgeBetweenAndHexesRoundTrip)
{
  const HexCentre home{Abc{}};
  for (int i = 0; i < HexCoord::kDirections; ++i) {
    const Direction d = static_cast<Direction>(i);
    const HexCentre away = home.neighbour(d);
    const HexEdge side = HexEdge::between(home, d);
    EXPECT_EQ(side, home.edge(d));
    EXPECT_EQ(side, away.edge(HexCoord::opposite(d)));  // one hexside, two hexes

    const std::pair<HexCentre, HexCentre> pair = side.hexes();
    const bool bothP = (pair.first == home && pair.second == away) ||
                       (pair.first == away && pair.second == home);
    EXPECT_TRUE(bothP);

    const std::pair<HexVertex, HexVertex> corners = side.vertices();
    EXPECT_EQ(1, edgeDist(corners.first.abc(), corners.second.abc()));
    EXPECT_EQ(1, edgeDist(home.abc(), corners.first.abc()));
    EXPECT_EQ(1, edgeDist(home.abc(), corners.second.abc()));
    EXPECT_EQ(corners.first.abc() + corners.second.abc(), side.twiceMidpoint());
  }
  // Vertices that are not one edge apart do not bound a hexside.
  const std::array<HexVertex, 6> corners = home.vertices();
  EXPECT_THROW(HexEdge::of(corners[0], corners[2]), std::invalid_argument);
  EXPECT_THROW(HexEdge::of(corners[0], corners[0]), std::invalid_argument);
}

TEST(HexAddressTest, VerticesAreClockwiseAndShared)
{
  const HexCentre home{Abc{}};
  const std::array<HexVertex, 6> corners = home.vertices();
  for (std::size_t i = 0; i < corners.size(); ++i) {
    EXPECT_EQ(1, edgeDist(home.abc(), corners[i].abc()));
    for (std::size_t j = i + 1; j < corners.size(); ++j) {
      EXPECT_NE(corners[i], corners[j]);
    }
    // Consecutive corners bound the hexside of the direction they start.
    const Direction d = static_cast<Direction>(i);
    const std::pair<HexVertex, HexVertex> side = home.edge(d).vertices();
    const HexVertex next = corners[(i + 1) % corners.size()];
    const bool pairP = (side.first == corners[i] && side.second == next) ||
                       (side.first == next && side.second == corners[i]);
    EXPECT_TRUE(pairP);
  }
  // Two adjacent hexes share exactly the two corners of the hexside between them.
  for (const HexCentre& away : HexCoord::neighbours(home)) {
    const std::array<HexVertex, 6> theirs = away.vertices();
    int shared = 0;
    for (const HexVertex& mine : corners) {
      if (theirs.end() != std::find(theirs.begin(), theirs.end(), mine)) {
        ++shared;
      }
    }
    EXPECT_EQ(2, shared);
    EXPECT_EQ(1, HexCoord::hexDist(home, away));
  }
}

TEST(HexAddressTest, RingAndDiscCounts)
{
  const HexCentre home = HexCentre::fromQrs(Qrs{3, -2, 0});
  for (int radius = 1; radius <= 5; ++radius) {
    const std::vector<HexCentre> shell = HexCoord::ring(home, radius);
    EXPECT_EQ(static_cast<std::size_t>(6 * radius), shell.size());
    for (const HexCentre& h : shell) {
      EXPECT_EQ(radius, HexCoord::hexDist(home, h));
    }
    std::vector<HexCentre> sorted = shell;
    std::sort(sorted.begin(), sorted.end());
    EXPECT_EQ(sorted.end(), std::unique(sorted.begin(), sorted.end()));
    // Consecutive hexes of a ring are neighbours, and the walk closes.
    for (std::size_t i = 0; i < shell.size(); ++i) {
      EXPECT_EQ(1, HexCoord::hexDist(shell[i], shell[(i + 1) % shell.size()]));
    }
    EXPECT_EQ(home.neighbour(Direction::D0) + HexCoord::step(Direction::D0) * (radius - 1),
              shell.front());

    const std::vector<HexCentre> filled = HexCoord::disc(home, radius);
    EXPECT_EQ(static_cast<std::size_t>(3 * radius * radius + 3 * radius + 1), filled.size());
    EXPECT_EQ(home, filled.front());
  }
  EXPECT_EQ(std::size_t{1}, HexCoord::disc(home, 0).size());
  EXPECT_THROW(HexCoord::ring(home, 0), std::invalid_argument);
  EXPECT_THROW(HexCoord::disc(home, -1), std::invalid_argument);
}
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------

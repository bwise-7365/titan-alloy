// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
// The ABC and QRS algebra, ported from panj/hexmap/src/testtri.cpp: the same random walk over the
// same range, with the same assertions, and the same number of iterations.
// ----------------------------------------------
#include "hexcoord/Abc.h"

#include <gtest/gtest.h>

#include <algorithm>
#include <cstdlib>
#include <random>
#include <sstream>

namespace {

  using HexCoord::Abc;
  using HexCoord::edgeDist;
  using HexCoord::hexDist;
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

}  // namespace

TEST(AbcTest, ReduceIsIdempotentAndCanonical)
{
  std::mt19937_64 rng = seeded();
  std::uniform_int_distribution<int> draw = spread();
  for (int i = 0; i < kIterations; ++i) {
    const int a = draw(rng);
    const int b = draw(rng);
    const int c = draw(rng);
    const Abc v{a, b, c};

    // height() is the least of the three ways of zeroing one component ...
    const int byA = std::abs(b - a) + std::abs(c - a);
    const int byB = std::abs(a - b) + std::abs(c - b);
    const int byC = std::abs(a - c) + std::abs(b - c);
    const int least = std::min(byA, std::min(byB, byC));
    EXPECT_EQ(least, v.height());

    // ... and ties are broken in favour of zeroing a, then b, then c.
    if (byA <= byB && byA <= byC) {
      EXPECT_EQ(0, v.a());
    }
    else if (byB <= byC) {
      EXPECT_EQ(0, v.b());
    }
    else {
      EXPECT_EQ(0, v.c());
    }

    const Abc again{v.a(), v.b(), v.c()};
    EXPECT_EQ(v, again);
  }
}

TEST(AbcTest, DiagonalShiftIsSameClass)
{
  std::mt19937_64 rng = seeded();
  std::uniform_int_distribution<int> draw = spread();
  for (int i = 0; i < kIterations; ++i) {
    const int a = draw(rng);
    const int b = draw(rng);
    const int c = draw(rng);
    const int d = draw(rng);
    const Abc v1{a, b, c};
    const Abc v2{a + d, b + d, c + d};
    EXPECT_EQ(v1, v2);
    EXPECT_EQ(v1.hvCode(), v2.hvCode());
  }
}

TEST(AbcTest, OffsetHeightEqualsEdgeDist)
{
  std::mt19937_64 rng = seeded();
  std::uniform_int_distribution<int> draw = spread();
  for (int i = 0; i < kIterations; ++i) {
    const Abc v1{draw(rng), draw(rng), draw(rng)};
    const Abc offset{draw(rng), draw(rng), draw(rng)};
    const Abc v2 = v1 + offset;
    EXPECT_EQ(offset.height(), edgeDist(v1, v2));
  }
}

TEST(AbcTest, StraightLineEdgeDistIsTwiceHexDist)
{
  std::mt19937_64 rng = seeded();
  std::uniform_int_distribution<int> draw = spread();
  std::uniform_int_distribution<int> small(-6, 6);
  for (int i = 0; i < kIterations; ++i) {
    const Qrs w1{draw(rng), draw(rng), draw(rng)};
    const int mult = small(rng);
    const Qrs along[3] = {HexCoord::QVec * mult, HexCoord::RVec * mult, HexCoord::SVec * mult};
    for (const Qrs& offset : along) {
      const Qrs w2 = w1 + offset;
      const int hops = hexDist(w1, w2);
      EXPECT_EQ(offset.height(), hops);
      EXPECT_EQ(2 * hops, edgeDist(w1.toAbc(), w2.toAbc()));
    }
  }
}

TEST(AbcTest, EdgeDistBounds)
{
  std::mt19937_64 rng = seeded();
  std::uniform_int_distribution<int> draw = spread();
  for (int i = 0; i < kIterations; ++i) {
    const Qrs w1{draw(rng), draw(rng), draw(rng)};
    const Qrs w2{draw(rng), draw(rng), draw(rng)};
    const int hops = hexDist(w1, w2);
    const int edges = edgeDist(w1.toAbc(), w2.toAbc());
    EXPECT_LE(3 * hops, 2 * edges);
    EXPECT_LE(edges, 2 * hops);
  }
}

TEST(AbcTest, PrintsLikeTricoord)
{
  std::ostringstream abc;
  abc << Abc{1, 0, 0};
  EXPECT_EQ("[ABC   1,   0,   0]", abc.str());
  std::ostringstream qrs;
  qrs << HexCoord::QVec;
  EXPECT_EQ("[QRS   1,   0,   0]", qrs.str());
}
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------

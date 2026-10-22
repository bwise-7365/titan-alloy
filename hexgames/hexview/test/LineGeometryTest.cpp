// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
// The line geometry on PGG's flat grid. That every link and river of the six reference sheets is
// drawn exactly as hexsheet2svg.py draws it is MapSvgGoldenTest's job (links and edges layers).
// ----------------------------------------------
#include "TestSupport.h"
#include "hexview/LineGeometry.h"

#include <gtest/gtest.h>

#include <cmath>

namespace {

  using namespace HexView;
  using HexCoord::Direction;

  const MapFrame&
  pgg()
  {
    static const HexXml::SheetDoc sheet = HexViewTest::loadSheet("panzergruppe-guderian");
    static const MapFrame frame = MapFrame::of(sheet);
    return frame;
  }

  HexId
  step(const HexId& id, Direction d)
  {
    const HexCoord::Grid& g = pgg().gridOf(id);
    return g.idOf(*g.indexOf(g.centreOf(*g.find(id)).neighbour(d)));
  }

  Pixel
  mid(Pixel a, Pixel b)
  {
    return Pixel{(a.x + b.x) / 2, (a.y + b.y) / 2};
  }

  void
  expectNear(Pixel want, Pixel got)
  {
    EXPECT_NEAR(want.x, got.x, 1e-9);
    EXPECT_NEAR(want.y, got.y, 1e-9);
  }

  bool
  samePointP(Pixel a, Pixel b)
  {
    return std::hypot(a.x - b.x, a.y - b.y) < 1e-6;
  }

  TEST(LineGeometryTest, SmoothedLinksStraightThroughPassesTheCentre)
  {
    const HexId a{"1010"};
    const HexId b = step(a, Direction::D3);  // flat: south
    const HexId c = step(b, Direction::D3);
    const std::vector<Polyline> lines = smoothedLinks({{a, b, c}}, pgg());
    ASSERT_EQ(1u, lines.size());
    const Polyline& p = lines[0];
    ASSERT_EQ(4u, p.size());
    expectNear(pgg().centre(a), p[0]);
    expectNear(mid(pgg().centre(a), pgg().centre(b)), p[1]);
    expectNear(mid(pgg().centre(b), pgg().centre(c)), p[2]);
    expectNear(pgg().centre(c), p[3]);
    // p[1] -> p[2] passes through b's centre
    expectNear(pgg().centre(b), mid(p[1], p[2]));
  }

  TEST(LineGeometryTest, SmoothedLinksCutTheCornerAtABend)
  {
    const HexId a{"1010"};
    const HexId b = step(a, Direction::D3);
    const HexId c = step(b, Direction::D2);  // a 60-degree bend at b
    const Polyline p = smoothedLinks({{a, b, c}}, pgg()).at(0);
    ASSERT_EQ(4u, p.size());
    for (const Pixel& q : p) {
      EXPECT_FALSE(samePointP(pgg().centre(b), q));
    }
    expectNear(mid(pgg().centre(b), pgg().centre(c)), p[2]);
  }

  TEST(LineGeometryTest, EndsAndJunctionsMeetAtCentres)
  {
    const HexId j{"1010"};
    const HexId n = step(j, Direction::D0);
    const HexId s = step(j, Direction::D3);
    const HexId e = step(j, Direction::D2);
    const std::vector<Polyline> lines = smoothedLinks({{n, j, s}, {j, e}}, pgg());
    ASSERT_EQ(3u, lines.size());  // j has three neighbours: every branch ends at its centre
    for (const Polyline& p : lines) {
      EXPECT_TRUE(samePointP(pgg().centre(j), p.front()) || samePointP(pgg().centre(j), p.back()));
      ASSERT_EQ(3u, p.size());
    }
  }

  TEST(LineGeometryTest, ExplicitChainsCrossWithoutJoining)
  {
    const HexId j{"1010"};
    const HexId n = step(j, Direction::D0);
    const HexId s = step(j, Direction::D3);
    const HexId ne = step(j, Direction::D1);
    const HexId sw = step(j, Direction::D4);
    const std::vector<std::vector<LinkNode>> chains{{{n, "a:n"}, {j, "a:j"}, {s, "a:s"}},
                                                    {{ne, "b:ne"}, {j, "b:j"}, {sw, "b:sw"}}};
    const std::vector<Polyline> lines = smoothedLinks(chains, pgg());
    ASSERT_EQ(2u, lines.size());
    EXPECT_EQ(4u, lines[0].size());  // each passes through j midpoint to midpoint
    EXPECT_EQ(4u, lines[1].size());
  }

  TEST(LineGeometryTest, PassThroughLoopCloses)
  {
    const HexId c{"1010"};
    std::vector<HexId> ring;
    for (int k = 0; k < 6; ++k) {
      ring.push_back(step(c, static_cast<Direction>(k)));
    }
    ring.push_back(ring.front());
    const std::vector<Polyline> lines = smoothedLinks({ring}, pgg());
    ASSERT_EQ(1u, lines.size());
    ASSERT_EQ(7u, lines[0].size());  // six midpoints, the first repeated
    expectNear(lines[0].front(), lines[0].back());
  }

  TEST(LineGeometryTest, CornerChainsMatchCornersDespiteFloatNoise)
  {
    // A river along a's south-east side and then b's south side, b being a's south-east neighbour
    // seen from the other hex: the shared corner is computed from two different centres.
    const HexId a{"1010"};
    const HexId b = step(a, Direction::D2);
    const auto s1 = pgg().hexsideEnds(EdgeRef{a, Direction::D3});  // a's south side
    const auto s2 = pgg().hexsideEnds(EdgeRef{b, Direction::D4});  // b's south-west side
    const auto s3 = pgg().hexsideEnds(EdgeRef{b, Direction::D3});  // b's south side
    const std::vector<Polyline> chains = cornerChains({s1, s2, s3});
    ASSERT_EQ(1u, chains.size());
    EXPECT_EQ(4u, chains[0].size());
  }

  TEST(LineGeometryTest, RoundedCornersUseQuadraticsThroughMidpoints)
  {
    const Polyline chain{{0, 0}, {10, 0}, {10, 10}, {20, 10}};
    const std::vector<PathCommand> d = roundedCorners(chain);
    ASSERT_EQ(5u, d.size());
    expectNear({0, 0}, std::get<MoveTo>(d[0]).to);
    expectNear({5, 0}, std::get<LineTo>(d[1]).to);
    expectNear({10, 0}, std::get<QuadTo>(d[2]).control);
    expectNear({10, 5}, std::get<QuadTo>(d[2]).to);
    expectNear({10, 10}, std::get<QuadTo>(d[3]).control);
    expectNear({15, 10}, std::get<QuadTo>(d[3]).to);
    expectNear({20, 10}, std::get<LineTo>(d[4]).to);
    // two corners: a single side stays straight
    EXPECT_EQ(2u, roundedCorners({{0, 0}, {10, 0}}).size());
  }

  TEST(LineGeometryTest, RoundedClosedChainRoundsItsStart)
  {
    const Polyline loop{{0, 0}, {10, 0}, {10, 10}, {0, 10}, {0, 0}};
    const std::vector<PathCommand> d = roundedCorners(loop);
    ASSERT_EQ(5u, d.size());
    expectNear({0, 5}, std::get<MoveTo>(d[0]).to);  // between the last corner and the first
    expectNear({0, 0}, std::get<QuadTo>(d[1]).control);
    expectNear({0, 5}, std::get<QuadTo>(d[4]).to);
  }

  TEST(LineGeometryTest, BoundariesStayStraight)
  {
    const Polyline chain{{0, 0}, {10, 0}, {10, 10}};
    const std::vector<PathCommand> d = straightChain(chain);
    ASSERT_EQ(3u, d.size());
    expectNear({10, 0}, std::get<LineTo>(d[1]).to);
    expectNear({10, 10}, std::get<LineTo>(d[2]).to);
  }

}  // namespace
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------

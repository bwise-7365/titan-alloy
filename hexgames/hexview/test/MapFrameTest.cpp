// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
// The reference geometry is hexsheet2svg.py's own Grid, dumped at test time by svg-golden.py
// ("centres"): every printed id with its centre and its six corners in the reference's order.
// ----------------------------------------------
#include "TestSupport.h"
#include "hexview/MapFrame.h"

#include <gtest/gtest.h>

#include <cmath>
#include <sstream>

namespace {

  using namespace HexViewTest;
  using HexView::HexId;
  using HexView::MapFrame;

  struct RefHex {
    std::string id;
    double v[14];
  };

  std::vector<RefHex>
  referenceGeometry(const std::string& name)
  {
    const std::filesystem::path out = outDir() / (name + ".centres.txt");
    if (0 != runReferenceScript({"centres", sheetPath(name).string(), out.string()})) {
      throw std::runtime_error("svg-golden.py centres failed for " + name);
    }
    std::istringstream in(readFile(out));
    std::vector<RefHex> hexes;
    RefHex h;
    while (in >> h.id) {
      for (double& x : h.v) {
        in >> x;
      }
      hexes.push_back(h);
    }
    return hexes;
  }

  void
  expectGeometryMatches(const std::string& name)
  {
    const MapFrame frame = MapFrame::of(loadSheet(name));
    const std::vector<RefHex> ref = referenceGeometry(name);
    ASSERT_FALSE(ref.empty());
    std::size_t printed = 0;
    for (const HexCoord::Grid& g : frame.grids()) {
      printed += g.ids().size();
    }
    EXPECT_EQ(ref.size(), printed);
    for (const RefHex& h : ref) {
      const HexView::Pixel c = frame.centre(HexId{h.id});
      ASSERT_NEAR(h.v[0], c.x, 0.01) << name << " " << h.id;
      ASSERT_NEAR(h.v[1], c.y, 0.01) << name << " " << h.id;
      const auto corners = frame.corners(HexId{h.id});
      for (std::size_t k = 0; k < 6; ++k) {
        ASSERT_NEAR(h.v[2 + 2 * k], corners[k].x, 0.01) << name << " " << h.id << " corner " << k;
        ASSERT_NEAR(h.v[3 + 2 * k], corners[k].y, 0.01) << name << " " << h.id << " corner " << k;
      }
    }
    return;
  }

  TEST(MapFrameTest, CentresAndCornersMatchReferenceOnSixSheets)
  {
    for (const char* name : {"the-russian-campaign", "panzergruppe-guderian", "d-day-at-tarawa",
                             "dai-senso", "stalin-moves-west", "velikiye-luki"}) {
      expectGeometryMatches(name);
    }
  }

  TEST(MapFrameTest, HexsideEndsAreTheCornersEitherSideOfTheSide)
  {
    const MapFrame frame = MapFrame::of(loadSheet("panzergruppe-guderian"));  // flat
    const auto corners = frame.corners(HexId{"1010"});
    // flat corners: e se sw w nw ne; the north side runs nw -> ne
    const auto [a, b] = frame.hexsideEnds(frame.edgeRef("1010:n"));
    EXPECT_NEAR(corners[4].x, a.x, 1e-9);
    EXPECT_NEAR(corners[5].x, b.x, 1e-9);
    const HexView::Pixel mid = frame.hexsideMidpoint(frame.edgeRef("1010:n"));
    EXPECT_NEAR((a.x + b.x) / 2, mid.x, 1e-6);
    EXPECT_NEAR((a.y + b.y) / 2, mid.y, 1e-6);
  }

  TEST(MapFrameTest, EdgeRefParsesAndRefusesBadDirection)
  {
    const MapFrame frame = MapFrame::of(loadSheet("the-russian-campaign"));  // pointy
    const HexView::EdgeRef e = frame.edgeRef("KK20:e");
    EXPECT_EQ("KK20", e.hex.text);
    EXPECT_EQ(HexCoord::Direction::D1, e.dir);
    for (const char* bad : {"KK20:n", "KK20", "ZZ99:e", ":e"}) {
      try {
        frame.edgeRef(bad);
        FAIL() << "accepted " << bad;
      }
      catch (const std::invalid_argument& ex) {
        EXPECT_NE(std::string::npos, std::string(ex.what()).find(std::string(bad).substr(0, 4)))
            << ex.what();
      }
    }
  }

  TEST(MapFrameTest, HexAtInvertsCentre)
  {
    const MapFrame frame = MapFrame::of(loadSheet("d-day-at-tarawa"));
    const double size = frame.grids().front().spec().size;
    int k = 0;
    for (const HexId& id : frame.grids().front().ids()) {
      const HexView::Pixel c = frame.centre(id);
      const double jx = 0.5 * size * std::sin(1.7 * k);
      const double jy = 0.5 * size * std::cos(2.3 * k);
      ++k;
      const std::optional<HexId> found = frame.hexAt({c.x + jx, c.y + jy});
      ASSERT_TRUE(found.has_value()) << id.text;
      EXPECT_EQ(id.text, found->text);
    }
    EXPECT_FALSE(frame.hexAt({-1000, -1000}).has_value());
  }

  TEST(MapFrameTest, TwoGridSheetSharesOneLatticeAndNumbersHexesGridByGrid)
  {
    const MapFrame frame = MapFrame::of(loadSheet("dai-senso"));
    ASSERT_EQ(2u, frame.grids().size());
    const std::size_t west = frame.grids()[0].ids().size();
    EXPECT_EQ(0u, frame.index(frame.grids()[0].ids().front()).value);
    EXPECT_EQ(west, frame.index(frame.grids()[1].ids().front()).value);
    EXPECT_THROW(frame.index(HexId{"nowhere"}), std::invalid_argument);
  }

}  // namespace
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------

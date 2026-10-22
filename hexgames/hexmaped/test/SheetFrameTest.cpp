// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
#include "hexmaped/SheetFrame.h"
#include "hexxml/XmlDocument.h"

#include <gtest/gtest.h>

#include <cmath>
#include <numbers>

namespace {

  HexXml::SheetDoc
  sheet(const char* name)
  {
    return HexXml::SheetDoc::parse(
      HexXml::XmlDocument::load(std::string(HEXGAMES_SOURCE_DIR) + "/map_graphics/xml/" + name));
  }

  double
  angleOf(HexMapEd::Pixel from, HexMapEd::Pixel to)
  {
    const double a = std::atan2(to.y - from.y, to.x - from.x) * 180.0 / std::numbers::pi;
    return a < 0.0 ? a + 360.0 : a;
  }

  double
  gap(double a, double b)
  {
    const double d = std::fabs(a - b);
    return d > 180.0 ? 360.0 - d : d;
  }

}  // namespace

TEST(SheetFrameTest, EveryHexsideFacesItsDirection)
{
  for (const char* name : {"stalin-moves-west.xml", "panzergruderian.xml"}) {
    if (std::string(name) == "panzergruderian.xml") {
      continue;  // placeholder kept out: the sheet list below is the real one
    }
  }
  for (const char* name : {"stalin-moves-west.xml", "panzergruppe-guderian.xml"}) {
    const HexMapEd::SheetFrame frame = HexMapEd::SheetFrame::of(sheet(name));
    int checked = 0;
    for (const std::string& id : frame.ids()) {
      const HexMapEd::Pixel c = frame.centre(id);
      for (int k = 0; k < HexCoord::kDirections; ++k) {
        const auto d = static_cast<HexCoord::Direction>(k);
        const HexMapEd::Pixel mid = frame.hexsideMidpoint(HexMapEd::Hexside{id, d});
        const double want = HexCoord::edgeAngleDegrees(d, frame.orientation());
        EXPECT_LT(gap(angleOf(c, mid), want), 1.0) << name << " " << id << " dir " << k;
        ++checked;
      }
      if (checked > 600) {
        break;
      }
    }
  }
}

TEST(SheetFrameTest, NeighboursShareTheHexside)
{
  const HexMapEd::SheetFrame frame = HexMapEd::SheetFrame::of(sheet("stalin-moves-west.xml"));
  int shared = 0;
  for (const std::string& id : frame.ids()) {
    for (int k = 0; k < HexCoord::kDirections; ++k) {
      const auto d = static_cast<HexCoord::Direction>(k);
      const std::optional<std::string> other = frame.neighbour(id, d);
      if (!other.has_value()) {
        continue;
      }
      const HexMapEd::Pixel mine = frame.hexsideMidpoint(HexMapEd::Hexside{id, d});
      const HexMapEd::Pixel theirs = frame.hexsideMidpoint(HexMapEd::Hexside{*other, HexCoord::opposite(d)});
      EXPECT_NEAR(mine.x, theirs.x, 0.01) << id;
      EXPECT_NEAR(mine.y, theirs.y, 0.01) << id;
      EXPECT_EQ(frame.canonical(HexMapEd::Hexside{id, d}), frame.canonical(HexMapEd::Hexside{*other, HexCoord::opposite(d)}));
      ++shared;
    }
    if (shared > 300) {
      break;
    }
  }
  EXPECT_GT(shared, 0);
}

TEST(SheetFrameTest, TokensRoundTripAndBadOnesThrow)
{
  const HexMapEd::SheetFrame frame = HexMapEd::SheetFrame::of(sheet("stalin-moves-west.xml"));
  const HexMapEd::Hexside side = frame.hexside("1028:ne");
  EXPECT_EQ(frame.token(side), "1028:ne");
  EXPECT_THROW(frame.hexside("1028"), std::invalid_argument);
  EXPECT_THROW(frame.hexside("1028:n"), std::invalid_argument);  // pointy hexes have no n side
  EXPECT_THROW(frame.centre("9999"), std::invalid_argument);
}

TEST(SheetFrameTest, HexAtInvertsCentreAndHexsideAtFindsTheNearSide)
{
  const HexMapEd::SheetFrame frame = HexMapEd::SheetFrame::of(sheet("stalin-moves-west.xml"));
  int checked = 0;
  for (const std::string& id : frame.ids()) {
    const HexMapEd::Pixel c = frame.centre(id);
    EXPECT_EQ(frame.hexAt(HexMapEd::Pixel{c.x + 3.0, c.y - 2.0}), id);
    const HexMapEd::Hexside side{id, HexCoord::Direction::D2};
    const HexMapEd::Pixel mid = frame.hexsideMidpoint(side);
    const HexMapEd::Pixel inside{c.x + 0.9 * (mid.x - c.x), c.y + 0.9 * (mid.y - c.y)};
    const std::optional<HexMapEd::Hexside> found = frame.hexsideAt(inside, 8.0);
    ASSERT_TRUE(found.has_value()) << id;
    EXPECT_EQ(found->hex, id);
    EXPECT_EQ(found->dir, HexCoord::Direction::D2);
    if (++checked > 50) {
      break;
    }
  }
  EXPECT_FALSE(frame.hexAt(HexMapEd::Pixel{-1000.0, -1000.0}).has_value());
}
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------

// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
#include "hexmaped/Document.h"

#include <gtest/gtest.h>

#include <cmath>
#include <filesystem>

namespace {

  HexMapEd::Document
  smw()
  {
    return HexMapEd::Document::load(std::filesystem::path(HEXGAMES_SOURCE_DIR) / "map_graphics" / "xml" / "stalin-moves-west.xml");
  }

}  // namespace

TEST(DocumentTest, LoadsAndAnswersTerrain)
{
  const HexMapEd::Document doc = smw();
  EXPECT_EQ(doc.terrainOf("1428"), "rough");
  EXPECT_EQ(doc.terrainOf("2028"), "clear");
  EXPECT_FALSE(doc.dirtyP());
  EXPECT_FALSE(doc.canUndoP());
}

TEST(DocumentTest, SetTerrainMovesTheHexAndUndoRestoresIt)
{
  HexMapEd::Document doc = smw();
  doc.setTerrain("2028", "rough");
  EXPECT_EQ(doc.terrainOf("2028"), "rough");
  EXPECT_TRUE(doc.dirtyP());
  doc.setTerrain("2028", "clear");
  EXPECT_EQ(doc.terrainOf("2028"), "clear");
  for (const HexXml::SheetHexesDoc& bulk : doc.sheet().hexesBulk) {
    EXPECT_EQ(std::count(bulk.ids.begin(), bulk.ids.end(), "2028"), 0) << bulk.terrain;
  }
  doc.undo();
  EXPECT_EQ(doc.terrainOf("2028"), "rough");
  doc.undo();
  EXPECT_EQ(doc.terrainOf("2028"), "clear");
  doc.redo();
  EXPECT_EQ(doc.terrainOf("2028"), "rough");
}

TEST(DocumentTest, RefusesWhatTheSheetDoesNotDeclare)
{
  HexMapEd::Document doc = smw();
  EXPECT_THROW(doc.setTerrain("2028", "desert"), std::invalid_argument);
  EXPECT_THROW(doc.setTerrain("9999", "rough"), std::invalid_argument);
  EXPECT_THROW(doc.toggleEdge(doc.frame().hexside("2028:e"), "canal"), std::invalid_argument);
  EXPECT_THROW(doc.addGlyph("2028", "castle", "c", std::nullopt), std::invalid_argument);
  EXPECT_THROW(doc.addGlyph("2028", "city", "centre", std::nullopt), std::invalid_argument);
  EXPECT_THROW(doc.addGlyph("2028", "city", "c", std::string("mauve")), std::invalid_argument);
  EXPECT_THROW(doc.addLinkStep("2028", "1030", "rail", "rail"), std::invalid_argument);  // not neighbours
  EXPECT_FALSE(doc.dirtyP());
}

TEST(DocumentTest, ToggleEdgeAddsThenRemovesOnEitherSpelling)
{
  HexMapEd::Document doc = smw();
  const HexMapEd::Hexside side = doc.frame().hexside("2028:e");
  ASSERT_FALSE(doc.lineOfEdge(side).has_value());
  doc.toggleEdge(side, "river");
  EXPECT_EQ(doc.lineOfEdge(side), "river");
  const std::optional<std::string> other = doc.frame().neighbour("2028", side.dir);
  ASSERT_TRUE(other.has_value());
  const HexMapEd::Hexside theirs{*other, HexCoord::opposite(side.dir)};
  EXPECT_EQ(doc.lineOfEdge(theirs), "river");
  doc.toggleEdge(theirs, "river");
  EXPECT_FALSE(doc.lineOfEdge(side).has_value());
}

TEST(DocumentTest, LinkStepsExtendSplitAndDropShortChains)
{
  HexMapEd::Document doc = smw();
  const std::size_t before = doc.sheet().links.size();
  const std::optional<std::string> b = doc.frame().neighbour("2028", HexCoord::Direction::D0);
  ASSERT_TRUE(b.has_value());
  const std::optional<std::string> c = doc.frame().neighbour(*b, HexCoord::Direction::D0);
  ASSERT_TRUE(c.has_value());
  doc.addLinkStep("2028", *b, "road", "road");
  EXPECT_EQ(doc.sheet().links.size(), before + 1);
  doc.addLinkStep(*b, *c, "road", "road");
  EXPECT_EQ(doc.sheet().links.size(), before + 1);
  EXPECT_EQ(doc.sheet().links.back().hexes.size(), 3u);
  doc.removeLinkStep(*b, *c);
  EXPECT_EQ(doc.sheet().links.back().hexes.size(), 2u);
  doc.removeLinkStep("2028", *b);
  EXPECT_EQ(doc.sheet().links.size(), before);
}

TEST(DocumentTest, GlyphsNamesRingsAndClip)
{
  HexMapEd::Document doc = smw();
  doc.addGlyph("2028", "town", "n", std::nullopt);
  ASSERT_NE(doc.hexElement("2028"), nullptr);
  EXPECT_EQ(doc.hexElement("2028")->glyphs.size(), 1u);
  doc.setName("2028", std::string("Somewhere"));
  EXPECT_EQ(doc.hexElement("2028")->name, "Somewhere");
  doc.setRing("2028", std::string("ink"));
  EXPECT_EQ(doc.hexElement("2028")->ring, "ink");
  doc.removeGlyph("2028", 0);
  EXPECT_TRUE(doc.hexElement("2028")->glyphs.empty());
  EXPECT_TRUE(doc.frame().printsP("2028"));
  doc.toggleClip("2028");
  EXPECT_FALSE(doc.frame().printsP("2028"));
  doc.toggleClip("2028");
  EXPECT_TRUE(doc.frame().printsP("2028"));
}

TEST(DocumentTest, AddHexAtRestoresAClippedCellAndGrowsTheGrid)
{
  HexMapEd::Document doc = smw();
  const HexMapEd::Pixel c = doc.frame().centre("2028");
  doc.toggleClip("2028");
  ASSERT_FALSE(doc.frame().printsP("2028"));
  EXPECT_EQ(doc.cellAt(c), "2028");
  EXPECT_EQ(doc.addHexAt(c), "2028");
  EXPECT_TRUE(doc.frame().printsP("2028"));
  EXPECT_EQ(doc.addHexAt(c), "2028");  // already printed: no edit
  // grow by one column to the left of the whole grid
  const HexXml::SheetGridDoc before = doc.sheet().grids.front();
  const HexCoord::Grid& g = doc.frame().grids().front();
  const HexMapEd::Pixel first = g.pixelOf(g.centreOf(HexCoord::GridIndex{0, 0}));
  const double colPitch = ("flat" == before.orientation ? 1.5 : std::sqrt(3.0)) * before.size;
  const HexMapEd::Pixel outside{first.x - colPitch, first.y};
  const std::size_t printedBefore = doc.frame().ids().size();
  const std::string added = doc.addHexAt(outside);
  const HexXml::SheetGridDoc after = doc.sheet().grids.front();
  EXPECT_EQ(after.cols, before.cols + 1);
  EXPECT_EQ(after.colStart, before.colStart - before.colStep);
  EXPECT_EQ(doc.frame().ids().size(), printedBefore + 1);
  EXPECT_TRUE(doc.frame().printsP(added));
  EXPECT_NEAR(doc.frame().centre("2028").x, c.x, 0.01);  // every old hex keeps its place and name
  EXPECT_NEAR(doc.frame().centre("2028").y, c.y, 0.01);
  EXPECT_NEAR(doc.frame().centre(added).x, outside.x, 0.01);
  EXPECT_NEAR(doc.frame().centre(added).y, outside.y, 0.01);
  doc.undo();
  EXPECT_EQ(doc.sheet().grids.front().cols, before.cols);
}

TEST(DocumentTest, SaveWritesWhatLoadsBack)
{
  HexMapEd::Document doc = smw();
  doc.setTerrain("2028", "rough");
  const std::filesystem::path tmp = std::filesystem::temp_directory_path() / "hexmaped-document-save.xml";
  doc.saveAs(tmp);
  EXPECT_FALSE(doc.dirtyP());
  const HexMapEd::Document back = HexMapEd::Document::load(tmp);
  EXPECT_EQ(back.terrainOf("2028"), "rough");
  std::filesystem::remove(tmp);
}
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------

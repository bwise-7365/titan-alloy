// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
#include "hexmaped/Document.h"

#include <gtest/gtest.h>

#include <algorithm>
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

TEST(DocumentTest, LinkStepsExtendSplitAndKeepOneHexRemnants)
{
  HexMapEd::Document doc = smw();
  const std::size_t before = doc.sheet().links.size();
  const std::optional<std::string> b = doc.frame().neighbour("2028", HexCoord::Direction::D0);
  ASSERT_TRUE(b.has_value());
  const std::optional<std::string> c = doc.frame().neighbour(*b, HexCoord::Direction::D0);
  ASSERT_TRUE(c.has_value());
  doc.addLinkStep("2028", *b, "spur", "rail");
  EXPECT_EQ(doc.sheet().links.size(), before + 1);
  EXPECT_EQ(doc.sheet().links.back().id, "spur-new-1");
  doc.addLinkStep(*b, *c, "spur", "rail");
  EXPECT_EQ(doc.sheet().links.size(), before + 1);
  EXPECT_EQ(doc.sheet().links.back().hexes.size(), 3u);
  doc.removeLinkStep(*b, *c);
  EXPECT_EQ(doc.sheet().links.size(), before + 2);
  EXPECT_EQ(doc.sheet().links.back().hexes, std::vector<std::string>{*c});
  EXPECT_EQ(doc.sheet().links.back().id, "spur-new-1-2");
  doc.removeLinkStep("2028", *b);
  EXPECT_EQ(doc.sheet().links.size(), before + 3);
}

TEST(DocumentTest, NewLinksNeedAKindThatCanStartAnId)
{
  HexMapEd::Document doc = smw();
  const std::optional<std::string> b = doc.frame().neighbour("2028", HexCoord::Direction::D0);
  ASSERT_TRUE(b.has_value());
  EXPECT_THROW(doc.addLinkStep("2028", *b, "main rail", "rail"), std::invalid_argument);
  EXPECT_THROW(doc.addLinkStep("2028", *b, "2nd", "rail"), std::invalid_argument);
  EXPECT_FALSE(doc.dirtyP());
}

TEST(DocumentTest, OneHexLinksCanBeDeletedUnlessAJunctionNamesThem)
{
  HexMapEd::Document doc = smw();
  const std::size_t before = doc.sheet().links.size();
  const std::optional<std::string> b = doc.frame().neighbour("2028", HexCoord::Direction::D0);
  ASSERT_TRUE(b.has_value());
  doc.addLinkStep("2028", *b, "spur", "rail");
  doc.removeLinkStep("2028", *b);  // leaves two one-hex links
  EXPECT_EQ(doc.sheet().links.size(), before + 2);
  doc.removeOneHexLinks("2028");
  doc.removeOneHexLinks(*b);
  EXPECT_EQ(doc.sheet().links.size(), before);
  EXPECT_THROW(doc.removeOneHexLinks("2028"), std::invalid_argument);  // nothing left there
  EXPECT_THROW(doc.removeOneHexLinks("2345"), std::invalid_argument);  // rl-smolensk-e: the junction names it
  EXPECT_EQ(doc.sheet().links.size(), before);
}

TEST(DocumentTest, JunctionModeTogglesAJunctionOfOneKind)
{
  HexMapEd::Document doc = smw();
  const auto at = [&](const std::string& hex) {
    return std::count_if(doc.sheet().junctionElements.begin(), doc.sheet().junctionElements.end(),
                         [&](const HexXml::SheetJunctionDoc& j) { return j.at == hex; });
  };
  ASSERT_EQ(1, at("2345"));  // Smolensk: four rail links meet
  doc.toggleJunction("2345", "rail");
  EXPECT_EQ(0, at("2345"));
  doc.toggleJunction("2345", "rail");
  EXPECT_EQ(1, at("2345"));
  EXPECT_THROW(doc.toggleJunction("2028", "rail"), std::invalid_argument);  // no two rail links there
  HexMapEd::Document pgg = HexMapEd::Document::load(std::filesystem::path(HEXGAMES_SOURCE_DIR) / "map_graphics" /
                                                    "xml" / "panzergruppe-guderian.xml");
  EXPECT_THROW(pgg.toggleJunction("0412", "rail"), std::invalid_argument);  // an implicit sheet
}

TEST(DocumentTest, ReaddingACutStepAbsorbsTheRemnantAndItsJunction)
{
  HexMapEd::Document doc = smw();
  const auto odessa = [&] {
    return std::find_if(doc.sheet().junctionElements.begin(), doc.sheet().junctionElements.end(),
                        [](const HexXml::SheetJunctionDoc& j) { return "0945" == j.at; })->links;
  };
  const std::vector<std::string> before = odessa();
  const std::size_t links = doc.sheet().links.size();
  doc.removeLinkStep("1044", "0945");  // rl-kiev-odessa loses its last step; 0945 is left as a remnant
  EXPECT_EQ(links + 1, doc.sheet().links.size());
  doc.addLinkStep("1044", "0945", "rail", "rail");  // the step back: the chain absorbs the remnant
  EXPECT_EQ(links, doc.sheet().links.size());
  EXPECT_EQ(before, odessa());
  const auto kievOdessa = std::find_if(doc.sheet().links.begin(), doc.sheet().links.end(),
                                       [](const HexXml::SheetLinkDoc& l) { return l.id == "rl-kiev-odessa"; });
  ASSERT_NE(kievOdessa, doc.sheet().links.end());
  EXPECT_EQ(kievOdessa->ends, "place place");  // the cut's "unexplained" is undone
  // a map exit is a one-hex link too, but a real one: extending past it leaves it alone
  const auto exitP = [&] {
    return std::any_of(doc.sheet().links.begin(), doc.sheet().links.end(),
                       [](const HexXml::SheetLinkDoc& l) { return l.id == "rl-odessa-e"; });
  };
  EXPECT_TRUE(exitP());
}

TEST(DocumentTest, CuttingALinkRenamesTheTailAndRepointsItsJunctions)
{
  HexMapEd::Document doc = smw();
  // rl-kiev-odessa: 1644 1544 1443 1344 1244 1145 1044 0945; junction at 1145 with rl-odessa-e
  doc.removeLinkStep("1344", "1244");
  const HexXml::SheetDoc& s = doc.sheet();
  const auto byId = [&](const std::string& id) {
    return std::find_if(s.links.begin(), s.links.end(), [&](const HexXml::SheetLinkDoc& l) { return l.id == id; });
  };
  ASSERT_NE(byId("rl-kiev-odessa"), s.links.end());
  ASSERT_NE(byId("rl-kiev-odessa-2"), s.links.end());
  EXPECT_EQ(byId("rl-kiev-odessa")->hexes.back(), "1344");
  EXPECT_EQ(byId("rl-kiev-odessa")->ends, "place unexplained");
  EXPECT_EQ(byId("rl-kiev-odessa-2")->hexes.front(), "1244");
  EXPECT_EQ(byId("rl-kiev-odessa-2")->ends, "unexplained place");
  const auto j = std::find_if(s.junctionElements.begin(), s.junctionElements.end(),
                              [](const HexXml::SheetJunctionDoc& x) { return "1145" == x.at; });
  ASSERT_NE(j, s.junctionElements.end());
  EXPECT_EQ(j->links, (std::vector<std::string>{"rl-kiev-odessa-2", "rl-odessa-e"}));
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

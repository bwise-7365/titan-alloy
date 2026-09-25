// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
#include "hexxml/SheetDoc.h"

#include <gtest/gtest.h>

#include <algorithm>
#include <filesystem>
#include <stdexcept>

namespace {

  std::filesystem::path
  mapGraphics(const char* name)
  {
    return std::filesystem::path(HEXGAMES_SOURCE_DIR) / "map_graphics" / "xml" / name;
  }

}  // namespace

TEST(SheetDocTest, TrcParsesWithExpectedShape)
{
  const HexXml::XmlDocument xml = HexXml::XmlDocument::load(mapGraphics("the-russian-campaign.xml"));
  const HexXml::SheetDoc doc = HexXml::SheetDoc::parse(xml);

  EXPECT_EQ("trc", doc.id);
  ASSERT_EQ(1u, doc.grids.size());
  EXPECT_EQ("pointy", doc.grids[0].orientation);
  EXPECT_EQ(33, doc.grids[0].cols);
  EXPECT_EQ(43, doc.grids[0].rows);

  EXPECT_FALSE(doc.palette.empty());
  EXPECT_FALSE(doc.terrains.empty());
  EXPECT_FALSE(doc.lines.empty());

  EXPECT_FALSE(doc.hexesBulk.empty());
  EXPECT_FALSE(doc.hexes.empty());  // Moscow, cities and a few other named hexes are per-hex elements
  EXPECT_FALSE(doc.edges.empty());
  EXPECT_EQ(39u, doc.links.size());  // rail chains written by map_graphics/xml/tools/tidy_networks.py
  EXPECT_TRUE(doc.regions.empty());
  ASSERT_EQ(12u, doc.panels.size());

  const auto panelIt = std::find_if(doc.panels.begin(), doc.panels.end(),
                                     [](const HexXml::SheetPanelDoc& p) { return "axis-pool" == p.id; });
  ASSERT_NE(doc.panels.end(), panelIt);
}

TEST(SheetDocTest, DaiSensoHasTwoGridsOnOneLattice)
{
  const HexXml::XmlDocument xml = HexXml::XmlDocument::load(mapGraphics("dai-senso.xml"));
  const HexXml::SheetDoc doc = HexXml::SheetDoc::parse(xml);
  ASSERT_EQ(2u, doc.grids.size());
  EXPECT_EQ("west", doc.grids[0].id.value());
  EXPECT_EQ("east", doc.grids[1].id.value());
}

TEST(SheetDocTest, FourSheetsParse)
{
  for (const char* name : {"the-russian-campaign.xml", "dai-senso.xml", "d-day-at-tarawa.xml",
                            "panzergruppe-guderian.xml"}) {
    const HexXml::XmlDocument xml = HexXml::XmlDocument::load(mapGraphics(name));
    EXPECT_NO_THROW((void)HexXml::SheetDoc::parse(xml)) << name;
  }
}

TEST(SheetDocTest, VelikiyeLukiHasExplicitJunctions)
{
  const HexXml::SheetDoc doc = HexXml::SheetDoc::parse(HexXml::XmlDocument::load(mapGraphics("velikiye-luki.xml")));
  EXPECT_EQ("explicit", doc.junctions);
  EXPECT_LE(16u, doc.junctionElements.size());
  ASSERT_EQ(4u, doc.legend.size());  // lake, german-objective, russian-objective, bridge
  EXPECT_EQ("bridge", doc.legend[3].id);
  EXPECT_EQ("bar", doc.legend[3].shape);
  EXPECT_EQ("edge", doc.legend[3].across.value_or(""));
  for (const HexXml::SheetLinkDoc& l : doc.links) {
    EXPECT_TRUE(l.id.has_value()) << l.name.value_or("(unnamed)");
  }
  for (const HexXml::SheetJunctionDoc& j : doc.junctionElements) {
    EXPECT_LE(2u, j.links.size()) << j.at;
    EXPECT_TRUE(j.paths.empty()) << j.at;
  }
}

TEST(SheetDocTest, ImplicitSheetWithJunctionThrows)
{
  const std::filesystem::path bad =
      std::filesystem::path(HEXGAMES_SOURCE_DIR) / "hexxml" / "test" / "sheet-implicit-junction.xml";
  EXPECT_THROW((void)HexXml::SheetDoc::parse(HexXml::XmlDocument::load(bad)), std::invalid_argument);
}

TEST(SheetDocTest, BadEnumThrows)
{
  const std::filesystem::path bad =
      std::filesystem::path(HEXGAMES_SOURCE_DIR) / "hexxml" / "test" / "sheet-bad-orientation.xml";
  const HexXml::XmlDocument xml = HexXml::XmlDocument::load(bad);
  EXPECT_THROW((void)HexXml::SheetDoc::parse(xml), std::invalid_argument);
}
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------

// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
#include "hexxml/RulesDoc.h"

#include <gtest/gtest.h>

#include <algorithm>
#include <filesystem>
#include <stdexcept>

namespace {

  std::filesystem::path
  gameRules(const char* name)
  {
    return std::filesystem::path(HEXGAMES_SOURCE_DIR) / "game_rules" / "xml" / name;
  }

}  // namespace

TEST(RulesDocTest, TrcParsesWithExpectedShape)
{
  const HexXml::XmlDocument xml = HexXml::XmlDocument::load(gameRules("the-russian-campaign.xml"));
  const HexXml::RulesDoc doc = HexXml::RulesDoc::parse(xml);

  EXPECT_EQ("trc", doc.id);
  EXPECT_EQ(2, doc.players);
  ASSERT_EQ(2u, doc.sides.size());
  EXPECT_EQ("axis", doc.sides[0].id);
  EXPECT_EQ("human", doc.sides[0].control);

  EXPECT_FALSE(doc.map.hexTerrain.empty());
  EXPECT_FALSE(doc.map.hexsideTerrain.empty());
  ASSERT_EQ(1u, doc.map.networks.size());
  EXPECT_EQ("rail", doc.map.networks[0].id);
  EXPECT_FALSE(doc.map.regionLayers.empty());
  EXPECT_FALSE(doc.map.spaces.empty());

  EXPECT_FALSE(doc.unitTypes.empty());

  ASSERT_FALSE(doc.phases.empty());
  EXPECT_EQ("game-turn", doc.phases[0].id);
  EXPECT_FALSE(doc.phases[0].children.empty());

  EXPECT_EQ("eliminate-excess", doc.stacking.repair);

  ASSERT_FALSE(doc.zocs.empty());
  EXPECT_EQ("zoc", doc.zocs[0].id);

  EXPECT_EQ("hexes", doc.movement.budget);
  EXPECT_FALSE(doc.movement.modes.empty());

  ASSERT_FALSE(doc.supply.traces.empty());
  EXPECT_FALSE(doc.supply.traces[0].segments.empty());

  EXPECT_TRUE(doc.combat.mandatory);
  ASSERT_FALSE(doc.combat.resolvers.empty());
  const auto crtIt =
      std::find_if(doc.combat.resolvers.begin(), doc.combat.resolvers.end(),
                    [](const HexXml::ResolverDoc& r) { return "crt" == r.id; });
  ASSERT_NE(doc.combat.resolvers.end(), crtIt);
  ASSERT_EQ(1u, crtIt->tables.size());
  EXPECT_EQ(9u, crtIt->tables[0].columns.size());
  EXPECT_EQ(6u, crtIt->tables[0].tableRows.size());
  for (const HexXml::TableRowDoc& row : crtIt->tables[0].tableRows) {
    EXPECT_EQ(9u, row.cells.size());
  }

  ASSERT_TRUE(doc.retreat.has_value());
  EXPECT_EQ("attacker", doc.retreat->routedBy);

  ASSERT_TRUE(doc.weather.has_value());
  EXPECT_EQ("rolled", doc.weather->source);

  EXPECT_FALSE(doc.victory.conditions.empty());

  // The annex is collected wherever it appears, not only at the top level.
  EXPECT_EQ(54u, doc.allRules.size());
  EXPECT_EQ(1u, doc.allTables.size());
  EXPECT_FALSE(doc.allLists.empty());
  EXPECT_FALSE(doc.allNotes.empty());
}

TEST(RulesDocTest, DaiSensoAndTarawaParse)
{
  const HexXml::XmlDocument ds = HexXml::XmlDocument::load(gameRules("dai-senso.xml"));
  const HexXml::RulesDoc dsDoc = HexXml::RulesDoc::parse(ds);
  EXPECT_EQ(39u, dsDoc.allRules.size());
  EXPECT_EQ(3, dsDoc.players);

  const HexXml::XmlDocument tarawa = HexXml::XmlDocument::load(gameRules("d-day-at-tarawa.xml"));
  const HexXml::RulesDoc tarawaDoc = HexXml::RulesDoc::parse(tarawa);
  EXPECT_EQ(35u, tarawaDoc.allRules.size());
  EXPECT_EQ(1, tarawaDoc.players);
}

TEST(RulesDocTest, MissingRequiredAttributeThrows)
{
  const std::filesystem::path bad =
      std::filesystem::path(HEXGAMES_SOURCE_DIR) / "hexxml" / "test" / "rules-missing-id.xml";
  const HexXml::XmlDocument xml = HexXml::XmlDocument::load(bad);
  EXPECT_THROW((void)HexXml::RulesDoc::parse(xml), std::invalid_argument);
}

TEST(RulesDocTest, TableCellArityMismatchThrows)
{
  const std::filesystem::path bad =
      std::filesystem::path(HEXGAMES_SOURCE_DIR) / "hexxml" / "test" / "rules-bad-table.xml";
  const HexXml::XmlDocument xml = HexXml::XmlDocument::load(bad);
  try {
    (void)HexXml::RulesDoc::parse(xml);
    FAIL() << "expected std::invalid_argument";
  } catch (const std::invalid_argument& e) {
    const std::string message = e.what();
    EXPECT_NE(std::string::npos, message.find("crt"));
  }
}
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------

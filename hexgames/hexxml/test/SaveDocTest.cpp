// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
#include "hexxml/SaveDoc.h"

#include <gtest/gtest.h>

#include <algorithm>
#include <filesystem>

namespace {

  std::filesystem::path
  gameRecords(const char* name)
  {
    return std::filesystem::path(HEXGAMES_SOURCE_DIR) / "game_records" / "xml" / name;
  }

}  // namespace

TEST(SaveDocTest, TrcTestScenarioParses)
{
  const HexXml::XmlDocument xml = HexXml::XmlDocument::load(gameRecords("trc-test.xml"));
  const HexXml::SaveDoc doc = HexXml::SaveDoc::parse(xml);

  EXPECT_EQ("hexsave-1.0", doc.format);
  EXPECT_EQ("scenario", doc.kind);
  EXPECT_EQ("trc", doc.game);
  EXPECT_EQ(20260912u, doc.seed);

  EXPECT_EQ(1, doc.cursor.turn);
  EXPECT_EQ("axis-i1-move", doc.cursor.phase);
  EXPECT_EQ("axis", doc.cursor.side.value());

  ASSERT_EQ(2u, doc.sides.size());
  EXPECT_EQ("axis", doc.sides[0].id);
  ASSERT_EQ(1u, doc.sides[0].registers.size());
  EXPECT_EQ("turn-track", doc.sides[0].registers[0].track);
  EXPECT_TRUE(doc.sides[0].flags.empty());  // the engine smoke scenario carries no flags (M6b review)

  ASSERT_EQ(11u, doc.units.size());
  const auto hq = std::find_if(doc.units.begin(), doc.units.end(),
                                [](const HexXml::SaveUnitDoc& u) { return "g-ge-n-hq" == u.id; });
  ASSERT_NE(doc.units.end(), hq);
  EXPECT_EQ("E27", hq->hex.value());
  EXPECT_EQ("axis", hq->owner);

  const auto inBox = std::find_if(doc.units.begin(), doc.units.end(),
                                   [](const HexXml::SaveUnitDoc& u) { return "g-ge-24-armour" == u.id; });
  ASSERT_NE(doc.units.end(), inBox);
  EXPECT_FALSE(inBox->hex.has_value());
  EXPECT_EQ("omb", inBox->space.value());

  ASSERT_EQ(2u, doc.controlHexes.size());
  ASSERT_EQ(2u, doc.streams.size());
  EXPECT_TRUE(doc.log.empty());
  EXPECT_FALSE(doc.notes.empty());
}

TEST(SaveDocTest, SideFlagsParse)
{
  const std::filesystem::path scenario =
      std::filesystem::path(HEXGAMES_SOURCE_DIR) / "games" / "trc" / "scenario" / "trc-1941.xml";
  const HexXml::SaveDoc doc = HexXml::SaveDoc::parse(HexXml::XmlDocument::load(scenario));
  const auto axis = std::find_if(doc.sides.begin(), doc.sides.end(),
                                  [](const HexXml::SaveSideDoc& s) { return "axis" == s.id; });
  ASSERT_NE(doc.sides.end(), axis);
  ASSERT_EQ(2u, axis->flags.size());
  EXPECT_EQ("weather", axis->flags[0].name);
  EXPECT_EQ("clear", axis->flags[0].value);
  EXPECT_EQ("weather-drm", axis->flags[1].name);
}
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------

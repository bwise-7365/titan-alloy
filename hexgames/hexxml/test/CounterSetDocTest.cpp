// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
#include "hexxml/CounterSetDoc.h"

#include <gtest/gtest.h>

#include <algorithm>
#include <filesystem>
#include <stdexcept>

namespace {

  std::filesystem::path
  unitGraphics(const char* name)
  {
    return std::filesystem::path(HEXGAMES_SOURCE_DIR) / "unit_graphics" / "xml" / name;
  }

  const HexXml::CounterDoc&
  find(const HexXml::CounterSetDoc& doc, const std::string& id)
  {
    const auto it = std::find_if(doc.counters.begin(), doc.counters.end(),
                                  [&](const HexXml::CounterDoc& c) { return c.id == id; });
    if (doc.counters.end() == it) {
      throw std::runtime_error("counter not found: " + id);
    }
    return *it;
  }

}  // namespace

TEST(CounterSetDocTest, TrcParsesAllTwoHundredFourteenCounters)
{
  const HexXml::XmlDocument xml = HexXml::XmlDocument::load(unitGraphics("the-russian-campaign.xml"));
  const HexXml::CounterSetDoc doc = HexXml::CounterSetDoc::parse(xml);

  EXPECT_EQ("trc-2020", doc.id);
  EXPECT_EQ(214u, doc.counters.size());
  EXPECT_FALSE(doc.palette.empty());
  EXPECT_FALSE(doc.styles.empty());
  ASSERT_FALSE(doc.sheets.empty());

  const HexXml::CounterDoc& armour = find(doc, "g-ge-41-armour");
  EXPECT_EQ("unit", armour.family);
  ASSERT_EQ(1u, armour.front.values.size());
  EXPECT_EQ("8-7", armour.front.values[0].text);
  ASSERT_TRUE(armour.back.has_value());

  const HexXml::CounterDoc& hitler = find(doc, "g-ge-hitler-hitler");
  EXPECT_EQ("leader", hitler.family);
  ASSERT_EQ(1u, hitler.front.values.size());
  EXPECT_EQ("1-8", hitler.front.values[0].text);
  ASSERT_TRUE(hitler.back.has_value());
  ASSERT_EQ(1u, hitler.back->face.texts.size());
  EXPECT_EQ("Berlin", hitler.back->face.texts[0].text);

  const HexXml::CounterDoc& stuka = find(doc, "g-lu-stuka-stuka");
  EXPECT_EQ("support", stuka.family);
  ASSERT_TRUE(stuka.back.has_value());
  EXPECT_EQ("same", stuka.back->derived.value());

  int leaders = 0, markers = 0, supports = 0, units = 0;
  for (const HexXml::CounterDoc& c : doc.counters) {
    if ("leader" == c.family) {
      ++leaders;
    } else if ("marker" == c.family) {
      ++markers;
    } else if ("support" == c.family) {
      ++supports;
    } else if ("unit" == c.family) {
      ++units;
    }
  }
  EXPECT_EQ(2, leaders);
  EXPECT_EQ(12, markers);
  EXPECT_EQ(3, supports);
  EXPECT_EQ(197, units);
}

TEST(CounterSetDocTest, FourCounterSetsParse)
{
  for (const char* name : {"the-russian-campaign.xml", "dai-senso.xml", "d-day-at-tarawa.xml",
                            "panzergruppe-guderian.xml"}) {
    const HexXml::XmlDocument xml = HexXml::XmlDocument::load(unitGraphics(name));
    EXPECT_NO_THROW((void)HexXml::CounterSetDoc::parse(xml)) << name;
  }
}

TEST(CounterSetDocTest, BadFamilyThrows)
{
  const std::filesystem::path bad =
      std::filesystem::path(HEXGAMES_SOURCE_DIR) / "hexxml" / "test" / "counters-bad-family.xml";
  const HexXml::XmlDocument xml = HexXml::XmlDocument::load(bad);
  EXPECT_THROW((void)HexXml::CounterSetDoc::parse(xml), std::invalid_argument);
}
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------

// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
#include "hexxml/PackageDoc.h"

#include <gtest/gtest.h>

#include <algorithm>
#include <filesystem>

namespace {

  std::filesystem::path
  packagesXml(const char* name)
  {
    return std::filesystem::path(HEXGAMES_SOURCE_DIR) / "packages" / "xml" / name;
  }

}  // namespace

TEST(PackageDocTest, TrcParses)
{
  const HexXml::XmlDocument xml = HexXml::XmlDocument::load(packagesXml("trc.package.xml"));
  const HexXml::PackageDoc doc = HexXml::PackageDoc::parse(xml);

  EXPECT_EQ("trc", doc.id);
  EXPECT_NE(std::string::npos, doc.rules.path.find("the-russian-campaign.xml"));
  ASSERT_EQ(1u, doc.sheets.size());
  ASSERT_EQ(1u, doc.counters.size());
  ASSERT_EQ(1u, doc.scenarios.size());
  EXPECT_EQ("trc-test", doc.scenarios[0].id);

  EXPECT_FALSE(doc.terrain.empty());
  EXPECT_FALSE(doc.hexside.empty());
  ASSERT_EQ(1u, doc.network.size());
  EXPECT_EQ("rail", doc.network[0].kind);
  EXPECT_FALSE(doc.space.empty());
  EXPECT_TRUE(doc.layer.empty());
  EXPECT_FALSE(doc.unit.empty());

  const auto infantry = std::find_if(doc.unit.begin(), doc.unit.end(), [](const HexXml::PackageUnitBindingDoc& u) {
    return "infantry" == u.type;
  });
  ASSERT_NE(doc.unit.end(), infantry);
  ASSERT_TRUE(infantry->match.has_value());

  const auto leader = std::find_if(doc.unit.begin(), doc.unit.end(), [](const HexXml::PackageUnitBindingDoc& u) {
    return "leader" == u.type;
  });
  ASSERT_NE(doc.unit.end(), leader);
  EXPECT_EQ(2u, leader->counters.size());
}
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------

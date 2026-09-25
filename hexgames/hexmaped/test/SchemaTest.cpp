// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
// The editor's vocabularies are the schema's: every enumeration list here equals the one in
// hexsheet.xsd, read from the file, so a schema change that forgets Schema.cpp is a failing test.
#include "hexmaped/Schema.h"
#include "hexxml/XmlDocument.h"

#include <gtest/gtest.h>

#include <string>
#include <vector>

namespace {

  std::vector<std::string>
  enumerationOf(const std::string& typeName)
  {
    const HexXml::XmlDocument xsd =
      HexXml::XmlDocument::load(std::string(HEXGAMES_SOURCE_DIR) + "/map_graphics/xml/hexsheet.xsd");
    for (const HexXml::XmlNode& t : xsd.root().children("xs:simpleType")) {
      if (t.optional("name") != typeName) {
        continue;
      }
      std::vector<std::string> out;
      const std::optional<HexXml::XmlNode> r = t.child("xs:restriction");
      if (!r.has_value()) {
        return out;
      }
      for (const HexXml::XmlNode& e : r->children("xs:enumeration")) {
        out.push_back(e.required("value"));
      }
      return out;
    }
    throw std::invalid_argument("hexsheet.xsd has no simpleType " + typeName);
  }

  std::vector<std::string>
  asStrings(std::span<const std::string_view> values)
  {
    return std::vector<std::string>(values.begin(), values.end());
  }

}  // namespace

TEST(SchemaTest, SymbolsEqualTheXsd) { EXPECT_EQ(enumerationOf("Symbol"), asStrings(HexMapEd::Schema::symbolList())); }

TEST(SchemaTest, SlotsEqualTheXsd) { EXPECT_EQ(enumerationOf("Slot"), asStrings(HexMapEd::Schema::slotList())); }

TEST(SchemaTest, DirsEqualTheXsd) { EXPECT_EQ(enumerationOf("Dir"), asStrings(HexMapEd::Schema::dirList())); }

TEST(SchemaTest, PatternsEqualTheXsd) { EXPECT_EQ(enumerationOf("Pattern"), asStrings(HexMapEd::Schema::patternList())); }

TEST(SchemaTest, EndReasonsEqualTheXsd)
{
  EXPECT_EQ(enumerationOf("EndReason"), asStrings(HexMapEd::Schema::endReasonList()));
}

TEST(SchemaTest, RefusesValuesOutsideTheVocabulary)
{
  EXPECT_THROW(HexMapEd::Schema::requireSymbol("castle", "test"), std::invalid_argument);
  EXPECT_THROW(HexMapEd::Schema::requireSlot("centre", "test"), std::invalid_argument);
  EXPECT_THROW(HexMapEd::Schema::requireHexId("12-34", "test"), std::invalid_argument);
  EXPECT_THROW(HexMapEd::Schema::requireColourValue("#12345", "test"), std::invalid_argument);
  EXPECT_NO_THROW(HexMapEd::Schema::requireSymbol("city", "test"));
  EXPECT_NO_THROW(HexMapEd::Schema::requireColourValue("none", "test"));
}
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------

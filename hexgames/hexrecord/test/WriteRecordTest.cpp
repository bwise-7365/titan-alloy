// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
// The canonical writer: fixed attribute order, LF endings, two-space indent, no trailing whitespace,
// and writing the same model twice gives identical bytes.
// ----------------------------------------------
#include "hexrecord/SaveModel.h"

#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>
#include <sstream>

namespace {

  using HexRecord::SaveControlHex;
  using HexRecord::SaveModel;
  using HexRecord::SaveUnit;

  SaveModel
  sampleModel()
  {
    SaveModel m;
    m.kind = "scenario";
    m.game = "trc";
    m.package = "../../packages/xml/trc.package.xml";
    m.seed = 20260912;
    m.title = "A test scenario";
    m.cursor.turn = 1;
    m.cursor.phase = "axis-i1-move";
    m.cursor.side = "axis";

    SaveUnit armour;
    armour.id = "g-ge-41-armour";
    armour.counter = "g-ge-41-armour";
    armour.type = "armour";
    armour.owner = "axis";
    armour.hex = "F27";
    m.units.push_back(armour);

    SaveUnit infantry;
    infantry.id = "g-ge-1-infantry";
    infantry.counter = "g-ge-1-infantry";
    infantry.type = "infantry";
    infantry.owner = "axis";
    infantry.hex = "F28";
    m.units.push_back(infantry);

    m.controlHexes.push_back(SaveControlHex{"E31", "axis"});
    m.controlHexes.push_back(SaveControlHex{"P12", "russian"});
    return m;
  }

  std::string
  readFile(const std::filesystem::path& path)
  {
    std::ifstream in(path, std::ios::binary);
    std::ostringstream contents;
    contents << in.rdbuf();
    return contents.str();
  }

}  // namespace

TEST(WriteRecordTest, CanonicalAndStable)
{
  const SaveModel model = sampleModel();
  const std::filesystem::path dir = std::filesystem::path(::testing::TempDir()) / "hexrecord_write_test";
  std::filesystem::create_directories(dir);
  const std::filesystem::path first = dir / "one.xml";
  const std::filesystem::path second = dir / "two.xml";

  HexRecord::writeCanonical(model, first);
  HexRecord::writeCanonical(model, second);

  const std::string text1 = readFile(first);
  const std::string text2 = readFile(second);
  ASSERT_FALSE(text1.empty());
  EXPECT_EQ(text1, text2);
  EXPECT_EQ(text1, HexRecord::canonicalText(model));

  EXPECT_EQ(std::string::npos, text1.find('\r')) << "LF line endings only";

  std::istringstream lines(text1);
  std::string line;
  while (std::getline(lines, line)) {
    if (!line.empty()) {
      EXPECT_NE(' ', line.back()) << "no trailing whitespace: " << line;
      EXPECT_NE('\t', line.back()) << "no trailing whitespace: " << line;
    }
  }

  EXPECT_EQ(0u, text1.find("<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"));
  EXPECT_NE(std::string::npos,
            text1.find("xsi:noNamespaceSchemaLocation=\"hexsave.xsd\" format=\"hexsave-1.0\" kind=\"scenario\" "
                       "game=\"trc\" package=\"../../packages/xml/trc.package.xml\""));
  EXPECT_NE(std::string::npos, text1.find("\n  <units>\n"));
  EXPECT_NE(std::string::npos, text1.find("\n    <unit id=\"g-ge-1-infantry\""));
}

TEST(WriteRecordTest, SelfClosesEmptyElements)
{
  SaveModel model = sampleModel();
  const std::string text = HexRecord::canonicalText(model);
  // Neither unit has text or child elements, so both tags self-close.
  EXPECT_NE(std::string::npos, text.find("F27\" face=\"front\" moved=\"false\"/>"));
}
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------

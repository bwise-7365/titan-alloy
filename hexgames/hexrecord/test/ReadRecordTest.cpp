// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
// SaveModel::read: the real TRC test scenario, and the three checks that are document-level (exactly
// one of unit/@hex and unit/@space; move numbers contiguous from 1; a "#k" copy suffix with k >= 1).
// Cross-document checks -- an unknown hex, an unknown counter, a "#k" beyond the counter count -- need
// a Board and Roster and are deferred to M4 (see the task file).
// ----------------------------------------------
#include "hexrecord/SaveModel.h"

#include <gtest/gtest.h>

#include <algorithm>
#include <filesystem>
#include <fstream>

namespace {

  using HexRecord::SaveModel;
  using HexRecord::SaveUnit;

  std::filesystem::path
  gameRecordsFile(const char* name)
  {
    return std::filesystem::path(HEXGAMES_SOURCE_DIR) / "game_records" / "xml" / name;
  }

  // Writes `xml` to a fresh file under the test's scratch directory and returns its path.
  std::filesystem::path
  writeFixture(const char* name, const std::string& xml)
  {
    const std::filesystem::path dir = std::filesystem::path(::testing::TempDir()) / "hexrecord_read_test";
    std::filesystem::create_directories(dir);
    const std::filesystem::path path = dir / name;
    std::ofstream out(path, std::ios::binary | std::ios::trunc);
    out << xml;
    return path;
  }

  const char* const kHeader =
      "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
      "<save xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" "
      "xsi:noNamespaceSchemaLocation=\"hexsave.xsd\"\n"
      "      format=\"hexsave-1.0\" kind=\"scenario\" game=\"trc\" package=\"p.xml\" seed=\"1\">\n"
      "  <cursor turn=\"1\" phase=\"p\"/>\n";

}  // namespace

TEST(ReadRecordTest, TrcTestScenario)
{
  const SaveModel model = SaveModel::read(gameRecordsFile("trc-test.xml"));

  EXPECT_EQ("scenario", model.kind);
  EXPECT_EQ("trc", model.game);
  EXPECT_EQ(20260912u, model.seed);
  ASSERT_EQ(11u, model.units.size());

  const auto stalin = std::find_if(model.units.begin(), model.units.end(),
                                    [](const SaveUnit& u) { return "r-ru-stalin-stalin" == u.id; });
  const auto stavka = std::find_if(model.units.begin(), model.units.end(),
                                    [](const SaveUnit& u) { return "r-ru-stavka-hq" == u.id; });
  ASSERT_NE(model.units.end(), stalin);
  ASSERT_NE(model.units.end(), stavka);
  EXPECT_EQ("P12", stalin->hex.value());
  EXPECT_EQ("P12", stavka->hex.value());

  const auto inBox = std::find_if(model.units.begin(), model.units.end(),
                                   [](const SaveUnit& u) { return "g-ge-24-armour" == u.id; });
  ASSERT_NE(model.units.end(), inBox);
  EXPECT_FALSE(inBox->hex.has_value());
  EXPECT_EQ("omb", inBox->space.value());

  ASSERT_EQ(2u, model.controlHexes.size());
  const auto p12 = std::find_if(model.controlHexes.begin(), model.controlHexes.end(),
                                 [](const auto& h) { return "P12" == h.id; });
  const auto e31 = std::find_if(model.controlHexes.begin(), model.controlHexes.end(),
                                 [](const auto& h) { return "E31" == h.id; });
  ASSERT_NE(model.controlHexes.end(), p12);
  ASSERT_NE(model.controlHexes.end(), e31);
  EXPECT_EQ("russian", p12->side);
  EXPECT_EQ("axis", e31->side);

  ASSERT_EQ(2u, model.streams.size());
}

TEST(ReadRecordTest, UnitWithBothHexAndSpaceThrows)
{
  const std::string xml = std::string(kHeader) +
                           "  <units>\n"
                           "    <unit id=\"u1\" counter=\"u1\" type=\"infantry\" owner=\"axis\" hex=\"A1\" "
                           "space=\"omb\"/>\n"
                           "  </units>\n"
                           "</save>\n";
  const std::filesystem::path path = writeFixture("both-hex-and-space.xml", xml);

  try {
    SaveModel::read(path);
    FAIL() << "expected std::invalid_argument";
  }
  catch (const std::invalid_argument& e) {
    const std::string what = e.what();
    EXPECT_NE(std::string::npos, what.find("u1"));
    EXPECT_NE(std::string::npos, what.find("hex"));
  }
}

TEST(ReadRecordTest, UnitWithNeitherHexNorSpaceThrows)
{
  const std::string xml = std::string(kHeader) +
                           "  <units>\n"
                           "    <unit id=\"u1\" counter=\"u1\" type=\"infantry\" owner=\"axis\"/>\n"
                           "  </units>\n"
                           "</save>\n";
  const std::filesystem::path path = writeFixture("neither-hex-nor-space.xml", xml);

  try {
    SaveModel::read(path);
    FAIL() << "expected std::invalid_argument";
  }
  catch (const std::invalid_argument& e) {
    const std::string what = e.what();
    EXPECT_NE(std::string::npos, what.find("u1"));
  }
}

TEST(ReadRecordTest, CopySuffixBelowOneThrows)
{
  const std::string xml = std::string(kHeader) +
                           "  <units>\n"
                           "    <unit id=\"u1#0\" counter=\"u1\" type=\"infantry\" owner=\"axis\" hex=\"A1\"/>\n"
                           "  </units>\n"
                           "</save>\n";
  const std::filesystem::path path = writeFixture("bad-copy-suffix.xml", xml);

  try {
    SaveModel::read(path);
    FAIL() << "expected std::invalid_argument";
  }
  catch (const std::invalid_argument& e) {
    const std::string what = e.what();
    EXPECT_NE(std::string::npos, what.find("u1#0"));
    EXPECT_NE(std::string::npos, what.find("#0"));
  }
}

TEST(ReadRecordTest, MoveNumbersMustBeContiguousFromOne)
{
  const std::string xml = std::string(kHeader) +
                           "  <log>\n"
                           "    <move n=\"1\" turn=\"1\" phase=\"p\" side=\"axis\" cmd=\"end-phase\"/>\n"
                           "    <move n=\"3\" turn=\"1\" phase=\"p\" side=\"axis\" cmd=\"end-phase\"/>\n"
                           "  </log>\n"
                           "</save>\n";
  const std::filesystem::path path = writeFixture("gap-in-move-numbers.xml", xml);

  try {
    SaveModel::read(path);
    FAIL() << "expected std::invalid_argument";
  }
  catch (const std::invalid_argument& e) {
    const std::string what = e.what();
    EXPECT_NE(std::string::npos, what.find("contiguous"));
  }
}
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------

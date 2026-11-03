// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
#include "HexGenTest.h"

#include "hexgen/Settings.h"

#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <string>

namespace {

  using HexGen::Role;
  using HexGen::Settings;
  using HexGenTest::corpus;

  TEST(HexGenSettings, TheCorpusDocumentIsHexKriegsDefaults)
  {
    const Settings read = Settings::load(corpus("valid/generator-hexkrieg.xml"));
    EXPECT_EQ(HexGen::writeSettings(Settings::hexkrieg()), HexGen::writeSettings(read));
    EXPECT_EQ(60834130, read.seed);
    EXPECT_EQ(10, read.terrain(Role::Low).percent);
    EXPECT_FALSE(read.terrain(Role::Low).roadCost.has_value());
    ASSERT_TRUE(read.roads.has_value());
    EXPECT_EQ(2u, read.roads->sides.size());
    EXPECT_FALSE(read.altitude.has_value());
    return;
  }

  TEST(HexGenSettings, WritingThenReadingGivesTheSameSettings)
  {
    Settings s = Settings::hexkrieg();
    s.orientation = HexCoord::Orientation::Pointy;
    s.offset = HexCoord::Parity::Even;
    s.idFormat = "{col}-{row}";
    s.title = "A & B <test>";
    s.altitude = HexGen::AltitudeSettings{1500.0, 50.0};
    const std::filesystem::path tmp = std::filesystem::temp_directory_path() / "hexgen-settings-roundtrip.xml";
    HexGen::writeSettings(tmp, s);
    EXPECT_EQ(HexGen::writeSettings(s), HexGen::writeSettings(Settings::load(tmp)));
    return;
  }

  TEST(HexGenSettings, TheInvalidCorpusDocumentsAreRefused)
  {
    for (const char* name : {"generator-no-percent.xml", "generator-over-100.xml",
                             "generator-cities-above-roads.xml", "generator-two-lows.xml"}) {
      EXPECT_THROW(Settings::load(corpus(std::string("invalid/") + name)), std::invalid_argument) << name;
    }
    return;
  }

  TEST(HexGenSettings, ValidateRefusesWhatTheSchemaRefuses)
  {
    Settings s = Settings::hexkrieg();
    s.cols = 4;
    EXPECT_THROW(HexGen::validate(s, "test"), std::invalid_argument);
    s = Settings::hexkrieg();
    HexGenTest::terrainOf(s, Role::City).roadCost.reset();
    EXPECT_THROW(HexGen::validate(s, "test"), std::invalid_argument);
    s = Settings::hexkrieg();
    HexGenTest::terrainOf(s, Role::Cover).roadCost = 1.234;  // not whole hundredths
    EXPECT_THROW(HexGen::validate(s, "test"), std::invalid_argument);
    s = Settings::hexkrieg();
    s.idFormat = "{colletter}{row}";  // columns are numbered from 0, which has no letter
    EXPECT_THROW(HexGen::validate(s, "test"), std::invalid_argument);
    s = Settings::hexkrieg();
    s.terrains.pop_back();
    EXPECT_THROW(HexGen::validate(s, "test"), std::invalid_argument);
    return;
  }

  TEST(HexGenSettings, IdsWidenAtOneHundredAndMustBeDistinct)
  {
    Settings s = Settings::hexkrieg();
    EXPECT_EQ("{col:02}{row:02}", s.idFormatOrDefault());
    s.cols = 100;
    EXPECT_EQ("{col:03}{row:03}", s.idFormatOrDefault());
    HexGen::validate(s, "test");
    s.cols = 150;
    s.rows = 150;
    s.idFormat = "{col:02}{row:02}";  // 10100 is both col 101 row 00 and col 10 row 100
    EXPECT_THROW(HexGen::validate(s, "test"), std::invalid_argument);
    return;
  }

}  // namespace
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------

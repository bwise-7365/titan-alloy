// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
// The generated sheet: repeatable, read back unchanged, drawable by the editor, and equal to its
// golden. The golden is regenerated (after a deliberate change, reviewed by git diff) with
//   hexgen_cli <HexRules>/corpus/valid/generator-hexkrieg.xml hexgen/test/golden/generator-hexkrieg.sheet.xml
// ----------------------------------------------
#include "HexGenTest.h"

#include "hexgen/Generate.h"

#include "hexmaped/SheetFrame.h"
#include "hexmaped/SheetWriter.h"
#include "hexxml/XmlDocument.h"

#include <gtest/gtest.h>

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <sstream>
#include <string>

namespace {

  using HexGen::Settings;

  std::string
  text(const std::filesystem::path& path)
  {
    std::ifstream file(path, std::ios::binary);
    std::string s((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    s.erase(std::remove(s.begin(), s.end(), '\r'), s.end());  // a checkout may have CRLF line ends
    return s;
  }

  std::vector<Settings>
  variants()
  {
    std::vector<Settings> out{Settings::hexkrieg()};
    Settings pointy = Settings::hexkrieg();
    pointy.orientation = HexCoord::Orientation::Pointy;
    pointy.method = HexGen::Method::Fractal;
    out.push_back(pointy);
    Settings even = Settings::hexkrieg();
    even.offset = HexCoord::Parity::Even;
    even.altitude = HexGen::AltitudeSettings{};
    for (const HexGen::Side side : {HexGen::Side::Top, HexGen::Side::Bottom}) {
      even.roads->sides.push_back({side, 3, 1});
    }
    out.push_back(even);
    return out;
  }

  TEST(HexGenSheet, TheSameSettingsGiveTheSameSheet)
  {
    for (const Settings& s : variants()) {
      EXPECT_EQ(HexMapEd::writeSheet(HexGen::generate(s)), HexMapEd::writeSheet(HexGen::generate(s)));
    }
    Settings other = Settings::hexkrieg();
    other.seed = 1;
    EXPECT_NE(HexMapEd::writeSheet(HexGen::generate(Settings::hexkrieg())), HexMapEd::writeSheet(HexGen::generate(other)));
    return;
  }

  TEST(HexGenSheet, ReadingTheSheetBackChangesNothing)
  {
    int k = 0;
    for (const Settings& s : variants()) {
      const std::filesystem::path tmp =
          std::filesystem::temp_directory_path() / ("hexgen-sheet-" + std::to_string(++k) + ".xml");
      const std::string written = HexMapEd::writeSheet(HexGen::generate(s));
      HexMapEd::writeSheet(tmp, HexGen::generate(s));
      const HexXml::SheetDoc read = HexXml::SheetDoc::parse(HexXml::XmlDocument::load(tmp));
      EXPECT_EQ(written, HexMapEd::writeSheet(read));
      EXPECT_EQ(s.seed, read.seed);
      ASSERT_EQ(1u, read.grids.size());
      // The editor's geometry accepts it, and every hex has a printed id.
      const HexMapEd::SheetFrame frame = HexMapEd::SheetFrame::of(read);
      EXPECT_EQ(static_cast<std::size_t>(s.cols * s.rows), frame.ids().size());
      EXPECT_FALSE(read.links.empty());
      EXPECT_EQ(s.altitude.has_value(), !read.hexes.empty());
    }
    return;
  }

  TEST(HexGenSheet, TheCorpusSettingsGiveTheGolden)
  {
    const Settings s = Settings::load(HexGenTest::corpus("valid/generator-hexkrieg.xml"));
    const std::filesystem::path golden =
        std::filesystem::path(HEXGEN_TEST_DIR) / "golden" / "generator-hexkrieg.sheet.xml";
    ASSERT_TRUE(std::filesystem::exists(golden)) << golden.string();
    EXPECT_EQ(text(golden), HexMapEd::writeSheet(HexGen::generate(s)))
        << "regenerate with hexgen_cli only after a deliberate change, and review the diff";
    return;
  }

}  // namespace
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------

// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
// writeSvg of the map scene against hexsheet2svg.py's SVG of the same sheet, both normalised by
// svg-golden.py (its docstring says how), element for element and layer for layer. The reference
// SVG is rendered at test time into the build tree, beside the C++ one, so a failure can be
// inspected; the comparison report (per-layer counts, first differences) is in the test's output.
// ----------------------------------------------
#include "TestSupport.h"
#include "hexview/MapSceneBuilder.h"
#include "hexview/SceneWriters.h"

#include <gtest/gtest.h>

namespace {

  using namespace HexViewTest;

  void
  expectMatchesReference(const std::string& name)
  {
    const HexXml::SheetDoc sheet = loadSheet(name);
    const HexView::MapFrame frame = HexView::MapFrame::of(sheet);
    const HexView::SymbolLibrary lib = HexView::SymbolLibrary::reference();
    HexView::Scene scene(frame.width(), frame.height());
    HexView::MapSceneBuilder(frame, lib, HexView::MapStyle::reference()).build(sheet, scene);

    const std::filesystem::path mine = outDir() / (name + ".hexview.svg");
    const std::filesystem::path ref = outDir() / (name + ".reference.svg");
    writeFile(mine, HexView::writeSvg(scene, lib, sheet.title));
    ASSERT_EQ(0, runReferenceScript({"reference", sheetPath(name).string(), ref.string()}));
    EXPECT_EQ(
        0, runReferenceScript({"compare", sheetPath(name).string(), ref.string(), mine.string()}))
        << "see the report above; the SVGs are in " << outDir().string();
    return;
  }

  TEST(MapSvgGoldenTest, TheRussianCampaign)
  {
    expectMatchesReference("the-russian-campaign");
  }

  TEST(MapSvgGoldenTest, PanzergruppeGuderian)
  {
    expectMatchesReference("panzergruppe-guderian");
  }

  TEST(MapSvgGoldenTest, DDayAtTarawa)
  {
    expectMatchesReference("d-day-at-tarawa");
  }

  TEST(MapSvgGoldenTest, DaiSenso)
  {
    expectMatchesReference("dai-senso");
  }

  TEST(MapSvgGoldenTest, StalinMovesWest)
  {
    expectMatchesReference("stalin-moves-west");
  }

  TEST(MapSvgGoldenTest, VelikiyeLuki)
  {
    expectMatchesReference("velikiye-luki");
  }

}  // namespace
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------

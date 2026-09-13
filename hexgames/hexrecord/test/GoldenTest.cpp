// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
// Golden comparison at the document level: two hand-built SaveModels differing at one move's draw.
// GoldenTest.ScriptIsAlsoASave (replaying a save's own log against its own position) needs a working
// Session and is deferred to M4.
// ----------------------------------------------
#include "hexrecord/GoldenCompare.h"

#include <gtest/gtest.h>

#include <filesystem>

namespace {

  using HexRecord::SaveDraw;
  using HexRecord::SaveModel;
  using HexRecord::SaveMove;
  using HexRecord::SaveResult;

  SaveModel
  makeLog(bool mismatchAtMoveThreeP)
  {
    SaveModel m;
    m.kind = "golden";
    m.game = "trc";
    m.package = "p.xml";
    m.seed = 1;
    m.cursor.turn = 1;
    m.cursor.phase = "p";

    for (int n = 1; n <= 5; ++n) {
      SaveMove move;
      move.n = n;
      move.turn = 1;
      move.phase = "p";
      move.side = "axis";
      move.cmd = "move-unit";
      SaveResult result;
      result.outcome = "ok";
      move.result = result;
      if (3 == n) {
        SaveDraw draw;
        draw.stream = "combat";
        draw.n = 5;
        draw.value = mismatchAtMoveThreeP ? "4" : "3";
        move.draws.push_back(draw);
      }
      m.log.push_back(move);
    }
    return m;
  }

}  // namespace

TEST(GoldenTest, ReportOnMismatch)
{
  const SaveModel golden = makeLog(false);
  const SaveModel actual = makeLog(true);

  const std::filesystem::path dir = std::filesystem::path(::testing::TempDir()) / "hexrecord_golden_test";
  std::filesystem::create_directories(dir);
  const std::filesystem::path goldenPath = dir / "t1.golden.xml";
  HexRecord::writeCanonical(golden, goldenPath);

  const HexRecord::GoldenReport report = HexRecord::buildGoldenReport(golden, actual, goldenPath);

  EXPECT_FALSE(report.matchP);
  ASSERT_TRUE(report.first.has_value());
  EXPECT_EQ(3, report.first->moveNumber);
  EXPECT_EQ("draw", report.first->what);
  EXPECT_EQ("combat#5=3", report.first->expected);
  EXPECT_EQ("combat#5=4", report.first->actual);
  ASSERT_TRUE(std::filesystem::exists(report.actualWritten));
  EXPECT_EQ(dir / "t1.actual.xml", report.actualWritten);
  EXPECT_NE(std::string::npos, report.unifiedDiff.find("value=\"4\""));

  const std::string formatted = HexRecord::formatGoldenReport(report, "trc", "t1");
  EXPECT_NE(std::string::npos, formatted.find("move 3"));
  EXPECT_NE(std::string::npos, formatted.find("re-bless: tools\\bless-goldens.ps1 -Game trc -Name t1"));
}

TEST(GoldenTest, ReportOnMatch)
{
  const SaveModel golden = makeLog(false);
  const SaveModel actual = makeLog(false);

  const std::filesystem::path dir = std::filesystem::path(::testing::TempDir()) / "hexrecord_golden_test";
  std::filesystem::create_directories(dir);
  const std::filesystem::path goldenPath = dir / "t2.golden.xml";
  HexRecord::writeCanonical(golden, goldenPath);

  const HexRecord::GoldenReport report = HexRecord::buildGoldenReport(golden, actual, goldenPath);

  EXPECT_TRUE(report.matchP);
  EXPECT_FALSE(report.first.has_value());
  EXPECT_TRUE(std::filesystem::exists(report.actualWritten));

  const std::string formatted = HexRecord::formatGoldenReport(report, "trc", "t2");
  EXPECT_NE(std::string::npos, formatted.find("golden match"));
}
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------

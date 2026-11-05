// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
// A script of either game steps forward and back to the same positions, and a run file is read as written.
// ----------------------------------------------
#include "ViewerGame.h"

#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>

namespace {

  const std::filesystem::path kRoot = HEXGAMES_SOURCE_DIR;

  std::size_t
  shapesIn(const HexView::Scene& scene, HexView::Layer layer)
  {
    return scene.layer(layer).size();
  }

}  // namespace

TEST(ViewerGameTest, ScriptsOfBothGamesStepToTheSamePlaceFromAnyDirection)
{
  for (const std::filesystem::path& script : {kRoot / "games" / "pgg" / "golden" / "interdiction.script.xml",
                                              kRoot / "games" / "trc" / "golden" / "combat-crt.script.xml"}) {
    std::unique_ptr<HexQt::ViewerGame> game = HexQt::ViewerGame::fromScript(script);
    ASSERT_LT(0u, game->moveCount()) << script;
    EXPECT_NE(0u, game->fingerprint());
    EXPECT_FALSE(game->expectedFingerprint().has_value());
    EXPECT_FALSE(game->fingerprintMismatchP());
    const std::string lastStatus = game->statusAt(game->moveCount());
    const HexView::Scene last = game->overlay(game->moveCount(), {});
    const std::string firstStatus = game->statusAt(0);
    EXPECT_EQ(firstStatus, game->statusAt(0));
    EXPECT_EQ(lastStatus, game->statusAt(game->moveCount()));  // forward again from the start
    EXPECT_EQ(shapesIn(last, HexView::Layer::Units), shapesIn(game->overlay(game->moveCount(), {}), HexView::Layer::Units));
    EXPECT_THROW((void)game->statusAt(game->moveCount() + 1), std::invalid_argument);
  }
}

TEST(ViewerGameTest, TheSameStepIsTheSameWhateverTheWayToIt)
{
  std::unique_ptr<HexQt::ViewerGame> game = HexQt::ViewerGame::fromScript(kRoot / "games" / "pgg" / "golden" / "full-turn.script.xml");
  ASSERT_LT(30u, game->moveCount());
  const std::string step25 = game->statusAt(25);
  const std::size_t units25 = shapesIn(game->overlay(25, {}), HexView::Layer::Units);
  (void)game->statusAt(game->moveCount());
  EXPECT_EQ(step25, game->statusAt(25));
  EXPECT_EQ(units25, shapesIn(game->overlay(25, {}), HexView::Layer::Units));
  (void)game->statusAt(0);
  EXPECT_EQ(step25, game->statusAt(25));
  EXPECT_FALSE(game->commandOf(25).empty());
  EXPECT_TRUE(game->commandOf(0).empty());
}

TEST(ViewerGameTest, AGameIsMarkedWhereItsCommandActed)
{
  std::unique_ptr<HexQt::ViewerGame> game = HexQt::ViewerGame::fromScript(kRoot / "games" / "pgg" / "golden" / "full-turn.script.xml");
  bool markedP = false;
  for (std::size_t move = 1; move <= game->moveCount() && !markedP; ++move) {
    markedP = 0 < shapesIn(game->overlay(move, {}), HexView::Layer::Highlights);
  }
  EXPECT_TRUE(markedP);
  EXPECT_EQ(0u, shapesIn(game->overlay(0, {}), HexView::Layer::Highlights));
}

TEST(ViewerGameTest, RunFilesAreReadWithTheirSettingsAndGames)
{
  const HexQt::RunFile run = HexQt::readRunFile(kRoot / "doc" / "pgg-runs" / "2026-11-04-drawn-seize-seize.txt");
  EXPECT_EQ("pgg", run.game);
  EXPECT_EQ("drawn", run.setUp);
  EXPECT_EQ("seize", run.german);
  EXPECT_EQ("seize", run.soviet);
  ASSERT_EQ(12u, run.lines.size());
  EXPECT_EQ(7, run.lines[7].number);
  EXPECT_NE(0u, run.lines[7].fingerprint);
  EXPECT_THROW((void)HexQt::readRunFile(kRoot / "BUGS.txt"), std::invalid_argument);
  EXPECT_THROW((void)HexQt::readRunFile(kRoot / "no-such-file.txt"), std::invalid_argument);
}

TEST(ViewerGameTest, ARunOfAGameWithNoAgentsIsRefusedByName)
{
  HexQt::RunFile run;
  run.game = "trc";
  EXPECT_THROW((void)HexQt::ViewerGame::fromRun(kRoot, run, HexQt::RunLine{}), std::invalid_argument);
}
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------

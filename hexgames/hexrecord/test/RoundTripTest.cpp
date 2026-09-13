// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
// read -> write -> read -> write gives identical bytes: the real TRC test scenario, and a synthetic
// DDaT-style record with a draw pile order and @next.
// ----------------------------------------------
#include "hexrecord/SaveModel.h"

#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>
#include <sstream>

namespace {

  using HexRecord::SaveModel;
  using HexRecord::SavePile;

  std::filesystem::path
  gameRecordsFile(const char* name)
  {
    return std::filesystem::path(HEXGAMES_SOURCE_DIR) / "game_records" / "xml" / name;
  }

  std::string
  readFile(const std::filesystem::path& path)
  {
    std::ifstream in(path, std::ios::binary);
    std::ostringstream contents;
    contents << in.rdbuf();
    return contents.str();
  }

  // read(path) -> writeCanonical -> read -> writeCanonical; returns the two written texts, which
  // should be identical.
  std::pair<std::string, std::string>
  roundTripTwice(const std::filesystem::path& source, const std::filesystem::path& scratchDir)
  {
    std::filesystem::create_directories(scratchDir);
    const std::filesystem::path first = scratchDir / "first.xml";
    const std::filesystem::path second = scratchDir / "second.xml";

    const SaveModel loadedOnce = SaveModel::read(source);
    HexRecord::writeCanonical(loadedOnce, first);

    const SaveModel loadedTwice = SaveModel::read(first);
    HexRecord::writeCanonical(loadedTwice, second);

    return {readFile(first), readFile(second)};
  }

}  // namespace

TEST(RoundTripTest, SaveReloadSave)
{
  const std::filesystem::path dir = std::filesystem::path(::testing::TempDir()) / "hexrecord_roundtrip_trc";
  const auto [firstText, secondText] = roundTripTwice(gameRecordsFile("trc-test.xml"), dir);

  ASSERT_FALSE(firstText.empty());
  EXPECT_EQ(firstText, secondText);
}

TEST(RoundTripTest, DdatStyleDrawPileWithNext)
{
  SaveModel model;
  model.kind = "save";
  model.game = "ddat";
  model.package = "p.xml";
  model.seed = 42;
  model.cursor.turn = 1;
  model.cursor.phase = "p";

  SavePile pile;
  pile.randomizer = "event-deck";
  pile.kind = "draw";
  pile.cards = {"card-3", "card-17", "card-1", "card-9"};
  pile.next = 2;
  model.piles.push_back(pile);

  const std::filesystem::path dir = std::filesystem::path(::testing::TempDir()) / "hexrecord_roundtrip_ddat";
  std::filesystem::create_directories(dir);
  const std::filesystem::path source = dir / "source.xml";
  HexRecord::writeCanonical(model, source);

  const auto [firstText, secondText] = roundTripTwice(source, dir);
  ASSERT_FALSE(firstText.empty());
  EXPECT_EQ(firstText, secondText);

  const SaveModel reloaded = SaveModel::read(source);
  ASSERT_EQ(1u, reloaded.piles.size());
  EXPECT_EQ("event-deck", reloaded.piles[0].randomizer);
  EXPECT_EQ("draw", reloaded.piles[0].kind);
  EXPECT_EQ(pile.cards, reloaded.piles[0].cards);
  ASSERT_TRUE(reloaded.piles[0].next.has_value());
  EXPECT_EQ(2, reloaded.piles[0].next.value());
}
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------

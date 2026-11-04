// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
// Whole games from the 1941 scenario: each reaches a result, and the same task is the same game.
// ----------------------------------------------
#include "PggGame.h"
#include "PggTestFixture.h"

#include <gtest/gtest.h>

namespace {

  Pgg::PggGameTask
  taskOf(std::uint64_t seed, Pgg::SetUp setUp, HexAi::Tactic german, HexAi::Tactic soviet)
  {
    Pgg::PggGameTask task;
    task.seed = seed;
    task.setUp = setUp;
    task.german = std::move(german);
    task.soviet = std::move(soviet);
    return task;
  }

  void
  expectAResult(const Pgg::PggGameResult& result)
  {
    EXPECT_TRUE(result.play.finishedP);
    EXPECT_TRUE(result.germanPoints.has_value());
    EXPECT_TRUE(result.sovietPoints.has_value());
    EXPECT_FALSE(result.level.empty());
    EXPECT_EQ(13, result.lastTurn);  // the victory is declared as turn 12 ends
    EXPECT_EQ(12u, result.play.turnFingerprints.size());
    return;
  }

}  // namespace

TEST(PggGameTest, AGameReachesAResultAndTheSameTaskIsTheSameGame)
{
  const Pgg::PggShared shared = Pgg::loadShared(PggTest::root());
  const Pgg::PggGameTask task = taskOf(1, Pgg::SetUp::Fixed, HexAi::AttackOrConsolidate{}, HexAi::AttackOrConsolidate{});
  const Pgg::PggGameResult once = Pgg::playGame(shared, task);
  expectAResult(once);
  const Pgg::PggGameResult again = Pgg::playGame(shared, task);
  EXPECT_EQ(once.play.fingerprint, again.play.fingerprint);
  EXPECT_EQ(once.play.turnFingerprints, again.play.turnFingerprints);
  EXPECT_EQ(once.germanPoints, again.germanPoints);
  EXPECT_EQ(once.sovietPoints, again.sovietPoints);

  const Pgg::PggGameResult other = Pgg::playGame(shared, taskOf(2, Pgg::SetUp::Fixed, HexAi::AttackOrConsolidate{}, HexAi::AttackOrConsolidate{}));
  expectAResult(other);
  EXPECT_NE(once.play.fingerprint, other.play.fingerprint);
}

TEST(PggGameTest, TheSeizeTacticPlaysToAResultAndKeepsAShareForTheGame)
{
  const Pgg::PggShared shared = Pgg::loadShared(PggTest::root());
  const Pgg::PggGameTask task = taskOf(4, Pgg::SetUp::Drawn, HexAi::SeizeLocations{}, HexAi::SeizeLocations{});
  const Pgg::PggGameResult once = Pgg::playGame(shared, task);
  expectAResult(once);
  EXPECT_LE(0.5, once.sovietFringeShare);
  EXPECT_GE(2.0 / 3.0, once.sovietFringeShare);
  const Pgg::PggGameResult again = Pgg::playGame(shared, task);
  EXPECT_EQ(once.play.fingerprint, again.play.fingerprint);
  EXPECT_EQ(once.sovietFringeShare, again.sovietFringeShare);
}

TEST(PggGameTest, ADrawnSetUpAndTheOtherTacticPlayToAResult)
{
  const Pgg::PggShared shared = Pgg::loadShared(PggTest::root());
  expectAResult(Pgg::playGame(shared, taskOf(3, Pgg::SetUp::Drawn, HexAi::AttackNearest{}, HexAi::AttackNearest{})));
}
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------

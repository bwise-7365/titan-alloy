// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
// The tactics on the toy board: the cases of HexKrieg's HKMoveTest, and the choice of a destination.
// ----------------------------------------------
#include "ToyBoard.h"

#include "hexai/Tactics.h"

#include <gtest/gtest.h>

#include <vector>

namespace {

  using HexAi::Piece;
  using HexModel::HexIndex;

  bool
  openP(HexIndex, HexModel::Direction)
  {
    return true;
  }

  // One entry per hex but `from`, at cost one; the paths do not matter to the choice.
  HexEngine::Reachability
  everywhereBut(const HexModel::Board& board, HexIndex from)
  {
    HexEngine::Reachability reach;
    for (std::size_t h = 0; h < board.hexCount(); ++h) {
      const HexIndex hex{static_cast<std::uint32_t>(h)};
      if (hex != from) {
        reach.hexes.push_back(hex);
        reach.paths.push_back({from, hex});
        reach.costs.push_back(HexModel::MovementPoints{2});
      }
    }
    return reach;
  }

  bool
  anywhereP(HexIndex)
  {
    return true;
  }

}  // namespace

TEST(TacticsTest, NamesAreRead)
{
  EXPECT_TRUE(std::holds_alternative<HexAi::AttackNearest>(HexAi::tacticNamed("nearest")));
  EXPECT_TRUE(std::holds_alternative<HexAi::AttackOrConsolidate>(HexAi::tacticNamed("consolidate")));
  EXPECT_THROW((void)HexAi::tacticNamed("charge"), std::invalid_argument);
}

TEST(TacticsTest, WithNoEnemyOnTheMapAUnitHolds)
{
  const HexModel::Board board = ToyBoard::build();
  const Piece me{ToyBoard::at(board, "C3"), 4.0};
  const std::vector<Piece> friends{me};
  EXPECT_TRUE(std::holds_alternative<HexAi::Hold>(HexAi::goalFor(board, HexAi::AttackNearest{}, me, friends, {})));
  EXPECT_TRUE(std::holds_alternative<HexAi::Hold>(HexAi::goalFor(board, HexAi::AttackOrConsolidate{}, me, friends, {})));
}

TEST(TacticsTest, AttackNearestClosesHoweverOutnumbered)
{
  const HexModel::Board board = ToyBoard::build();
  const Piece me{ToyBoard::at(board, "C3"), 1.0};
  const std::vector<Piece> friends{me};
  const std::vector<Piece> enemies{{ToyBoard::at(board, "C4"), 100.0}};
  EXPECT_TRUE(std::holds_alternative<HexAi::CloseWithEnemy>(HexAi::goalFor(board, HexAi::AttackNearest{}, me, friends, enemies)));
}

TEST(TacticsTest, AttackOrConsolidateReadsTheLocalRatio)
{
  const HexModel::Board board = ToyBoard::build();
  const Piece me{ToyBoard::at(board, "C3"), 1.0};
  const Piece ally{ToyBoard::at(board, "C2"), 1.0};
  const std::vector<Piece> friends{me, ally};
  const std::vector<Piece> strong{{ToyBoard::at(board, "C4"), 10.0}};
  const std::vector<Piece> weak{{ToyBoard::at(board, "C4"), 2.0}};

  const HexAi::AttackOrConsolidate tactic;
  EXPECT_TRUE(std::holds_alternative<HexAi::Rally>(HexAi::goalFor(board, tactic, me, friends, strong)));
  EXPECT_TRUE(std::holds_alternative<HexAi::CloseWithEnemy>(HexAi::goalFor(board, tactic, me, friends, weak)));

  HexAi::AttackOrConsolidate always;
  always.attackRatio = 0.0;  // a ratio of 0 always attacks, as HKMoveTest has it
  EXPECT_TRUE(std::holds_alternative<HexAi::CloseWithEnemy>(HexAi::goalFor(board, always, me, friends, strong)));

  HexAi::AttackOrConsolidate shortSighted;
  shortSighted.attackRadius = 0;  // no enemy in sight: the unit goes looking for one
  EXPECT_TRUE(std::holds_alternative<HexAi::CloseWithEnemy>(HexAi::goalFor(board, shortSighted, me, friends, strong)));
}

TEST(TacticsTest, ARallyIsNearestItsCentreOfMass)
{
  const HexModel::Board board = ToyBoard::build();
  const HexIndex a = ToyBoard::at(board, "A3");
  const HexIndex b = ToyBoard::at(board, "C3");
  const HexIndex middle = ToyBoard::at(board, "B3");
  ASSERT_EQ(2, board.distance(a, b));
  const std::vector<Piece> friends{{a, 1.0}, {b, 1.0}, {ToyBoard::at(board, "E5"), 1.0}};
  const HexAi::Rally rally = HexAi::rallyOf(board, a, 2, friends);
  EXPECT_EQ(2, rally.count);  // E5 is beyond the radius
  const HexAi::EnemyField none(board, {}, &openP);
  const long long centre = *HexAi::goalDistance(board, none, rally, middle);
  EXPECT_LT(centre, *HexAi::goalDistance(board, none, rally, a));
  EXPECT_LT(centre, *HexAi::goalDistance(board, none, rally, b));
}

TEST(TacticsTest, TheEnemyFieldCountsHexesAndStopsAtWalls)
{
  const HexModel::Board board = ToyBoard::build();
  const HexIndex enemy = ToyBoard::at(board, "C3");
  const std::vector<HexIndex> enemies{enemy};
  const HexAi::EnemyField open(board, enemies, &openP);
  for (std::size_t h = 0; h < board.hexCount(); ++h) {
    const HexIndex hex{static_cast<std::uint32_t>(h)};
    EXPECT_EQ(board.distance(enemy, hex), open.steps(hex)) << board.id(hex).text;
  }
  const HexAi::EnemyField walled(board, enemies, [](HexIndex, HexModel::Direction) { return false; });
  EXPECT_EQ(0, walled.steps(enemy));
  EXPECT_FALSE(walled.steps(ToyBoard::at(board, "C4")).has_value());
}

TEST(TacticsTest, AUnitMovesOnlyToGainSomething)
{
  const HexModel::Board board = ToyBoard::build();
  const HexIndex enemy = ToyBoard::at(board, "E5");
  const std::vector<HexIndex> enemies{enemy};
  const HexAi::EnemyField field(board, enemies, &openP);
  const HexIndex from = ToyBoard::at(board, "A1");
  const HexEngine::Reachability reach = everywhereBut(board, from);

  const std::optional<std::size_t> best = HexAi::chooseDestination(board, field, HexAi::CloseWithEnemy{}, reach, from, false,
                                                                   [&](HexIndex hex) { return hex != enemy; });
  ASSERT_TRUE(best.has_value());
  EXPECT_EQ(1, board.distance(enemy, reach.hexes[*best]));

  const HexIndex beside = reach.hexes[*best];
  const std::vector<HexIndex> only{beside};
  HexEngine::Reachability oneHex;
  oneHex.hexes = only;
  oneHex.paths = {{from, beside}};
  oneHex.costs = {HexModel::MovementPoints{2}};
  // Already as near as that hex: stay.
  EXPECT_FALSE(HexAi::chooseDestination(board, field, HexAi::CloseWithEnemy{}, oneHex, beside, false, &anywhereP).has_value());
}

TEST(TacticsTest, AnOverstackedUnitMustLeaveEvenWithNoGoal)
{
  const HexModel::Board board = ToyBoard::build();
  const HexAi::EnemyField none(board, {}, &openP);
  const HexIndex from = ToyBoard::at(board, "C3");
  const HexEngine::Reachability reach = everywhereBut(board, from);
  EXPECT_FALSE(HexAi::chooseDestination(board, none, HexAi::Hold{}, reach, from, false, &anywhereP).has_value());
  EXPECT_TRUE(HexAi::chooseDestination(board, none, HexAi::Hold{}, reach, from, true, &anywhereP).has_value());
  EXPECT_FALSE(HexAi::chooseDestination(board, none, HexAi::Hold{}, reach, from, true, [](HexIndex) { return false; }).has_value());
}
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------

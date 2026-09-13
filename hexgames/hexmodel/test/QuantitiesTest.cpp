// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
#include "hexmodel/Quantities.h"

#include <gtest/gtest.h>

#include <stdexcept>

using namespace HexModel;

TEST(QuantitiesTest, StepsInvariant)
{
  EXPECT_THROW(Steps(0, 4), std::invalid_argument);
  EXPECT_THROW(Steps(5, 4), std::invalid_argument);

  std::optional<Steps> s = Steps(4, 4);
  ASSERT_TRUE(s.has_value());
  EXPECT_EQ(4, s->current());
  s = s->reduced();
  ASSERT_TRUE(s.has_value());
  EXPECT_EQ(3, s->current());
  s = s->reduced();
  ASSERT_TRUE(s.has_value());
  EXPECT_EQ(2, s->current());
  s = s->reduced();
  ASSERT_TRUE(s.has_value());
  EXPECT_EQ(1, s->current());
  s = s->reduced();
  EXPECT_FALSE(s.has_value());
}

TEST(QuantitiesTest, MovementPointsHalves)
{
  const MovementPoints total = MovementPoints::whole(2) + MovementPoints::half();
  EXPECT_EQ(5, total.halves);
  EXPECT_LT(MovementPoints::half(), MovementPoints::whole(1));
}

TEST(QuantitiesTest, OddsRoundsTowardTheDefender)
{
  auto expectOdds = [](OddsOutcome outcome, int attacker, int defender) {
    ASSERT_TRUE(std::holds_alternative<Odds>(outcome));
    const Odds odds = std::get<Odds>(outcome);
    EXPECT_EQ(attacker, odds.attacker);
    EXPECT_EQ(defender, odds.defender);
  };

  const Odds minimum{1, 6};
  const Odds maximum{9, 1};

  expectOdds(makeOdds(Strength{3}, Strength{2}, Rounding::Defender, minimum, maximum), 1, 1);
  expectOdds(makeOdds(Strength{2}, Strength{3}, Rounding::Defender, minimum, maximum), 1, 2);
  expectOdds(makeOdds(Strength{10}, Strength{1}, Rounding::Defender, {1, 6}, {10, 1}), 10, 1);
  expectOdds(makeOdds(Strength{7}, Strength{2}, Rounding::Defender, minimum, maximum), 3, 1);

  EXPECT_TRUE(std::holds_alternative<BelowMinimum>(
      makeOdds(Strength{1}, Strength{7}, Rounding::Defender, minimum, maximum)));
  EXPECT_TRUE(std::holds_alternative<AboveMaximum>(
      makeOdds(Strength{10}, Strength{1}, Rounding::Defender, minimum, {9, 1})));

  // Rounding::Attacker mirrors: the same raw ratios now round the other way.
  expectOdds(makeOdds(Strength{3}, Strength{2}, Rounding::Attacker, minimum, maximum), 2, 1);
  expectOdds(makeOdds(Strength{2}, Strength{3}, Rounding::Attacker, minimum, maximum), 1, 1);
  expectOdds(makeOdds(Strength{7}, Strength{2}, Rounding::Attacker, minimum, maximum), 4, 1);
}

TEST(QuantitiesTest, TurnSelectorParses)
{
  EXPECT_TRUE(TurnSelector::parse("5").containsP(5));
  EXPECT_FALSE(TurnSelector::parse("5").containsP(4));

  const TurnSelector range = TurnSelector::parse("1-10");
  EXPECT_TRUE(range.containsP(1));
  EXPECT_TRUE(range.containsP(10));
  EXPECT_FALSE(range.containsP(11));

  const TurnSelector openEnded = TurnSelector::parse("11+");
  EXPECT_FALSE(openEnded.containsP(10));
  EXPECT_TRUE(openEnded.containsP(11));
  EXPECT_TRUE(openEnded.containsP(1000));

  const TurnSelector list = TurnSelector::parse("3,5,7,9");
  for (int t : {3, 5, 7, 9}) {
    EXPECT_TRUE(list.containsP(t)) << t;
  }
  for (int t : {4, 6, 8, 10}) {
    EXPECT_FALSE(list.containsP(t)) << t;
  }

  const TurnSelector twoPlus = TurnSelector::parse("2+");
  EXPECT_FALSE(twoPlus.containsP(1));
  EXPECT_TRUE(twoPlus.containsP(2));

  EXPECT_THROW(TurnSelector::parse("a-b"), std::invalid_argument);
  EXPECT_THROW(TurnSelector::parse("10-1"), std::invalid_argument);
}
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------

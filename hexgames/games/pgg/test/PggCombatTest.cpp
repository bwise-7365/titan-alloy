// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
// The CRT codes and columns, divisional integration and the order of operations, and a battle played
// through a Session: the defender chooses to retreat, the attacker routes him and advances.
// ----------------------------------------------
#include "PggTestFixture.h"

#include <gtest/gtest.h>

namespace {

  using HexModel::Direction;
  using HexModel::HexIndex;
  using HexModel::Odds;
  using HexModel::UnitId;

}  // namespace

TEST(PggCombatTest, ResultCodes)
{
  const Pgg::CrtResult split = Pgg::crtResultOf("D1*/A1");
  EXPECT_EQ(Pgg::Loss::Steps, split.defender.loss);
  EXPECT_EQ(1, split.defender.steps);
  EXPECT_EQ(1, split.attacker.steps);
  EXPECT_TRUE(split.splitP);
  EXPECT_TRUE(split.disruptsP);
  EXPECT_TRUE(Pgg::crtResultOf("Eng").engagedP);
  EXPECT_EQ(Pgg::Loss::All, Pgg::crtResultOf("De").defender.loss);
  EXPECT_EQ(2, Pgg::crtResultOf("A2").attacker.steps);
  EXPECT_FALSE(Pgg::crtResultOf("A2").disruptsP);
  EXPECT_THROW((void)Pgg::crtResultOf("DR"), std::invalid_argument);
  EXPECT_THROW((void)Pgg::crtResultOf("A1*"), std::invalid_argument);
}

TEST(PggCombatTest, OddsAndColumns)
{
  using HexModel::Strength;
  EXPECT_EQ((Odds{2, 1}), Pgg::clampedOdds(Strength{26}, Strength{9}));  // 9.4
  EXPECT_EQ((Odds{1, 3}), Pgg::clampedOdds(Strength{1}, Strength{5}));
  EXPECT_EQ((Odds{10, 1}), Pgg::clampedOdds(Strength{50}, Strength{2}));
  const auto definition = PggTest::definition();
  const HexRules::Table& table = Pgg::crtTable(*definition->rules);
  EXPECT_EQ(0u, Pgg::crtColumn(table, Odds{1, 3}));
  EXPECT_EQ(2u, Pgg::crtColumn(table, Odds{1, 1}));
  EXPECT_EQ(11u, Pgg::crtColumn(table, Odds{10, 1}));
  EXPECT_EQ("D1*/A1", table.cell(4, 4));
}

TEST(PggCombatTest, IntegrationDoublesThenOverrunHalves)
{
  const auto definition = PggTest::definition();
  const Pgg::PggPolicySet set(*definition);
  const Pgg::PggFacts& facts = set.facts();
  const auto [from, direction] = PggTest::findHexside(set, [&](HexIndex a, Direction, HexIndex b) {
    return 10 >= facts.column(a) && PggTest::plainP(set, a) && PggTest::plainP(set, b);
  });
  const HexIndex target = *definition->board->neighbour(from, direction);
  HexModel::Position position = PggTest::blank(set, 3, "german-combat", "german");
  const std::vector<UnitId> seventh{PggTest::unit(set, "s2-25-7-arm-4-10"), PggTest::unit(set, "s2-6-7-inf-2-10"),
                                    PggTest::unit(set, "s2-7-7-inf-2-10")};
  for (UnitId unit : seventh) {
    position.place(unit, from);
    Pgg::stateOf(position).side(facts.german()).entered.insert(unit);
  }
  position.place(PggTest::unit(set, "s1-64-inf-2-4-6"), target);
  const HexEngine::Ctx ctx = PggTest::ctx(set, position);  // reads `position` as it changes
  EXPECT_TRUE(set.combat().integratedP(ctx, seventh.front()));
  EXPECT_EQ(16, set.combat().assess(ctx, seventh, target, false).attack.value);  // 8 doubled
  EXPECT_EQ(8, set.combat().assess(ctx, seventh, target, true).attack.value);    // 6.55
  EXPECT_EQ(1, set.combat().defenceMultiple(ctx, seventh, target));

  position.place(PggTest::unit(set, "s2-gd-mot-4-10"), from);
  EXPECT_FALSE(set.combat().integratedP(ctx, seventh.front()));  // 7.3: a foreign unit breaks it

  HexModel::Position motorized = PggTest::blank(set, 3, "german-combat", "german");
  for (const char* id : {"s2-11-14-mot-3-10", "s2-53-14-mot-3-10", "s2-lehr-mot-3-10"}) {
    motorized.place(PggTest::unit(set, id), from);
  }
  EXPECT_TRUE(set.combat().integratedP(PggTest::ctx(set, motorized), PggTest::unit(set, "s2-11-14-mot-3-10")));
}

TEST(PggCombatTest, TerrainMultiplesAdd)
{
  const auto definition = PggTest::definition();
  const Pgg::PggPolicySet set(*definition);
  const Pgg::PggFacts& facts = set.facts();
  const auto [from, direction] = PggTest::findHexside(set, [&](HexIndex, Direction d, HexIndex to) {
    const std::optional<HexIndex> back = definition->board->neighbour(to, HexCoord::rotate(d, 3));
    return facts.woodsP(to) && back && facts.riverP(to, HexCoord::rotate(d, 3));
  });
  const HexIndex target = *definition->board->neighbour(from, direction);
  HexModel::Position position = PggTest::blank(set, 3, "german-combat", "german");
  const UnitId panzer = PggTest::unit(set, "s2-6-3-arm-4-10");
  position.place(panzer, from);
  EXPECT_EQ(3, set.combat().defenceMultiple(PggTest::ctx(set, position), {panzer}, target));  // 9.33
}

TEST(PggCombatTest, DefenderRetreatsAttackerLosesAndAdvances)
{
  const auto definition = PggTest::definition();
  const Pgg::PggPolicySet set(*definition);
  const Pgg::PggFacts& facts = set.facts();
  const auto [from, direction] = PggTest::findHexside(set, [&](HexIndex a, Direction, HexIndex b) {
    return 10 >= facts.column(a) && PggTest::plainP(set, a) && PggTest::plainP(set, b);
  });
  const HexIndex target = *definition->board->neighbour(from, direction);
  HexModel::Position position = PggTest::blank(set, 3, "german-combat", "german");
  const std::vector<UnitId> seventh{PggTest::unit(set, "s2-25-7-arm-4-10"), PggTest::unit(set, "s2-6-7-inf-2-10"),
                                    PggTest::unit(set, "s2-7-7-inf-2-10")};
  for (UnitId unit : seventh) {
    position.place(unit, from);
    Pgg::stateOf(position).side(facts.german()).entered.insert(unit);
  }
  const UnitId rifle = PggTest::unit(set, "s1-64-inf-2-4-6");  // defence 4: 16 against 4 is 4-1
  position.place(rifle, target);
  position.state(rifle).flags.revealedP = false;
  Pgg::stateOf(position).side(facts.soviet()).entered.insert(rifle);

  // Die 5 at 4-1 is D1*/A1.
  HexEngine::Session session(definition, set.policies(), position, PggTest::seedRolling(HexEngine::StreamTag::Combat, 5));
  session.apply(HexEngine::DeclareAttack{seventh, target, {}});
  EXPECT_TRUE(session.position().unit(rifle).flags.revealedP);  // 12.2

  std::vector<std::string> asked;
  for (int answered = 0; session.prompt().decisionPendingP; ++answered) {
    ASSERT_GT(12, answered);
    const HexModel::PendingDecision& pending = session.position().pending();
    std::string what = "retreat";
    if (const HexModel::GameChoice* choice = std::get_if<HexModel::GameChoice>(&pending)) {
      what = choice->verb;
    }
    asked.push_back(what + ":" + definition->rules->sides()[session.prompt().side->value].id);
    if ("result" == what && facts.soviet() == *session.prompt().side) {
      session.apply(HexEngine::DecisionAnswer{"result", "retreat"});
    } else if ("result" == what) {
      session.apply(HexEngine::DecisionAnswer{"result", "s2-25-7-arm-4-10"});
    } else if ("advance" == what && 1 == session.position().unitsAt(target).size()) {
      session.apply(HexEngine::DecisionAnswer{"advance", "done"});
    } else if ("advance" == what) {
      session.apply(HexEngine::DecisionAnswer{"advance", "s2-6-7-inf-2-10"});
    } else {
      session.apply(session.legalCommands().front());
    }
  }
  ASSERT_LE(3u, asked.size());
  EXPECT_EQ("result:soviet", asked.front());
  EXPECT_NE(asked.end(), std::find(asked.begin(), asked.end(), "result:german"));
  EXPECT_TRUE(session.position().resolution().empty());
  EXPECT_EQ(HexModel::Face::Back, session.position().unit(seventh.front()).face);
  EXPECT_EQ(std::optional<HexModel::Location>(target), session.position().unit(PggTest::unit(set, "s2-6-7-inf-2-10")).where);
  EXPECT_EQ(1, definition->board->distance(target, std::get<HexIndex>(*session.position().unit(rifle).where)));
}
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------

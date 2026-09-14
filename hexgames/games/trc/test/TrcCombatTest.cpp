// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
// The TRC table: columns, the ratio ladder and air shifts, doubling, woods, and the combat plan
// played through a Session -- the defender picks his own loss, the attacker routes the retreats.
// ----------------------------------------------
#include "TrcTestFixture.h"

#include <gtest/gtest.h>

namespace {

  using HexModel::HexIndex;
  using HexModel::Odds;
  using HexModel::UnitId;

  struct NullSink : HexEngine::EventSink {
    void onEvent(const HexEngine::Event&) override { return; }
  };

}  // namespace

TEST(TrcCombatTest, ColumnsAndLadder)
{
  using Trc::TrcCombat;
  EXPECT_EQ(0u, TrcCombat::columnOf(Odds{1, 6}));
  EXPECT_EQ(0u, TrcCombat::columnOf(Odds{1, 5}));
  EXPECT_EQ(1u, TrcCombat::columnOf(Odds{1, 4}));
  EXPECT_EQ(1u, TrcCombat::columnOf(Odds{1, 3}));
  EXPECT_EQ(2u, TrcCombat::columnOf(Odds{1, 2}));
  EXPECT_EQ(3u, TrcCombat::columnOf(Odds{1, 1}));
  EXPECT_EQ(4u, TrcCombat::columnOf(Odds{2, 1}));
  EXPECT_EQ(5u, TrcCombat::columnOf(Odds{3, 1}));
  EXPECT_EQ(6u, TrcCombat::columnOf(Odds{4, 1}));
  EXPECT_EQ(7u, TrcCombat::columnOf(Odds{6, 1}));
  EXPECT_EQ(8u, TrcCombat::columnOf(Odds{8, 1}));
  EXPECT_EQ(8u, TrcCombat::columnOf(Odds{9, 1}));
  EXPECT_EQ(14u, TrcCombat::ladder().size());
}

TEST(TrcCombatTest, StukaShiftsThreeRungsAndCitiesDouble)
{
  const auto definition = TrcTest::definition();
  const Trc::TrcPolicySet set(*definition);
  const HexIndex target = TrcTest::openGround(*definition, set.facts(), 1).front();
  const HexIndex from = TrcTest::neighbour(*definition, target, 0);
  HexModel::Position position = TrcTest::blank(*definition, 1, "axis-i1-combat", "axis");
  const UnitId attacker = TrcTest::unit(*definition, "g-ss-2-armour");     // 9
  const UnitId defender = TrcTest::unit(*definition, "r-ru-52-infantry");  // 3
  position.place(attacker, from);
  position.place(defender, target);
  const HexEngine::Ctx ctx = TrcTest::ctx(*definition, position);

  HexEngine::CombatContext combat;
  combat.attackers = {attacker};
  combat.defenders = {defender};
  combat.target = target;
  HexEngine::PrngStreams plain(1);
  EXPECT_EQ("3-1", set.combat().report(ctx, combat, plain).odds);
  combat.declared = {set.facts().stuka()};
  HexEngine::PrngStreams shifted(1);
  EXPECT_EQ("6-1", set.combat().report(ctx, combat, shifted).odds);

  EXPECT_FALSE(set.combat().doubledP(ctx, {attacker}, target));
  const HexIndex moscow = set.facts().named("Moscow");
  EXPECT_TRUE(set.combat().doubledP(ctx, {attacker}, moscow));
  const Trc::TrcCombat::Factors inCity = set.combat().factors(ctx, {attacker}, moscow, {defender});
  EXPECT_EQ(6, inCity.defence.value);
}

TEST(TrcCombatTest, WoodsTurnDrIntoContact)
{
  const auto definition = TrcTest::definition();
  const Trc::TrcPolicySet set(*definition);
  const HexIndex woods = TrcTest::hex(*definition, "J24");
  ASSERT_EQ("woods", definition->rules->hexTerrain()[definition->board->terrain(woods).value].id);
  HexIndex from = woods;
  for (int d = 0; d < HexCoord::kDirections; ++d) {
    const HexIndex near = TrcTest::neighbour(*definition, woods, d);
    if ("clear" == definition->rules->hexTerrain()[definition->board->terrain(near).value].id &&
        !set.facts().riverHexP(near)) {
      from = near;
      break;
    }
  }
  ASSERT_NE(woods, from);
  HexModel::Position position = TrcTest::blank(*definition, 1, "axis-i1-combat", "axis");
  const UnitId attacker = TrcTest::unit(*definition, "g-ss-2-armour");
  const UnitId defender = TrcTest::unit(*definition, "r-ru-52-infantry");
  position.place(attacker, from);
  position.place(defender, woods);
  const HexEngine::Ctx ctx = TrcTest::ctx(*definition, position);
  HexEngine::CombatContext combat;
  combat.attackers = {attacker};
  combat.defenders = {defender};
  combat.target = woods;
  if (set.combat().doubledP(ctx, combat.attackers, woods)) {
    GTEST_SKIP() << "J24's neighbour puts the attack on a river";
  }
  HexEngine::PrngStreams streams(TrcTest::seedRolling(HexEngine::StreamTag::Combat, 4));  // 3-1, die 4: DR
  const HexEngine::CombatReport report = set.combat().report(ctx, combat, streams);
  EXPECT_EQ("3-1", report.odds);
  EXPECT_EQ("C", report.outcome);
}

TEST(TrcCombatTest, DefenderChoosesAttackerRoutes)
{
  const auto definition = TrcTest::definition();
  const Trc::TrcPolicySet set(*definition);
  const Trc::TrcFacts& facts = set.facts();
  const HexIndex target = TrcTest::openGround(*definition, facts, 1).front();
  HexModel::Position position = TrcTest::blank(*definition, 1, "axis-i1-combat", "axis");
  const UnitId first = TrcTest::unit(*definition, "g-ge-41-armour");   // 8
  const UnitId second = TrcTest::unit(*definition, "g-ge-1-infantry");  // 4
  position.place(first, TrcTest::neighbour(*definition, target, 0));
  position.place(second, TrcTest::neighbour(*definition, target, 1));
  position.place(TrcTest::unit(*definition, "r-ru-1-armour"), target);  // 2
  position.place(TrcTest::unit(*definition, "r-ru-2-armour"), target);  // 2: 12 against 4 is 3-1

  // Die 3 at 3-1 is EX: the attacker loses one of his two, then the defender one of his two, then
  // the defender's survivor retreats two hexes routed by the attacker.
  const std::uint64_t seed = TrcTest::seedRolling(HexEngine::StreamTag::Combat, 3);
  HexEngine::Session session(definition, set.policies(), position, seed);
  session.apply(HexEngine::DeclareAttack{{first, second}, target, {}});

  std::vector<std::string> askedOf;
  for (int answered = 0; session.prompt().decisionPendingP; ++answered) {
    ASSERT_GT(10, answered);
    const HexEngine::Prompt prompt = session.prompt();
    const HexModel::PendingDecision& pending = session.position().pending();
    const std::string kind = std::holds_alternative<HexModel::ChooseLoss>(pending) ? "loss" : "retreat";
    askedOf.push_back(kind + ":" + definition->rules->sides()[prompt.side->value].id);
    session.apply(session.legalCommands().front());
  }
  ASSERT_LE(2u, askedOf.size());
  EXPECT_EQ("loss:axis", askedOf[0]);
  EXPECT_EQ("loss:russian", askedOf[1]);
  for (std::size_t i = 2; i < askedOf.size(); ++i) {
    EXPECT_EQ("retreat:axis", askedOf[i]);
  }
  EXPECT_FALSE(session.position().combatPlan().has_value());
  EXPECT_TRUE(session.position().unitsAt(target).empty());
}
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------

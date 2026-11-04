// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
// One decision at a time, on small positions: what an agent knows, where it moves, when it attacks, and
// what a drawn set-up keeps.
// ----------------------------------------------
#include "PggAgent.h"
#include "PggGame.h"
#include "PggSeen.h"
#include "PggTestFixture.h"
#include "PggUnits.h"

#include <gtest/gtest.h>

#include <map>
#include <set>
#include <vector>

namespace {

  using HexModel::Direction;
  using HexModel::HexIndex;
  using HexModel::UnitId;

  constexpr std::uint64_t kSeed = 20261104ull;

  std::pair<HexIndex, HexIndex>
  plainPair(const Pgg::PggPolicySet& set)
  {
    const auto [from, direction] = PggTest::findHexside(
        set, [&](HexIndex a, Direction, HexIndex b) { return PggTest::plainP(set, a) && PggTest::plainP(set, b); });
    return {from, *set.facts().definition().board->neighbour(from, direction)};
  }

}  // namespace

TEST(PggSeenTest, AnUntriedDivisionCountsAtTheMeanOfItsArm)
{
  const auto definition = PggTest::definition();
  const Pgg::PggPolicySet set(*definition);
  HexModel::Position position = PggTest::blank(set, 3, "german-combat", "german");
  const UnitId strong = PggTest::unit(set, "s1-161-inf-8-8-6");
  const UnitId weakA = PggTest::unit(set, "s1-110-inf-1-1-6");
  const UnitId weakB = PggTest::unit(set, "s1-108-inf-1-1-6");
  position.place(strong, plainPair(set).first);
  position.place(weakA, set.facts().sovietPool());
  position.place(weakB, set.facts().sovietPool());
  for (UnitId unit : {strong, weakA, weakB}) {
    position.state(unit).flags.revealedP = false;
  }
  const HexEngine::Ctx untried = PggTest::ctx(set, position);
  const Pgg::PggSeen hidden(set, untried);
  EXPECT_DOUBLE_EQ(10.0 / 3.0, hidden.attack(strong));
  EXPECT_DOUBLE_EQ(10.0 / 3.0, hidden.defence(strong));

  position.state(strong).flags.revealedP = true;
  const HexEngine::Ctx revealed = PggTest::ctx(set, position);
  const Pgg::PggSeen known(set, revealed);
  EXPECT_DOUBLE_EQ(8.0, known.attack(strong));
  EXPECT_DOUBLE_EQ(1.0, known.attack(weakA));  // both untried now, and both 1-1-6
}

TEST(PggAgentTest, AUnitClosesWithTheNearestEnemy)
{
  const auto definition = PggTest::definition();
  const Pgg::PggPolicySet set(*definition);
  HexModel::Position position = PggTest::blank(set, 3, "german-move1", "german");
  const UnitId panzer = PggTest::unit(set, "s2-6-3-arm-4-10");
  const HexIndex from = PggTest::hex(set, "1020");
  const HexIndex enemyHex = PggTest::hex(set, "2520");
  position.place(panzer, from);
  position.place(PggTest::unit(set, "s1-64-inf-2-4-6"), enemyHex);
  HexEngine::Session session(definition, set.policies(), position, kSeed);
  Pgg::PggAgent agent(set, set.facts().german(), HexAi::AttackNearest{});
  const HexEngine::Command command = agent.choose(session, set.facts().german());
  const HexEngine::MoveUnit* move = std::get_if<HexEngine::MoveUnit>(&command);
  ASSERT_NE(nullptr, move);
  EXPECT_EQ(panzer, move->units.front());
  EXPECT_LT(definition->board->distance(move->path.back(), enemyHex), definition->board->distance(from, enemyHex));
  session.apply(command);

  // It has moved: the phase ends.
  EXPECT_TRUE(std::holds_alternative<HexEngine::EndPhase>(agent.choose(session, set.facts().german())));
}

TEST(PggAgentTest, AnOutnumberedUnitRalliesOnItsFriends)
{
  const auto definition = PggTest::definition();
  const Pgg::PggPolicySet set(*definition);
  HexModel::Position position = PggTest::blank(set, 3, "german-move1", "german");
  const UnitId exposed = PggTest::unit(set, "s2-1-cav-4-5");
  const UnitId ally = PggTest::unit(set, "s2-6-3-arm-4-10");
  const HexIndex from = PggTest::hex(set, "1020");
  const HexIndex allyHex = PggTest::hex(set, "0620");
  const HexIndex enemyHex = PggTest::hex(set, "1320");
  ASSERT_GE(3, definition->board->distance(from, enemyHex));
  ASSERT_LT(3, definition->board->distance(from, allyHex));
  ASSERT_GE(4, definition->board->distance(from, allyHex));
  position.place(exposed, from);
  position.place(ally, allyHex);
  position.place(PggTest::unit(set, "s1-161-inf-8-8-6"), enemyHex);
  HexEngine::Session session(definition, set.policies(), position, kSeed);
  Pgg::PggAgent agent(set, set.facts().german(), HexAi::AttackOrConsolidate{});
  std::optional<HexIndex> reached;
  for (int i = 0; i < 4 && !reached; ++i) {
    const HexEngine::Command command = agent.choose(session, set.facts().german());
    const HexEngine::MoveUnit* move = std::get_if<HexEngine::MoveUnit>(&command);
    ASSERT_NE(nullptr, move);
    if (exposed == move->units.front()) {
      reached = move->path.back();
    }
    session.apply(command);
  }
  ASSERT_TRUE(reached.has_value());
  EXPECT_LT(definition->board->distance(*reached, allyHex), definition->board->distance(from, allyHex));
}

TEST(PggAgentTest, ItAttacksAtOneToOneOrBetterAsFarAsItKnows)
{
  const auto definition = PggTest::definition();
  const Pgg::PggPolicySet set(*definition);
  const auto [here, there] = plainPair(set);
  const UnitId panzer = PggTest::unit(set, "s2-6-3-arm-4-10");
  const auto choiceAgainst = [&](const std::string& counter, bool untriedP) {
    HexModel::Position position = PggTest::blank(set, 3, "german-combat", "german");
    position.place(panzer, here);
    const UnitId defender = PggTest::unit(set, counter);
    position.place(defender, there);
    if (untriedP) {
      position.state(defender).flags.revealedP = false;
      for (const char* weak : {"s1-110-inf-1-1-6", "s1-108-inf-1-1-6"}) {
        position.place(PggTest::unit(set, weak), set.facts().sovietPool());
        position.state(PggTest::unit(set, weak)).flags.revealedP = false;
      }
    }
    const HexEngine::Session session(definition, set.policies(), position, kSeed);
    Pgg::PggAgent agent(set, set.facts().german(), HexAi::AttackNearest{});
    return agent.choose(session, set.facts().german());
  };
  EXPECT_TRUE(std::holds_alternative<HexEngine::DeclareAttack>(choiceAgainst("s1-64-inf-2-4-6", false)));  // 4 against 4
  EXPECT_TRUE(std::holds_alternative<HexEngine::EndPhase>(choiceAgainst("s1-161-inf-8-8-6", false)));      // 4 against 8
  // Untried, the same 8 counts at the mean of 8, 1 and 1: the agent does not know, and attacks.
  EXPECT_TRUE(std::holds_alternative<HexEngine::DeclareAttack>(choiceAgainst("s1-161-inf-8-8-6", true)));
}

TEST(PggAgentTest, ARefusedMoveIsNotTriedAgain)
{
  const auto definition = PggTest::definition();
  const Pgg::PggPolicySet set(*definition);
  HexModel::Position position = PggTest::blank(set, 3, "german-move1", "german");
  position.place(PggTest::unit(set, "s2-6-3-arm-4-10"), PggTest::hex(set, "1020"));
  position.place(PggTest::unit(set, "s1-64-inf-2-4-6"), PggTest::hex(set, "2520"));
  const HexEngine::Session session(definition, set.policies(), position, kSeed);
  Pgg::PggAgent agent(set, set.facts().german(), HexAi::AttackNearest{});
  std::set<HexIndex> tried;
  for (int i = 0; i < 3; ++i) {
    const HexEngine::Command command = agent.choose(session, set.facts().german());
    const HexEngine::MoveUnit* move = std::get_if<HexEngine::MoveUnit>(&command);
    ASSERT_NE(nullptr, move);
    EXPECT_TRUE(tried.insert(move->path.back()).second);
    agent.refused(command, "as a test");
  }
  // Three destinations refused: the unit is left where it is, and the phase can end.
  EXPECT_TRUE(std::holds_alternative<HexEngine::EndPhase>(agent.choose(session, set.facts().german())));
}

TEST(PggSeizeTest, ADivisionMovesAsOneTowardAnUnheldCity)
{
  const auto definition = PggTest::definition();
  const Pgg::PggPolicySet set(*definition);
  HexModel::Position position = PggTest::blank(set, 3, "german-move1", "german");
  const HexIndex from = PggTest::hex(set, "1020");
  const std::vector<UnitId>& regiments = set.facts().members(*set.facts().division(PggTest::unit(set, "s2-6-3-arm-4-10")));
  ASSERT_LT(1u, regiments.size());
  for (UnitId regiment : regiments) {
    position.place(regiment, from);
  }
  const HexEngine::Session session(definition, set.policies(), position, kSeed);
  Pgg::PggAgent agent(set, set.facts().german(), HexAi::SeizeLocations{});
  const HexEngine::Command command = agent.choose(session, set.facts().german());
  const HexEngine::MoveUnit* move = std::get_if<HexEngine::MoveUnit>(&command);
  ASSERT_NE(nullptr, move);
  EXPECT_EQ(regiments.size(), move->units.size());
  // No enemy is on the map, so what draws it can only be a city: the end of the path is nearer the nearest.
  int nearestFrom = 1000;
  int nearestTo = 1000;
  for (const Pgg::VictoryHex& city : set.facts().victoryHexes()) {
    nearestFrom = std::min(nearestFrom, definition->board->distance(from, city.hex));
    nearestTo = std::min(nearestTo, definition->board->distance(move->path.back(), city.hex));
  }
  EXPECT_LT(nearestTo, nearestFrom);
}

TEST(PggSeizeTest, ASplitDivisionComesBackTogether)
{
  const auto definition = PggTest::definition();
  const Pgg::PggPolicySet set(*definition);
  HexModel::Position position = PggTest::blank(set, 3, "german-move1", "german");
  const std::vector<UnitId>& regiments = set.facts().members(*set.facts().division(PggTest::unit(set, "s2-6-3-arm-4-10")));
  const HexIndex leadFrom = PggTest::hex(set, "1020");
  const HexIndex straggler = PggTest::hex(set, "1026");
  position.place(regiments.front(), leadFrom);
  for (std::size_t i = 1; i < regiments.size(); ++i) {
    position.place(regiments[i], straggler);
  }
  HexEngine::Session session(definition, set.policies(), position, kSeed);
  Pgg::PggAgent agent(set, set.facts().german(), HexAi::SeizeLocations{});
  const HexEngine::Command first = agent.choose(session, set.facts().german());
  const HexEngine::MoveUnit* lead = std::get_if<HexEngine::MoveUnit>(&first);
  ASSERT_NE(nullptr, lead);
  ASSERT_EQ(regiments.front(), lead->units.front());
  session.apply(first);
  const HexIndex leadTo = lead->path.back();
  const HexEngine::Command second = agent.choose(session, set.facts().german());
  const HexEngine::MoveUnit* follow = std::get_if<HexEngine::MoveUnit>(&second);
  ASSERT_NE(nullptr, follow);
  EXPECT_EQ(regiments.size() - 1, follow->units.size());
  EXPECT_LT(definition->board->distance(follow->path.back(), leadTo), definition->board->distance(straggler, leadTo));
}

TEST(PggSeizeTest, TheSovietsGoForWhatTheGermansHold)
{
  const auto definition = PggTest::definition();
  const Pgg::PggPolicySet set(*definition);
  HexModel::Position position = PggTest::blank(set, 3, "soviet-move", "soviet");
  const HexIndex city = PggTest::hex(set, "0513");
  const HexIndex from = PggTest::hex(set, "1520");
  position.setControl(city, set.facts().german());
  position.place(PggTest::unit(set, "s1-64-inf-2-4-6"), from);
  const HexEngine::Session session(definition, set.policies(), position, kSeed);
  HexAi::SeizeLocations allToTheFringe;
  allToTheFringe.fringeShare = 1.0;
  Pgg::PggAgent agent(set, set.facts().soviet(), allToTheFringe);
  const HexEngine::Command command = agent.choose(session, set.facts().soviet());
  const HexEngine::MoveUnit* move = std::get_if<HexEngine::MoveUnit>(&command);
  ASSERT_NE(nullptr, move);
  EXPECT_LT(definition->board->distance(move->path.back(), city), definition->board->distance(from, city));
}

TEST(PggSeizeTest, ASeizingUnitStillAttacks)
{
  const auto definition = PggTest::definition();
  const Pgg::PggPolicySet set(*definition);
  const auto [here, there] = plainPair(set);
  HexModel::Position position = PggTest::blank(set, 3, "german-combat", "german");
  position.place(PggTest::unit(set, "s2-6-3-arm-4-10"), here);
  position.place(PggTest::unit(set, "s1-64-inf-2-4-6"), there);
  HexEngine::Session session(definition, set.policies(), position, kSeed);
  Pgg::PggAgent agent(set, set.facts().german(), HexAi::SeizeLocations{});
  // The attack is declared by the same rule as the other tactics; a seizing unit still has to fight.
  EXPECT_TRUE(std::holds_alternative<HexEngine::DeclareAttack>(agent.choose(session, set.facts().german())));
}

TEST(PggDrawnSetUpTest, ItKeepsEveryPlaceAndArmAndChangesWhoIsThere)
{
  const Pgg::PggShared shared = Pgg::loadShared(PggTest::root());
  const Pgg::PggPolicySet& set = *shared.set;
  const Pgg::PggFacts& facts = set.facts();
  const HexModel::Roster& roster = *shared.definition->roster;
  const auto shape = [&](const HexModel::Position& position) {  // per place: how many of each arm stand there
    std::map<std::string, int> counts;
    for (UnitId unit : roster.ofSide(facts.soviet())) {
      const std::optional<HexIndex> hex = Pgg::Units::hexOf(position, unit);
      const std::string where = hex ? shared.definition->board->id(*hex).text
                                    : (Pgg::Units::inSpaceP(position, unit, facts.sovietPool()) ? "pool" : "elsewhere");
      counts[where + " " + std::to_string(static_cast<int>(facts.arm(unit)))] += 1;
    }
    return counts;
  };
  const HexModel::Position first = Pgg::drawnSetUp(set, shared.start, 1);
  const HexModel::Position second = Pgg::drawnSetUp(set, shared.start, 2);
  EXPECT_EQ(shape(shared.start), shape(first));
  EXPECT_EQ(shape(shared.start), shape(second));
  EXPECT_NE(shared.start.digest(), first.digest());
  EXPECT_NE(first.digest(), second.digest());
  EXPECT_EQ(first.digest(), Pgg::drawnSetUp(set, shared.start, 1).digest());

  // The armies of 5.2 are the units now on their hexes, as many as before.
  const std::map<UnitId, Pgg::Army>& armies = Pgg::stateOf(first).armies;
  EXPECT_EQ(Pgg::stateOf(shared.start).armies.size(), armies.size());
  for (const auto& entry : armies) {
    EXPECT_TRUE(Pgg::Units::hexOf(first, entry.first).has_value()) << roster.unit(entry.first).counter.text;
  }
}
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------

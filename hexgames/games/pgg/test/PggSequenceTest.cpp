// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
// PGG's sequence of play through a Session: the phase gate, a whole empty game turn from the set-up
// to the next Soviet Movement Phase with the schedule owed and the dice rolled, disruption removal,
// overstacked units eliminated, and the markers placed and removed.
// ----------------------------------------------
#include "PggTestFixture.h"

#include <gtest/gtest.h>

namespace {

  using HexEngine::Cap;
  using HexModel::HexIndex;
  using HexModel::UnitId;

  bool
  capP(const Pgg::PggPolicySet& set, const char* phase, Cap cap)
  {
    return set.policies().phases->capsFor(set.facts().phaseNamed(phase)).test(static_cast<std::size_t>(cap));
  }

}  // namespace

TEST(PggSequenceTest, PhaseGate)
{
  const auto definition = PggTest::definition();
  const Pgg::PggPolicySet set(*definition);
  EXPECT_TRUE(capP(set, "soviet-move", Cap::RailMove));
  EXPECT_FALSE(capP(set, "german-move1", Cap::RailMove));
  EXPECT_TRUE(capP(set, "german-move2", Cap::Move));
  EXPECT_TRUE(capP(set, "german-combat", Cap::Combat));
  EXPECT_FALSE(capP(set, "set-up", Cap::Move));
  EXPECT_THROW((void)set.policies().phases->capsFor(set.facts().phaseNamed("german-turn")), std::invalid_argument);
}

TEST(PggSequenceTest, AnEmptyTurnRuns)
{
  const auto definition = PggTest::definition();
  const Pgg::PggPolicySet set(*definition);
  HexEngine::Session session(definition, set.policies(), PggTest::blank(set, 1, "set-up", "german"), 20260914ull);
  session.apply(HexEngine::GameCommand{"interdict", {"2117"}});
  session.apply(HexEngine::EndPhase{});
  EXPECT_EQ(set.facts().phaseNamed("soviet-move"), session.prompt().phase);
  EXPECT_EQ(3u, session.streams().draws(HexEngine::StreamTag::Setup));  // two army dice, the provisional die
  EXPECT_EQ(6, Pgg::stateOf(session.position()).owed.at(Pgg::Area::X).rifles);
  EXPECT_EQ(1u, Pgg::stateOf(session.position()).airInterdiction.size());

  session.apply(HexEngine::EndPhase{});  // the Soviet Movement Phase ends: the markers come off
  EXPECT_TRUE(Pgg::stateOf(session.position()).airInterdiction.empty());
  for (int stops = 0; set.facts().phaseNamed("soviet-move") != session.prompt().phase; ++stops) {
    ASSERT_GT(12, stops);
    session.apply(HexEngine::EndPhase{});
  }
  EXPECT_EQ(2, session.prompt().turn);
  EXPECT_EQ(10, Pgg::stateOf(session.position()).owed.at(Pgg::Area::X).rifles);
  EXPECT_EQ(1, Pgg::stateOf(session.position()).owed.at(Pgg::Area::W).armour);
}

TEST(PggSequenceTest, AirInterdictionStaysWestOfColumnForty)
{
  const auto definition = PggTest::definition();
  const Pgg::PggPolicySet set(*definition);
  HexEngine::Session session(definition, set.policies(), PggTest::blank(set, 1, "set-up", "german"), 1ull);
  EXPECT_THROW((void)session.apply(HexEngine::GameCommand{"interdict", {"4515"}}), std::invalid_argument);  // 13.2
  session.apply(HexEngine::GameCommand{"interdict", {"4015"}});
  EXPECT_THROW((void)session.apply(HexEngine::GameCommand{"interdict", {"4015"}}), std::invalid_argument);
}

TEST(PggSequenceTest, DisruptionComesOffInTheOwnersPhase)
{
  const auto definition = PggTest::definition();
  const Pgg::PggPolicySet set(*definition);
  HexModel::Position position = PggTest::blank(set, 2, "soviet-combat", "soviet");
  const UnitId rifle = PggTest::unit(set, "s1-64-inf-2-4-6");
  position.place(rifle, PggTest::hex(set, "3310"));
  Pgg::stateOf(position).side(set.facts().soviet()).disrupted.insert(rifle);
  HexEngine::Session session(definition, set.policies(), position, 1ull);
  session.apply(HexEngine::EndPhase{});
  EXPECT_TRUE(Pgg::stateOf(session.position()).side(set.facts().soviet()).disrupted.empty());  // 6.63
}

TEST(PggSequenceTest, OverstackedUnitsAreEliminatedAtTheEndOfMovement)
{
  const auto definition = PggTest::definition();
  const Pgg::PggPolicySet set(*definition);
  HexModel::Position position = PggTest::blank(set, 2, "soviet-move", "soviet");
  const HexIndex hex = PggTest::hex(set, "3310");
  const UnitId fourth = PggTest::unit(set, "s1-48-inf-1-4-6");
  for (const char* id : {"s1-64-inf-2-4-6", "s1-f-inf-2-4-6", "s1-275-inf-2-4-6", "s1-48-inf-1-4-6"}) {
    position.place(PggTest::unit(set, id), hex);
  }
  HexEngine::Session session(definition, set.policies(), position, 1ull);
  session.apply(HexEngine::EndPhase{});
  EXPECT_EQ(HexModel::Location(set.facts().deadPile()), *session.position().unit(fourth).where);
  EXPECT_FALSE(session.position().unit(fourth).flags.revealedP);  // 12.4
}
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------

// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
// TRC's sequence of play through a Session: the phase gate, the weather roll on entering the
// Weather Phase, supply elimination in the owner's end phase, mandatory attacks, rail capacity and
// automatic victory.
// ----------------------------------------------
#include "TrcTestFixture.h"

#include "TrcMarkers.h"
#include "TrcState.h"

#include <gtest/gtest.h>

namespace {

  using HexEngine::Cap;
  using HexModel::HexIndex;
  using HexModel::UnitId;

  bool
  capP(const Trc::TrcPolicySet& set, const HexRules::GameDefinition& definition, const char* phase, Cap cap)
  {
    return set.policies().phases->capsFor(definition.rules->phase(phase)).test(static_cast<std::size_t>(cap));
  }

}  // namespace

TEST(TrcSequenceTest, PhaseGate)
{
  const auto definition = TrcTest::definition();
  const Trc::TrcPolicySet set(*definition);
  EXPECT_TRUE(capP(set, *definition, "axis-i1-move", Cap::RailMove));
  EXPECT_FALSE(capP(set, *definition, "axis-i2-move", Cap::RailMove));
  EXPECT_TRUE(capP(set, *definition, "russian-i2-move", Cap::Move));
  EXPECT_TRUE(capP(set, *definition, "russian-i1-combat", Cap::Combat));
  EXPECT_FALSE(capP(set, *definition, "weather-phase", Cap::Move));
  EXPECT_THROW((void)set.policies().phases->capsFor(definition->rules->phase("axis-turn")), std::invalid_argument);
}

TEST(TrcSequenceTest, WeatherIsRolledOnEnteringTheWeatherPhase)
{
  const auto definition = TrcTest::definition();
  const Trc::TrcPolicySet set(*definition);
  HexEngine::Session session(definition, set.policies(), TrcTest::blank(*definition, 2, "russian-end", "russian"), 20260914ull);
  session.apply(HexEngine::EndPhase{});
  EXPECT_EQ(3, session.prompt().turn);
  EXPECT_EQ(definition->rules->phase("weather-phase"), session.prompt().phase);
  EXPECT_EQ(1u, session.streams().draws(HexEngine::StreamTag::Weather));
  EXPECT_TRUE(Trc::stateOf(session.position()).weather.has_value());
}

TEST(TrcSequenceTest, UnsuppliedUnitsAreEliminatedInTheirEndPhase)
{
  const auto definition = TrcTest::definition();
  const Trc::TrcPolicySet set(*definition);
  HexModel::Position position = TrcTest::blank(*definition, 1, "axis-end", "axis");
  const UnitId lost = TrcTest::unit(*definition, "g-ge-1-infantry");
  position.place(lost, TrcTest::openGround(*definition, set.facts(), 1).front());
  HexEngine::Session session(definition, set.policies(), position, 1ull);
  session.apply(HexEngine::EndPhase{});
  EXPECT_EQ(HexModel::Location(set.facts().pool(set.facts().axis())), *session.position().unit(lost).where);
}

TEST(TrcSequenceTest, MandatoryAttacks)
{
  const auto definition = TrcTest::definition();
  const Trc::TrcPolicySet set(*definition);
  const HexIndex hex = TrcTest::openGround(*definition, set.facts(), 1).front();
  const UnitId armour = TrcTest::unit(*definition, "g-ge-41-armour");
  const UnitId hq = TrcTest::unit(*definition, "g-ge-n-hq");

  {
    // An 8 next to a 3: it can attack, so the phase may not end until it does.
    HexModel::Position position = TrcTest::blank(*definition, 1, "axis-i1-combat", "axis");
    position.place(armour, hex);
    position.place(TrcTest::unit(*definition, "r-ru-52-infantry"), TrcTest::neighbour(*definition, hex, 0));
    HexEngine::Session session(definition, set.policies(), position, 1ull);
    EXPECT_THROW((void)session.apply(HexEngine::EndPhase{}), std::invalid_argument);
  }
  {
    // A 1 next to a 10 cannot make 1-6: it surrenders at the end of the phase (12.5).
    HexModel::Position position = TrcTest::blank(*definition, 1, "axis-i1-combat", "axis");
    position.place(hq, hex);
    position.place(TrcTest::unit(*definition, "r-gu-1g-armour-2"), TrcTest::neighbour(*definition, hex, 0));
    HexEngine::Session session(definition, set.policies(), position, 1ull);
    EXPECT_THROW((void)session.apply(HexEngine::DeclareAttack{{hq}, TrcTest::neighbour(*definition, hex, 0), {}}),
                 std::invalid_argument);
    session.apply(HexEngine::EndPhase{});
    EXPECT_EQ(HexModel::Location(set.facts().surrendered(set.facts().axis())), *session.position().unit(hq).where);
  }
}

TEST(TrcSequenceTest, RailMovementAndCapacity)
{
  const auto definition = TrcTest::definition();
  const Trc::TrcPolicySet set(*definition);
  const Trc::TrcFacts& facts = set.facts();
  const HexModel::LinkNetwork& rail = definition->board->network(facts.railNetwork());

  // Two links end to end, both land and neither a city: A - B - C.
  for (std::size_t first = 0; first < rail.linkCount(); ++first) {
    const HexModel::LinkNetwork::Link& ab = rail.links()[first];
    for (std::size_t second : rail.linksAt(ab.b)) {
      const HexModel::LinkNetwork::Link& bc = rail.links()[second];
      const HexIndex c = bc.a == ab.b ? bc.b : bc.a;
      if (second == first || c == ab.a || facts.waterP(ab.a) || facts.waterP(ab.b) || facts.waterP(c)) {
        continue;
      }
      HexModel::Position position = TrcTest::blank(*definition, 1, "axis-i1-move", "axis");
      position.setLinkOwner(facts.railNetwork(), first, facts.axis());
      position.setLinkOwner(facts.railNetwork(), second, facts.axis());
      const UnitId infantry = TrcTest::unit(*definition, "g-ge-1-infantry");
      position.place(infantry, ab.a);
      {
        HexEngine::Session session(definition, set.policies(), position, 1ull);
        const HexEngine::Reachability field = session.reachable(std::span<const UnitId>(&infantry, 1), facts.railMode());
        EXPECT_NE(field.hexes.end(), std::find(field.hexes.begin(), field.hexes.end(), c));
      }
      Trc::stateOf(position).side(facts.axis()).railMoves = 6;
      HexEngine::Session spent(definition, set.policies(), position, 1ull);
      EXPECT_TRUE(spent.reachable(std::span<const UnitId>(&infantry, 1), facts.railMode()).hexes.empty());
      return;
    }
  }
  FAIL() << "no two land rail links end to end";
}

TEST(TrcSequenceTest, AutomaticVictory)
{
  const auto definition = TrcTest::definition();
  const Trc::TrcPolicySet set(*definition);
  const HexIndex target = TrcTest::openGround(*definition, set.facts(), 1).front();
  HexModel::Position position = TrcTest::blank(*definition, 1, "axis-i1-move", "axis");
  const UnitId gd = TrcTest::unit(*definition, "g-ge-gd-armour");    // 10
  const UnitId ss = TrcTest::unit(*definition, "g-ss-1-armour");     // 10
  const UnitId corps = TrcTest::unit(*definition, "r-ru-1-infantry");  // 1
  position.place(gd, TrcTest::neighbour(*definition, target, 0));
  position.place(ss, TrcTest::neighbour(*definition, target, 1));
  position.place(corps, target);

  HexEngine::Session session(definition, set.policies(), position, 1ull);
  session.apply(HexEngine::GameCommand{"av-attack", {"g-ge-gd-armour g-ss-1-armour", definition->board->id(target).text}});
  EXPECT_EQ(HexModel::Location(set.facts().pool(set.facts().russian())), *session.position().unit(corps).where);
  EXPECT_EQ(0, std::get<HexModel::HexCount>(set.movement().allowance(session.context(), gd, set.facts().normalMode())).value);

  HexModel::Position russian = TrcTest::blank(*definition, 3, "russian-i1-move", "russian");
  russian.place(TrcTest::unit(*definition, "r-gu-1g-armour-2"), TrcTest::neighbour(*definition, target, 0));
  russian.place(TrcTest::unit(*definition, "g-ge-15-infantry"), target);
  Trc::stateOf(russian).weather = Trc::Weather::Clear;
  HexEngine::Session early(definition, set.policies(), russian, 1ull);
  EXPECT_THROW((void)early.apply(HexEngine::GameCommand{"av-attack", {"r-gu-1g-armour-2", definition->board->id(target).text}}),
               std::invalid_argument);  // 16.4
}

TEST(TrcSequenceTest, TheRulesRefuseRailInTheSecondImpulse)
{
  const auto definition = TrcTest::definition();
  const Trc::TrcPolicySet set(*definition);
  const Trc::TrcFacts& facts = set.facts();
  const HexModel::LinkNetwork& rail = definition->board->network(facts.railNetwork());
  for (std::size_t link = 0; link < rail.linkCount(); ++link) {
    const HexModel::LinkNetwork::Link& ab = rail.links()[link];
    if (facts.waterP(ab.a) || facts.waterP(ab.b)) {
      continue;
    }
    HexModel::Position position = TrcTest::blank(*definition, 1, "axis-i2-move", "axis");
    position.setLinkOwner(facts.railNetwork(), link, facts.axis());
    const UnitId infantry = TrcTest::unit(*definition, "g-ge-1-infantry");
    position.place(infantry, ab.a);
    HexEngine::Session session(definition, set.policies(), position, 1ull);
    try {
      session.apply(HexEngine::MoveUnit{{infantry}, facts.railMode(), {ab.a, ab.b}});
      FAIL() << "rail movement was allowed in the second impulse";
    } catch (const std::invalid_argument& e) {
      // The axis-i2 phase's own step refuses it, naming the rule, before the engine looks at the move.
      EXPECT_NE(std::string::npos, std::string(e.what()).find("second-impulse")) << e.what();
    }
    return;
  }
  FAIL() << "no rail link between two land hexes";
}
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------

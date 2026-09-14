// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
// Stacking, zones of control, movement, weather, supply, victory and the command grammar, each on
// a hand-built position.
// ----------------------------------------------
#include "TrcTestFixture.h"

#include "TrcFlags.h"

#include <gtest/gtest.h>

namespace {

  using HexModel::HexIndex;
  using HexModel::UnitId;

  int
  hexes(const HexModel::Budget& budget)
  {
    return std::get<HexModel::HexCount>(budget).value;
  }

}  // namespace

TEST(TrcStackingTest, SizesAndNoValue)
{
  const auto definition = TrcTest::definition();
  const Trc::TrcPolicySet set(*definition);
  const HexIndex hex = TrcTest::openGround(*definition, set.facts(), 1).front();
  const auto excessWith = [&](const std::vector<std::string>& counters) {
    HexModel::Position position = TrcTest::blank(*definition, 1, "axis-i1-move", "axis");
    for (const std::string& id : counters) {
      position.place(TrcTest::unit(*definition, id), hex);
    }
    return set.stacking().excess(TrcTest::ctx(*definition, position), hex).size();
  };
  EXPECT_EQ(0u, excessWith({"r-ru-11-infantry", "r-ru-12-infantry"}));
  EXPECT_EQ(1u, excessWith({"r-ru-11-infantry", "r-ru-12-infantry", "r-ru-16-infantry"}));
  EXPECT_EQ(0u, excessWith({"g-ge-1-infantry", "g-ge-2-infantry", "g-ge-10-infantry"}));
  EXPECT_EQ(1u, excessWith({"g-ge-1-infantry", "g-ge-2-infantry", "g-ge-10-infantry", "g-ge-26-infantry"}));
  EXPECT_EQ(1u, excessWith({"r-ru-11-infantry", "r-ru-1-armour", "r-ru-2-armour"}));  // mixed: two at most
  EXPECT_EQ(0u, excessWith({"r-ru-11-infantry", "r-ru-12-infantry", "r-ru-stavka-hq", "r-ru-stalin-stalin"}));
}

TEST(TrcZocTest, PartisanOwnHexOnly)
{
  const auto definition = TrcTest::definition();
  const Trc::TrcPolicySet set(*definition);
  const HexIndex hex = TrcTest::openGround(*definition, set.facts(), 1).front();
  HexModel::Position position = TrcTest::blank(*definition, 1, "axis-i1-move", "axis");
  position.place(TrcTest::unit(*definition, "r-wo-partisans-partisan"), hex);
  const HexEngine::Ctx ctx = TrcTest::ctx(*definition, position);
  const Trc::SideId axis = set.facts().axis();

  EXPECT_TRUE(set.zoc().enemyZocP(ctx, hex, axis, true));
  EXPECT_FALSE(set.zoc().enemyZocP(ctx, hex, axis, false));  // no control, no supply block, no forced attack
  EXPECT_FALSE(set.zoc().enemyZocP(ctx, TrcTest::neighbour(*definition, hex, 0), axis, true));
}

TEST(TrcZocTest, BlockedHexsideStopsTheZone)
{
  const auto definition = TrcTest::definition();
  const Trc::TrcPolicySet set(*definition);
  const HexModel::Board& board = *definition->board;
  for (std::size_t h = 0; h < board.hexCount(); ++h) {
    const HexIndex hex{static_cast<std::uint32_t>(h)};
    for (int d = 0; d < HexCoord::kDirections; ++d) {
      const std::optional<HexIndex> other = board.neighbour(hex, static_cast<HexModel::Direction>(d));
      if (!other || !set.facts().blockedHexsideP(hex, static_cast<HexModel::Direction>(d)) ||
          set.facts().waterP(hex) || set.facts().waterP(*other)) {
        continue;
      }
      HexModel::Position position = TrcTest::blank(*definition, 1, "axis-i1-move", "axis");
      position.place(TrcTest::unit(*definition, "r-ru-11-infantry"), hex);
      EXPECT_FALSE(set.zoc().enemyZocP(TrcTest::ctx(*definition, position), *other, set.facts().axis(), true))
          << board.id(hex).text << " -> " << board.id(*other).text;
      return;
    }
  }
  FAIL() << "the sheet has no blocked hexside between two land hexes";
}

TEST(TrcMovementTest, HqsLeadersAndPinning)
{
  const auto definition = TrcTest::definition();
  const Trc::TrcPolicySet set(*definition);
  const HexIndex hex = TrcTest::openGround(*definition, set.facts(), 1).front();
  const UnitId hq = TrcTest::unit(*definition, "g-ge-n-hq");
  const UnitId hitler = TrcTest::unit(*definition, "g-ge-hitler-hitler");
  const UnitId armour = TrcTest::unit(*definition, "g-ge-41-armour");
  const HexModel::ModeId normal = set.facts().normalMode();

  for (const char* phase : {"axis-i1-move", "axis-i2-move"}) {
    HexModel::Position position = TrcTest::blank(*definition, 1, phase, "axis");
    position.place(hq, hex);
    position.place(hitler, hex);
    position.place(armour, TrcTest::neighbour(*definition, hex, 0));
    const HexEngine::Ctx ctx = TrcTest::ctx(*definition, position);
    const bool secondP = std::string("axis-i2-move") == phase;
    EXPECT_EQ(secondP ? 7 : 0, hexes(set.movement().allowance(ctx, hq, normal))) << phase;
    EXPECT_EQ(0, hexes(set.movement().allowance(ctx, hitler, normal))) << phase;
    EXPECT_EQ(7, hexes(set.movement().allowance(ctx, armour, normal))) << phase;  // clear: full in both impulses

    position.place(TrcTest::unit(*definition, "r-ru-11-infantry"), TrcTest::neighbour(*definition, TrcTest::neighbour(*definition, hex, 0), 0));
    const HexEngine::Ctx pinned = TrcTest::ctx(*definition, position);
    EXPECT_EQ(secondP ? 0 : 7, hexes(set.movement().allowance(pinned, armour, normal))) << phase;
  }
}

TEST(TrcMovementTest, NoZoneToZone)
{
  const auto definition = TrcTest::definition();
  const Trc::TrcPolicySet set(*definition);
  const HexIndex from = TrcTest::openGround(*definition, set.facts(), 1).front();
  const HexIndex enemy = TrcTest::neighbour(*definition, from, 0);
  HexModel::Position position = TrcTest::blank(*definition, 1, "axis-i1-move", "axis");
  const UnitId armour = TrcTest::unit(*definition, "g-ge-41-armour");
  position.place(armour, from);
  position.place(TrcTest::unit(*definition, "r-ru-11-infantry"), enemy);
  const HexEngine::Ctx ctx = TrcTest::ctx(*definition, position);

  // Direction 1 keeps next to the enemy (still in its zone); direction 3 steps directly away.
  const HexEngine::EntryVerdict along = set.movement().enter(ctx, armour, from, static_cast<HexModel::Direction>(1), set.facts().normalMode());
  const HexEngine::EntryVerdict away = set.movement().enter(ctx, armour, from, static_cast<HexModel::Direction>(3), set.facts().normalMode());
  EXPECT_TRUE(std::holds_alternative<HexModel::Prohibited>(along.cost));
  EXPECT_FALSE(std::holds_alternative<HexModel::Prohibited>(away.cost));
}

TEST(TrcMovementTest, SwampIsClearInSnow)
{
  const auto definition = TrcTest::definition();
  const Trc::TrcPolicySet set(*definition);
  const HexIndex swamp = TrcTest::hex(*definition, "R23");
  ASSERT_EQ("swamp", definition->rules->hexTerrain()[definition->board->terrain(swamp).value].id);
  const UnitId armour = TrcTest::unit(*definition, "g-ge-41-armour");
  const HexModel::Position summer = TrcTest::blank(*definition, 1, "axis-i1-move", "axis");
  const HexModel::Position winter = TrcTest::blank(*definition, 5, "axis-i1-move", "axis");  // January/February
  EXPECT_TRUE(set.movement().stopsInP(TrcTest::ctx(*definition, summer), armour, swamp, set.facts().normalMode()));
  EXPECT_FALSE(set.movement().stopsInP(TrcTest::ctx(*definition, winter), armour, swamp, set.facts().normalMode()));
}

TEST(TrcWeatherTest, ChartFixedTurnsAndTheRunningDrm)
{
  using Trc::Months;
  using Trc::Weather;
  EXPECT_EQ(Weather::Clear, Trc::TrcWeather::chart(Months::MarApr, 0));
  EXPECT_EQ(Weather::LightMud, Trc::TrcWeather::chart(Months::MarApr, 1));
  EXPECT_EQ(Weather::Mud, Trc::TrcWeather::chart(Months::MarApr, 6));
  EXPECT_EQ(Weather::Snow, Trc::TrcWeather::chart(Months::MarApr, 9));
  EXPECT_EQ(Weather::Clear, Trc::TrcWeather::chart(Months::SepOct, 2));
  EXPECT_EQ(Weather::LightMud, Trc::TrcWeather::chart(Months::SepOct, 3));
  EXPECT_EQ(Months::MayJun, Trc::monthsOf(1));
  EXPECT_EQ(Months::JanFeb, Trc::monthsOf(5));
  EXPECT_EQ(1942, Trc::yearOf(5));
  EXPECT_EQ(1941, Trc::yearOf(4));

  const auto definition = TrcTest::definition();
  const Trc::TrcPolicySet set(*definition);
  struct Sink : HexEngine::EventSink {
    void onEvent(const HexEngine::Event&) override { return; }
  } sink;

  const HexModel::Position may = TrcTest::blank(*definition, 1, "weather-phase", "axis");
  HexEngine::PrngStreams fixed(7);
  const HexModel::Position rolledMay = set.weather().roll(TrcTest::ctx(*definition, may), fixed, sink);
  EXPECT_EQ(0u, fixed.draws(HexEngine::StreamTag::Weather));
  EXPECT_EQ("clear", rolledMay.flag(set.facts().axis(), Trc::Flags::kWeather).value_or(""));

  const HexModel::Position september = TrcTest::blank(*definition, 3, "weather-phase", "axis");
  const std::uint64_t seed = TrcTest::seedRolling(HexEngine::StreamTag::Weather, 6);
  HexEngine::PrngStreams streams(seed);
  const HexModel::Position rolled = set.weather().roll(TrcTest::ctx(*definition, september), streams, sink);
  EXPECT_EQ(1u, streams.draws(HexEngine::StreamTag::Weather));
  EXPECT_EQ(Weather::Mud, set.weather().current(rolled));  // a six on Sep/Oct with no DRM
  EXPECT_EQ(Trc::TrcWeather::drmChange(Weather::Mud), set.weather().drm(rolled));
}

TEST(TrcSupplyTest, CitiesRangeAndSnow)
{
  const auto definition = TrcTest::definition();
  const Trc::TrcPolicySet set(*definition);
  const HexModel::Board& board = *definition->board;
  const HexIndex berlin = set.facts().named("Berlin");
  const UnitId infantry = TrcTest::unit(*definition, "g-ge-1-infantry");

  const auto suppliedAt = [&](int turn, HexIndex hex) {
    HexModel::Position position = TrcTest::blank(*definition, turn, "axis-end", "axis");
    position.setControl(berlin, set.facts().axis());
    position.place(infantry, hex);
    HexSearch::SearchScratch scratch;
    const HexEngine::SupplyReport report =
        set.supply().trace(TrcTest::ctx(*definition, position), scratch, set.facts().axis());
    return 1u == report.supplied.size();
  };
  // A land hex six from Berlin that an unobstructed line reaches in six: supplied in summer, not in snow.
  for (std::size_t h = 0; h < board.hexCount(); ++h) {
    const HexIndex hex{static_cast<std::uint32_t>(h)};
    if (6 != board.distance(hex, berlin) || set.facts().waterP(hex) || !board.features(hex).empty() ||
        !suppliedAt(1, hex)) {
      continue;
    }
    EXPECT_FALSE(suppliedAt(5, hex)) << board.id(hex).text;
    const HexIndex far = TrcTest::openGround(*definition, set.facts(), 1).front();
    if (12 < board.distance(far, berlin)) {
      EXPECT_FALSE(suppliedAt(1, far));
    }
    return;
  }
  FAIL() << "no hex six from Berlin traces in six";
}

TEST(TrcVictoryTest, BerlinMoscowAndTheLastTurn)
{
  const auto definition = TrcTest::definition();
  const Trc::TrcPolicySet set(*definition);
  const Trc::TrcFacts& facts = set.facts();

  HexModel::Position position = TrcTest::blank(*definition, 3, "axis-i1-move", "axis");
  position.place(facts.stalin(), facts.named("Moscow"));
  EXPECT_FALSE(set.victory().check(TrcTest::ctx(*definition, position)).has_value());

  position.setControl(facts.named("Moscow"), facts.axis());
  EXPECT_FALSE(set.victory().check(TrcTest::ctx(*definition, position)).has_value());  // Stalin still there
  position.place(facts.stalin(), facts.surrendered(facts.russian()));
  EXPECT_EQ("axis-moscow-stalin", set.victory().check(TrcTest::ctx(*definition, position))->condition);

  position.setControl(facts.named("Berlin"), facts.russian());
  EXPECT_EQ("russian-berlin", set.victory().check(TrcTest::ctx(*definition, position))->condition);

  HexModel::Position after = TrcTest::blank(*definition, 26, "weather-phase", "axis");
  EXPECT_EQ("axis-berlin-held", set.victory().check(TrcTest::ctx(*definition, after))->condition);
}

TEST(TrcGrammarTest, TrcVerbsRoundTrip)
{
  const auto definition = TrcTest::definition();
  const Trc::TrcPolicySet set(*definition);
  const HexEngine::CommandGrammar& grammar = *set.policies().grammar;

  const HexEngine::Command rail = grammar.parse("rail-move", {{"units", "g-ge-1-infantry"}, {"path", "F28 F27"}});
  ASSERT_TRUE(std::holds_alternative<HexEngine::MoveUnit>(rail));
  EXPECT_EQ(set.facts().railMode(), std::get<HexEngine::MoveUnit>(rail).mode);
  EXPECT_EQ("rail-move", grammar.verb(rail));

  for (const auto& [verb, second] : {std::pair<std::string, std::string>{"sea-move", "to"}, {"paradrop", "to"},
                                     {"av-attack", "target"}}) {
    const HexEngine::Command command = grammar.parse(verb, {{"units", "g-ge-1-infantry"}, {second, "F27"}});
    EXPECT_EQ(verb, grammar.verb(command));
    const std::vector<std::pair<std::string, std::string>> args = grammar.arguments(command);
    ASSERT_EQ(2u, args.size());
    EXPECT_EQ(second, args[1].first);
    EXPECT_EQ("F27", args[1].second);
  }
  EXPECT_THROW((void)grammar.parse("game:convert-rail", {}), std::invalid_argument);
}
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------

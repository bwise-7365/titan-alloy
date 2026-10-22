// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
#include "TrcFixture.h"

#include "hexengine/Defaults.h"

#include <gtest/gtest.h>

#include <optional>
#include <string>
#include <vector>

namespace {

  using HexModel::Direction;
  using HexModel::HexIndex;

  struct Battlefield {
    HexIndex attackerHex;
    HexIndex defenderHex;
    Direction fromDefender = Direction::D0;
  };

  // Two adjacent hexes of plain ground: nothing drawn on them, no hexside between them and no
  // terrain that doubles a defender, so the odds are exactly the printed factors.
  Battlefield
  plainPair(const HexRules::GameDefinition& definition, const HexModel::Position& position)
  {
    const HexModel::Board& board = *definition.board;
    for (std::size_t h = 0; h < board.hexCount(); ++h) {
      const HexIndex hex{static_cast<std::uint32_t>(h)};
      const HexRules::Terrain& terrain = definition.rules->hexTerrain()[board.terrain(hex).value];
      if (terrain.defenceMultiplier || terrain.stopP || !board.features(hex).empty() ||
          !position.unitsAt(hex).empty() ||
          !std::holds_alternative<HexModel::MovementPoints>(terrain.moveCost)) {
        continue;
      }
      for (int d = 0; d < HexCoord::kDirections; ++d) {
        const Direction direction = static_cast<Direction>(d);
        const std::optional<HexIndex> other = board.neighbour(hex, direction);
        if (!other || !board.edge(hex, direction).empty()) {
          continue;
        }
        const HexRules::Terrain& far = definition.rules->hexTerrain()[board.terrain(*other).value];
        if (far.defenceMultiplier || far.stopP || !board.features(*other).empty() ||
            !position.unitsAt(*other).empty() ||
            !std::holds_alternative<HexModel::MovementPoints>(far.moveCost)) {
          continue;
        }
        return Battlefield{hex, *other, HexCoord::opposite(direction)};
      }
    }
    throw std::invalid_argument("OddsTableResolverTest: the TRC sheet has no plain adjacent pair");
  }

  // The seed whose first combat roll is `wanted`, found by trying seeds in order: the resolver rolls
  // for itself, so a test that wants a named row has to pick the seed that produces it.
  std::uint64_t
  seedRolling(int wanted)
  {
    for (std::uint64_t seed = 1; seed < 1000; ++seed) {
      HexEngine::PrngStreams streams(seed);
      if (wanted == HexEngine::rollDie(streams, HexEngine::StreamTag::Combat, 6)) {
        return seed;
      }
    }
    throw std::invalid_argument("OddsTableResolverTest: no seed in the first thousand rolls that");
  }

}  // namespace

TEST(OddsTableResolverTest, TrcCrt)
{
  const std::shared_ptr<const HexRules::GameDefinition> definition = TrcFixture::definition();
  HexModel::Position position = TrcFixture::scenario(*definition);

  const Battlefield field = plainPair(*definition, position);
  const HexModel::UnitId attacker = TrcFixture::unitOf(*definition, "g-ss-2-armour");   // 9-8
  const HexModel::UnitId defender = TrcFixture::unitOf(*definition, "r-ru-52-infantry");  // 3-3
  position.place(attacker, field.attackerHex);
  position.place(defender, field.defenderHex);

  const HexEngine::Ctx ctx{*definition->board, *definition->rules, *definition->roster, position};
  const HexEngine::OddsTableResolver resolver(*definition->rules);

  HexEngine::CombatContext combat;
  combat.attackers = {attacker};
  combat.target = field.defenderHex;
  combat.defenders = {defender};
  combat.attackDirections = {field.fromDefender};

  const std::uint64_t four = seedRolling(4);
  {
    HexEngine::PrngStreams streams(four);
    const HexEngine::CombatReport report = resolver.report(ctx, combat, streams);
    EXPECT_EQ("3-1", report.odds);
    EXPECT_EQ("DR", report.outcome);
    ASSERT_EQ(1u, report.dice.size());
    EXPECT_EQ(4, report.dice.front());
    EXPECT_EQ(1u, streams.draws(HexEngine::StreamTag::Combat));
  }

  // A Stuka shifts three columns in the attacker's favour: 3-1 becomes 6-1.
  const HexEngine::GameNames names(*definition->board, *definition->roster, *definition->rules);
  combat.declared = {names.modifierOf("stuka-shift")};
  {
    HexEngine::PrngStreams streams(four);
    EXPECT_EQ("6-1", resolver.report(ctx, combat, streams).odds);
  }

  // Air and artillery together never shift more than three.
  combat.declared = {names.modifierOf("stuka-shift"), names.modifierOf("artillery")};
  {
    HexEngine::PrngStreams streams(four);
    EXPECT_EQ("6-1", resolver.report(ctx, combat, streams).odds);
  }

  // Terrain doubling saturates: a defender declared doubled twice is still only doubled.
  combat.declared = {names.modifierOf("terrain-doubling")};
  std::string once;
  {
    HexEngine::PrngStreams streams(four);
    once = resolver.report(ctx, combat, streams).odds;
  }
  EXPECT_EQ("1-1", once);
  combat.declared = {names.modifierOf("terrain-doubling"), names.modifierOf("terrain-doubling")};
  {
    HexEngine::PrngStreams streams(four);
    EXPECT_EQ(once, resolver.report(ctx, combat, streams).odds);
  }
}

TEST(OddsTableResolverTest, BelowTheMinimumSurrenders)
{
  const std::shared_ptr<const HexRules::GameDefinition> definition = TrcFixture::definition();
  HexModel::Position position = TrcFixture::scenario(*definition);

  const Battlefield field = plainPair(*definition, position);
  const HexModel::UnitId attacker = TrcFixture::unitOf(*definition, "g-ge-n-hq");         // 1-7
  const HexModel::UnitId defender = TrcFixture::unitOf(*definition, "r-gu-1g-armour");  // 8-4
  position.place(attacker, field.attackerHex);
  position.place(defender, field.defenderHex);

  const HexEngine::Ctx ctx{*definition->board, *definition->rules, *definition->roster, position};
  const HexEngine::OddsTableResolver resolver(*definition->rules);

  HexEngine::CombatContext combat;
  combat.attackers = {attacker};
  combat.target = field.defenderHex;
  combat.defenders = {defender};

  HexEngine::PrngStreams streams(20260913ull);
  const HexEngine::CombatReport report = resolver.report(ctx, combat, streams);
  EXPECT_EQ("below-minimum", report.odds);
  ASSERT_EQ(1u, report.effects.size());
  EXPECT_TRUE(std::holds_alternative<HexEngine::Surrender>(report.effects.front()));
  EXPECT_EQ(0u, streams.draws(HexEngine::StreamTag::Combat));  // no die is rolled at all
}
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------

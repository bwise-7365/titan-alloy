// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
// Movement costs, zones of control, stacking and supply, each on hexes found for the purpose.
// ----------------------------------------------
#include "PggTestFixture.h"

#include "PggTerrain.h"

#include <gtest/gtest.h>

#include <algorithm>

namespace {

  using HexModel::Direction;
  using HexModel::HexIndex;
  using HexModel::UnitId;

}  // namespace

TEST(PggRulesTest, TerrainEffectsChart)
{
  const auto definition = PggTest::definition();
  const Pgg::PggPolicySet set(*definition);
  const Pgg::PggFacts& facts = set.facts();
  const HexModel::SideId german = facts.german();
  const HexModel::SideId soviet = facts.soviet();

  const auto [roadFrom, roadDirection] = PggTest::findHexside(set, [&](HexIndex from, Direction d, HexIndex to) {
    return facts.roadP(from, to) && !facts.riverP(from, d) && facts.woodsP(to);
  });
  EXPECT_EQ(2, *Pgg::terrainHalves(facts, Pgg::MoveClass::Foot, german, roadFrom, roadDirection));
  EXPECT_EQ(1, *Pgg::terrainHalves(facts, Pgg::MoveClass::Motor, german, roadFrom, roadDirection));
  EXPECT_EQ(1, *Pgg::terrainHalves(facts, Pgg::MoveClass::Leader, soviet, roadFrom, roadDirection));

  const auto [woodsFrom, woodsDirection] = PggTest::findHexside(set, [&](HexIndex from, Direction d, HexIndex to) {
    return !facts.roadP(from, to) && !facts.riverP(from, d) && facts.woodsP(to);
  });
  EXPECT_EQ(2, *Pgg::terrainHalves(facts, Pgg::MoveClass::Foot, german, woodsFrom, woodsDirection));
  EXPECT_EQ(4, *Pgg::terrainHalves(facts, Pgg::MoveClass::Motor, german, woodsFrom, woodsDirection));
  EXPECT_EQ(2, *Pgg::terrainHalves(facts, Pgg::MoveClass::Leader, soviet, woodsFrom, woodsDirection));

  const auto [riverFrom, riverDirection] = PggTest::findHexside(set, [&](HexIndex from, Direction d, HexIndex to) {
    return facts.roadP(from, to) && facts.riverP(from, d) && "clear" == facts.terrainOf(to) && !facts.majorCityP(to);
  });
  EXPECT_EQ(6, *Pgg::terrainHalves(facts, Pgg::MoveClass::Motor, german, riverFrom, riverDirection));  // 11.13
  EXPECT_EQ(4, *Pgg::terrainHalves(facts, Pgg::MoveClass::Motor, soviet, riverFrom, riverDirection));

  const auto [lakeFrom, lakeDirection] =
      PggTest::findHexside(set, [&](HexIndex, Direction, HexIndex to) { return facts.lakeP(to); });
  EXPECT_FALSE(Pgg::terrainHalves(facts, Pgg::MoveClass::Foot, german, lakeFrom, lakeDirection).has_value());
}

TEST(PggRulesTest, ZonesOfControl)
{
  const auto definition = PggTest::definition();
  const Pgg::PggPolicySet set(*definition);
  const Pgg::PggFacts& facts = set.facts();
  const auto [from, direction] = PggTest::findHexside(
      set, [&](HexIndex a, Direction, HexIndex b) { return PggTest::plainP(set, a) && PggTest::plainP(set, b); });
  const HexIndex next = *definition->board->neighbour(from, direction);
  HexModel::Position position = PggTest::blank(set, 3, "german-move1", "german");
  const UnitId panzer = PggTest::unit(set, "s2-6-3-arm-4-10");
  const UnitId rifle = PggTest::unit(set, "s1-64-inf-2-4-6");
  position.place(panzer, from);
  const HexEngine::Ctx ctx = PggTest::ctx(set, position);
  EXPECT_TRUE(set.zoc().enemyZocP(ctx, next, facts.soviet()));
  EXPECT_TRUE(set.zoc().blockedForP(ctx, next, facts.soviet(), HexRules::Purpose::Movement));

  position.place(rifle, next);
  const HexEngine::Ctx occupied = PggTest::ctx(set, position);
  EXPECT_FALSE(set.zoc().blockedForP(occupied, next, facts.soviet(), HexRules::Purpose::Supply));  // 8.15
  EXPECT_TRUE(set.zoc().blockedForP(occupied, next, facts.soviet(), HexRules::Purpose::Movement));

  Pgg::stateOf(position).side(facts.german()).disrupted.insert(panzer);
  EXPECT_FALSE(set.zoc().enemyZocP(PggTest::ctx(set, position), next, facts.soviet()));  // 8.11
}

TEST(PggRulesTest, MovementLeavesNoZoneAndHalvesWithoutSupply)
{
  const auto definition = PggTest::definition();
  const Pgg::PggPolicySet set(*definition);
  const Pgg::PggFacts& facts = set.facts();
  const auto [from, direction] = PggTest::findHexside(
      set, [&](HexIndex a, Direction, HexIndex b) { return PggTest::plainP(set, a) && PggTest::plainP(set, b); });
  HexModel::Position position = PggTest::blank(set, 3, "german-move1", "german");
  const UnitId infantry = PggTest::unit(set, "s2-6-inf-9-7");
  position.place(infantry, from);
  const HexEngine::Ctx ctx = PggTest::ctx(set, position);  // reads `position` as it changes
  EXPECT_EQ(14, std::get<HexModel::MovementPoints>(set.movement().allowance(ctx, infantry, facts.normalMode())).halves);

  Pgg::stateOf(position).side(facts.german()).unsupplied.insert(infantry);
  EXPECT_EQ(6, std::get<HexModel::MovementPoints>(set.movement().allowance(ctx, infantry, facts.normalMode())).halves);

  position.place(PggTest::unit(set, "s1-64-inf-2-4-6"), *definition->board->neighbour(from, direction));
  const HexEngine::EntryVerdict leave =
      set.movement().enter(ctx, infantry, from, HexCoord::rotate(direction, 3), facts.normalMode());
  EXPECT_TRUE(std::holds_alternative<HexModel::Prohibited>(leave.cost));  // 8.13

  HexModel::Position mechanized = PggTest::blank(set, 3, "german-move2", "german");
  mechanized.place(infantry, from);
  EXPECT_EQ(0, std::get<HexModel::MovementPoints>(
                   set.movement().allowance(PggTest::ctx(set, mechanized), infantry, facts.normalMode())).halves);
}

TEST(PggRulesTest, SovietUnitsStayOutOfTheWesternmostColumns)
{
  const auto definition = PggTest::definition();
  const Pgg::PggPolicySet set(*definition);
  const Pgg::PggFacts& facts = set.facts();
  HexModel::Position position = PggTest::blank(set, 2, "soviet-move", "soviet");
  const UnitId rifle = PggTest::unit(set, "s1-64-inf-2-4-6");
  const HexIndex third = facts.hex("0305");
  position.place(rifle, third);
  const HexEngine::Ctx ctx = PggTest::ctx(set, position);
  for (int d = 0; d < HexCoord::kDirections; ++d) {
    const std::optional<HexIndex> to = definition->board->neighbour(third, static_cast<Direction>(d));
    if (to && facts.westmostColumnsP(*to)) {
      EXPECT_TRUE(std::holds_alternative<HexModel::Prohibited>(
          set.movement().enter(ctx, rifle, third, static_cast<Direction>(d), facts.normalMode()).cost));
    }
  }
}

TEST(PggRulesTest, Stacking)
{
  const auto definition = PggTest::definition();
  const Pgg::PggPolicySet set(*definition);
  const HexIndex hex = PggTest::hex(set, "3310");
  HexModel::Position position = PggTest::blank(set, 2, "soviet-move", "soviet");
  for (const char* id : {"s1-64-inf-2-4-6", "s1-f-inf-2-4-6", "s1-275-inf-2-4-6"}) {
    position.place(PggTest::unit(set, id), hex);
  }
  EXPECT_TRUE(set.stacking().excess(PggTest::ctx(set, position), hex).empty());
  const UnitId fourth = PggTest::unit(set, "s1-48-inf-1-4-6");
  position.place(fourth, hex);
  EXPECT_EQ(std::vector<UnitId>{fourth}, set.stacking().excess(PggTest::ctx(set, position), hex));
  position.place(PggTest::unit(set, "s1-lukin-16th-army-hq-3-10"), hex);
  EXPECT_EQ(std::vector<UnitId>{fourth}, set.stacking().excess(PggTest::ctx(set, position), hex));  // 7.1
}

TEST(PggRulesTest, SovietSupplyRunsThroughALeader)
{
  const auto definition = PggTest::definition();
  const Pgg::PggPolicySet set(*definition);
  const Pgg::PggFacts& facts = set.facts();
  HexModel::Position position = PggTest::blank(set, 2, "soviet-move", "soviet");
  const UnitId lukin = PggTest::unit(set, "s1-lukin-16th-army-hq-3-10");  // radius 3
  const UnitId near = PggTest::unit(set, "s1-64-inf-2-4-6");
  const UnitId far = PggTest::unit(set, "s1-f-inf-2-4-6");
  const HexIndex leaderHex = facts.hex("4015");
  position.place(lukin, leaderHex);
  HexSearch::SearchScratch scratch;
  std::optional<HexIndex> threeAway;
  std::optional<HexIndex> fiveAway;
  for (std::size_t h = 0; h < definition->board->hexCount() && (!threeAway || !fiveAway); ++h) {
    const HexIndex hex{static_cast<std::uint32_t>(h)};
    const int distance = definition->board->distance(leaderHex, hex);
    const bool landP = !facts.lakeP(hex) && !facts.swampP(hex);
    if (landP && 3 == distance && !threeAway) {
      threeAway = hex;
    }
    if (landP && 5 == distance && !fiveAway) {
      fiveAway = hex;
    }
  }
  position.place(near, *threeAway);
  position.place(far, *fiveAway);
  const HexEngine::Ctx ctx = PggTest::ctx(set, position);
  EXPECT_TRUE(set.supply().suppliedP(ctx, scratch, lukin));
  EXPECT_TRUE(set.supply().suppliedP(ctx, scratch, near));
  EXPECT_FALSE(set.supply().suppliedP(ctx, scratch, far));  // 11.22

  Pgg::stateOf(position).side(facts.soviet()).entered.insert(far);
  EXPECT_TRUE(set.supply().suppliedP(PggTest::ctx(set, position), scratch, far));  // 11.33
}

TEST(PggRulesTest, GermanSupplyByRoadAndTheSovietMarker)
{
  const auto definition = PggTest::definition();
  const Pgg::PggPolicySet set(*definition);
  const Pgg::PggFacts& facts = set.facts();
  HexModel::Position position = PggTest::blank(set, 3, "german-move1", "german");
  const UnitId panzer = PggTest::unit(set, "s2-6-3-arm-4-10");
  const HexIndex smolensk = facts.smolensk();
  position.place(panzer, smolensk);
  HexSearch::SearchScratch scratch;
  EXPECT_TRUE(set.supply().suppliedP(PggTest::ctx(set, position), scratch, panzer));

  // Deep in the east, far from any road joined to 0120 and from the west edge.
  HexModel::Position east = PggTest::blank(set, 3, "german-move1", "german");
  east.place(panzer, facts.hex("5525"));
  const HexModel::HexIndex westmostRoad = facts.supplyRoadHex();
  Pgg::stateOf(east).sovietInterdiction = westmostRoad;
  EXPECT_FALSE(set.supply().suppliedP(PggTest::ctx(set, east), scratch, panzer));  // 13.44
}
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------

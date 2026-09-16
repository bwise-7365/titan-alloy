// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
// The PGG package through the strict value-line reader, the facts the policies read off it, the data
// the rules need and no input carries, and the 1941 scenario.
// ----------------------------------------------
#include "PggTestFixture.h"

#include "PggSchedule.h"
#include "PggUnits.h"

#include "hexrecord/Record.h"

#include <gtest/gtest.h>

#include <algorithm>

namespace {

  using HexModel::UnitId;

}  // namespace

TEST(PggPackageTest, ValueLines)
{
  using HexModel::UnitKind;
  const HexModel::Strengths german = Pgg::valueLine("9-7", UnitKind::Ground);
  EXPECT_EQ(9, german.attack->value);
  EXPECT_EQ(9, german.defence->value);
  EXPECT_EQ(14, std::get<HexModel::MovementPoints>(*german.allowance).halves);
  const HexModel::Strengths soviet = Pgg::valueLine("2-4-6", UnitKind::Ground);
  EXPECT_EQ(2, soviet.attack->value);
  EXPECT_EQ(4, soviet.defence->value);
  const HexModel::Strengths untried = Pgg::valueLine("U-6", UnitKind::Ground);
  EXPECT_FALSE(untried.attack.has_value());
  EXPECT_EQ(12, std::get<HexModel::MovementPoints>(*untried.allowance).halves);
  const HexModel::Strengths leader = Pgg::valueLine("(3)\xE2\x98\x85" "10", UnitKind::Leader);
  EXPECT_FALSE(leader.attack.has_value());
  EXPECT_EQ(3, *leader.range);
  EXPECT_EQ(3, leader.defence->value);
  EXPECT_EQ(3, *Pgg::valueLine("(3)*10", UnitKind::Leader).range);
  EXPECT_THROW((void)Pgg::valueLine("Hero RD", UnitKind::Ground), std::invalid_argument);
  EXPECT_THROW((void)Pgg::valueLine("4-", UnitKind::Ground), std::invalid_argument);
}

TEST(PggPackageTest, LoadsWithTheStrictReader)
{
  const std::vector<HexRules::PackageProblem> problems =
      HexRules::PackageLoader::check(PggTest::root() / "packages" / "xml" / "pgg.package.xml", &Pgg::valueLine);
  EXPECT_TRUE(problems.empty()) << (problems.empty() ? "" : problems.front().message);
  const std::shared_ptr<const HexRules::GameDefinition> definition = PggTest::definition();
  EXPECT_LT(150u, definition->roster->units().size());
}

TEST(PggPackageTest, FactsReadOffTheDocuments)
{
  const auto definition = PggTest::definition();
  const Pgg::PggPolicySet set(*definition);
  const Pgg::PggFacts& facts = set.facts();

  // The printed map (maps: PGG accuracy): Smolensk's blocks fill 2216 and 2217, its name is printed over 2117;
  // Kaluga is 5921 on the east edge (column 59); the railway from the west and the road from the north end in 2216.
  EXPECT_TRUE(facts.majorCityP(facts.hex("2216")));
  EXPECT_TRUE(facts.majorCityP(facts.hex("2217")));
  EXPECT_FALSE(facts.majorCityP(facts.hex("2117")));
  EXPECT_TRUE(facts.minorCityP(facts.hex("1509")));
  EXPECT_TRUE(facts.lakeP(facts.hex("1401")));
  EXPECT_TRUE(facts.westEdgeP(facts.hex("0120")));
  EXPECT_TRUE(facts.eastEdgeP(facts.hex("5921")));
  EXPECT_TRUE(facts.majorCityP(facts.hex("5921")));
  EXPECT_TRUE(facts.roadP(facts.hex("2215"), facts.hex("2216")));
  EXPECT_TRUE(facts.railLink(facts.hex("2116"), facts.hex("2216")).has_value());
  EXPECT_TRUE(facts.roadP(facts.hex("0120"), facts.hex("0219")));  // the road reaches the German supply hex (11.11)

  EXPECT_EQ(3, facts.leaderRating(PggTest::unit(set, "s1-lukin-16th-army-hq-3-10")));
  EXPECT_EQ(Pgg::MoveClass::Leader, facts.moveClass(PggTest::unit(set, "s1-lukin-16th-army-hq-3-10")));
  EXPECT_EQ(3, facts.railPoints(PggTest::unit(set, "s1-7-arm-3-10")));

  EXPECT_EQ(3u, facts.members(Pgg::Division{Pgg::DivisionKind::Panzer, 7}).size());
  EXPECT_EQ(2u, facts.members(Pgg::Division{Pgg::DivisionKind::Motorized, 18}).size());
  EXPECT_EQ(3u, facts.members(Pgg::Division{Pgg::DivisionKind::DasReich, 0}).size());
  EXPECT_EQ(PggTest::unit(set, "s2-6-inf-2-7"), facts.successor(PggTest::unit(set, "s2-6-inf-9-7")));
  EXPECT_TRUE(facts.independentRegimentP(PggTest::unit(set, "s2-gd-mot-4-10")));

  ASSERT_EQ(12u, facts.victoryHexes().size());  // ten cities and the printed "(20 ПО)" hexes 5907 and 5915
  const auto smolensk = std::find_if(facts.victoryHexes().begin(), facts.victoryHexes().end(),
                                     [&](const Pgg::VictoryHex& vp) { return vp.hex == facts.smolensk(); });
  ASSERT_NE(facts.victoryHexes().end(), smolensk);
  EXPECT_EQ(25, smolensk->points);
  ASSERT_EQ(6u, facts.victoryLevels().size());
  EXPECT_EQ(facts.german(), facts.victoryLevels()[3].winner);
  EXPECT_EQ(50, *facts.victoryLevels()[3].lowest);
}

TEST(PggPackageTest, EveryScheduledGermanDivisionHasCounters)
{
  const auto definition = PggTest::definition();
  const Pgg::PggFacts facts(*definition);
  for (const Pgg::GermanArrival& arrival : Pgg::germanSchedule()) {
    EXPECT_FALSE(facts.members(arrival.division).empty());
  }
  for (const Pgg::Division& division : facts.divisions()) {
    const bool scheduledP = std::any_of(Pgg::germanSchedule().begin(), Pgg::germanSchedule().end(),
                                        [&](const Pgg::GermanArrival& arrival) { return arrival.division == division; });
    EXPECT_TRUE(scheduledP) << "division " << division.number;
  }
}

TEST(PggPackageTest, The1941ScenarioLoadsLegally)
{
  const auto definition = PggTest::definition();
  const Pgg::PggPolicySet set(*definition);
  const HexRecord::Record record =
      HexRecord::readRecord(PggTest::root() / "games" / "pgg" / "scenario" / "pgg-1941.xml", *definition, set.policies());
  const HexModel::Position& position = record.position;
  const Pgg::PggFacts& facts = set.facts();
  const HexEngine::Ctx ctx = PggTest::ctx(set, position);

  std::size_t onMap = 0;
  for (UnitId unit : Pgg::Units::onMap(ctx, facts, facts.soviet())) {
    ++onMap;
    EXPECT_EQ(facts.sovietDivisionP(unit), !position.unit(unit).flags.revealedP);  // 12.1
  }
  EXPECT_EQ(39u, onMap);  // 28 rifle and 6 armoured divisions, 5 Leaders (5.1)
  EXPECT_TRUE(Pgg::Units::onMap(ctx, facts, facts.german()).empty());  // 2.0
  EXPECT_EQ(32u, Pgg::stateOf(position).armies.size());  // 5.2: the 13th, 16th, 19th and 20th Armies
  EXPECT_EQ(Pgg::Army::Nineteenth, Pgg::stateOf(position).armies.at(PggTest::unit(set, "s1-konev-19th-army-hq-5-10")));
  EXPECT_EQ(std::optional<int>(2), position.unit(PggTest::unit(set, "s2-5-inf-9-7")).delay);  // 16.2: turn 3
  EXPECT_EQ(std::optional<int>(0), position.unit(PggTest::unit(set, "s2-25-7-arm-4-10")).delay);
  EXPECT_FALSE(position.unit(PggTest::unit(set, "s2-5-inf-2-7")).where.has_value());
  EXPECT_EQ(set.facts().phaseNamed("set-up"), position.clock().phase);
}

// The gaps are data, not code. The sheet rebuilt from the print (maps: PGG accuracy) carries a road into 0120,
// railways to the south edge and the Victory Point hexes 5907 and 5915, and the entrance areas are the printed
// ones, so nothing is missing; a gap reappearing means the sheet lost a printed feature.
TEST(PggPackageTest, DataGapsAreReportedNotHidden)
{
  const auto definition = PggTest::definition();
  const Pgg::PggFacts facts(*definition);
  EXPECT_TRUE(facts.dataGaps().empty()) << (facts.dataGaps().empty() ? "" : facts.dataGaps().front());
  EXPECT_TRUE(facts.inAreaP(Pgg::Area::X, facts.hex("5907")));
  EXPECT_TRUE(facts.inAreaP(Pgg::Area::P6, facts.hex("1831")));
  EXPECT_TRUE(facts.railHexP(facts.hex("1831")));  // provisional area 6 is a south-edge Railroad hex
}
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------

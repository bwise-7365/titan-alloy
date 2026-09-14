// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
// The TRC package through the TRC reader, the facts the policies read off it, the data the rules
// need and the documents do not carry, and the 1941 scenario.
// ----------------------------------------------
#include "TrcTestFixture.h"

#include "TrcStacking.h"

#include "hexrecord/Record.h"

#include <gtest/gtest.h>

#include <algorithm>

TEST(TrcPackageTest, LoadsWithTheStrictReader)
{
  const std::shared_ptr<const HexRules::GameDefinition> definition = TrcTest::definition();
  EXPECT_EQ(202u, definition->roster->units().size());
  const std::vector<HexRules::PackageProblem> problems =
      HexRules::PackageLoader::check(TrcTest::root() / "packages" / "xml" / "trc.package.xml", &Trc::valueLine);
  EXPECT_TRUE(problems.empty()) << (problems.empty() ? "" : problems.front().message);
}

TEST(TrcPackageTest, FactsReadOffTheDocuments)
{
  const std::shared_ptr<const HexRules::GameDefinition> definition = TrcTest::definition();
  const Trc::TrcFacts facts(*definition);

  EXPECT_EQ(TrcTest::hex(*definition, "E31"), facts.named("Berlin"));
  EXPECT_EQ(TrcTest::hex(*definition, "R9"), facts.named("MOSCOW"));
  EXPECT_EQ(TrcTest::hex(*definition, "G23"), facts.named("Königsberg"));
  EXPECT_TRUE(facts.majorCityP(facts.named("Kiev")));
  EXPECT_FALSE(facts.majorCityP(facts.named("Kursk")));
  EXPECT_TRUE(facts.cityP(facts.named("Kursk")));
  EXPECT_EQ(3u, facts.oilWells().size());
  EXPECT_TRUE(facts.kerchP(TrcTest::hex(*definition, "KK20"), TrcTest::hex(*definition, "KK19")));

  const std::vector<Trc::SeaArea> riga = facts.seasTouching(facts.named("Riga"));
  EXPECT_NE(riga.end(), std::find(riga.begin(), riga.end(), Trc::SeaArea::Baltic));
  const std::vector<Trc::SeaArea> odessa = facts.seasTouching(facts.named("Odessa"));
  EXPECT_NE(odessa.end(), std::find(odessa.begin(), odessa.end(), Trc::SeaArea::BlackSea));

  EXPECT_EQ(Trc::Echelon::Army, facts.echelon(TrcTest::unit(*definition, "r-ru-11-infantry")));
  EXPECT_EQ(Trc::Echelon::Corps, facts.echelon(TrcTest::unit(*definition, "g-ge-41-armour")));
  EXPECT_EQ(Trc::Echelon::ArmyGroup, facts.echelon(TrcTest::unit(*definition, "g-ge-n-hq")));
  EXPECT_EQ(Trc::Nation::Finnish, facts.nation(TrcTest::unit(*definition, "g-fi-2-infantry")));
  EXPECT_EQ(Trc::Nation::Russian, facts.nation(TrcTest::unit(*definition, "r-gu-1g-armour")));
  EXPECT_TRUE(facts.noStackingValueP(TrcTest::unit(*definition, "g-ss-res-infantry")));
  EXPECT_TRUE(facts.ownEdgeP(facts.axis(), TrcTest::hex(*definition, "A33")));
  EXPECT_TRUE(facts.ownEdgeP(facts.russian(), TrcTest::hex(*definition, "QQ10")));
  EXPECT_TRUE(facts.southEntryP(TrcTest::hex(*definition, "QQ10")));
  EXPECT_FALSE(facts.southEntryP(TrcTest::hex(*definition, "QQ20")));
}

// The gaps are data, not code: the day the sheet gains country membership, or moves Riga, Helsinki
// and Sevastopol onto land, this test fails and the rules that read them come to life (24.0
// Hungary, 19.1, 17.3, the 1945 objectives; sea movement to and from those ports).
TEST(TrcPackageTest, DataGapsAreReportedNotHidden)
{
  const std::shared_ptr<const HexRules::GameDefinition> definition = TrcTest::definition();
  const Trc::TrcFacts facts(*definition);
  ASSERT_EQ(2u, facts.dataGaps().size());
  EXPECT_NE(std::string::npos, facts.dataGaps()[0].find("F17 RIGA"));
  EXPECT_NE(std::string::npos, facts.dataGaps()[0].find("C14 HELSINKI"));
  EXPECT_NE(std::string::npos, facts.dataGaps()[0].find("KK23 SEVASTOPOL"));
  EXPECT_NE(std::string::npos, facts.dataGaps()[1].find("countries"));
}

TEST(TrcPackageTest, The1941ScenarioLoadsLegally)
{
  const std::shared_ptr<const HexRules::GameDefinition> definition = TrcTest::definition();
  const Trc::TrcPolicySet set(*definition);
  const HexRecord::Record record = HexRecord::readRecord(TrcTest::root() / "games" / "trc" / "scenario" / "trc-1941.xml",
                                                         *definition, set.policies());
  const HexModel::Position& position = record.position;
  const HexEngine::Ctx ctx = TrcTest::ctx(*definition, position);

  std::size_t onMap = 0;
  for (std::size_t h = 0; h < definition->board->hexCount(); ++h) {
    const HexModel::HexIndex hex{static_cast<std::uint32_t>(h)};
    if (position.unitsAt(hex).empty()) {
      continue;
    }
    onMap += position.unitsAt(hex).size();
    EXPECT_FALSE(set.facts().waterP(hex)) << definition->board->id(hex).text;
    EXPECT_TRUE(set.stacking().excess(ctx, hex).empty()) << definition->board->id(hex).text;
  }
  EXPECT_EQ(94u, onMap);  // 98 set-up counters less three Stukas and the Special Rail army in their boxes
  EXPECT_EQ(set.facts().axis(), position.control(set.facts().named("Berlin")));
  EXPECT_EQ(set.facts().russian(), position.control(set.facts().named("Moscow")));
  EXPECT_EQ(Trc::Weather::Clear, set.weather().current(position));
  EXPECT_EQ(0, set.weather().drm(position));
}
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------

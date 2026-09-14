// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
// The first golden-record system test: the TRC engine smoke script replayed against the golden the
// cli recorded from it. A failure here is either a real change in the engine's behaviour or a
// change that has to be blessed; goldens are never edited by hand.
// ----------------------------------------------
#include "TrcRecordFixture.h"

#include "hexrecord/GoldenCompare.h"

#include <gtest/gtest.h>

#include <string>

TEST(TrcSmokeGoldenTest, ScriptMatchesItsGolden)
{
  const std::shared_ptr<const HexRules::GameDefinition> definition = TrcRecord::definition();
  const HexEngine::DefaultPolicySet defaults(*definition, HexEngine::GameSteps::Withheld);

  const HexRecord::GoldenReport report = HexRecord::compareWithGolden(
      TrcRecord::script(), TrcRecord::golden(), definition, defaults.policies(),
      *defaults.policies().grammar);

  EXPECT_TRUE(report.matchP) << HexRecord::formatGoldenReport(report, "trc", "trc-test.golden");
  EXPECT_FALSE(report.first.has_value());
  EXPECT_EQ(HexRecord::actualPathFor(TrcRecord::golden()), report.actualWritten);

  // On engine defaults TRC's own step behaviours are withheld by name; the engine's are not.
  EXPECT_TRUE(defaults.steps().withheld().contains("roll-weather"));
  EXPECT_FALSE(defaults.steps().withheld().contains("stacking-repair"));
  // The engine's own "refuse" is withheld too: its TRC steps name rail-move, a verb the engine
  // grammar lacks, and the reason says so.
  ASSERT_TRUE(defaults.steps().withheld().contains("refuse"));
  EXPECT_NE(std::string::npos, defaults.steps().withheld().at("refuse").find("rail-move"));
}

TEST(TrcSmokeGoldenTest, ScriptIsAlsoASave)
{
  const std::shared_ptr<const HexRules::GameDefinition> definition = TrcRecord::definition();
  const HexEngine::DefaultPolicySet defaults(*definition, HexEngine::GameSteps::Withheld);

  // The golden's own <units>, <control> and <sides> sections describe the position the engine ends
  // in; replaying the script from the scenario has to reach exactly that position.
  const HexRecord::Record ended =
      HexRecord::readRecord(TrcRecord::golden(), *definition, defaults.policies());
  const HexRecord::Record script =
      HexRecord::readRecord(TrcRecord::script(), *definition, defaults.policies());

  HexEngine::Session session = HexRecord::sessionFor(script, definition, defaults.policies());
  HexRecord::playRecord(session, script);

  EXPECT_EQ(ended.position.digest(), session.position().digest());
}
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------

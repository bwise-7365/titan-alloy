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
  const HexEngine::DefaultPolicySet defaults(*definition);

  const HexRecord::GoldenReport report = HexRecord::compareWithGolden(
      TrcRecord::script(), TrcRecord::golden(), definition, defaults.policies(),
      *defaults.policies().grammar);

  EXPECT_TRUE(report.matchP) << HexRecord::formatGoldenReport(report, "trc", "trc-test.golden");
  EXPECT_FALSE(report.first.has_value());
  EXPECT_EQ(HexRecord::actualPathFor(TrcRecord::golden()), report.actualWritten);
}

TEST(TrcSmokeGoldenTest, ScriptIsAlsoASave)
{
  const std::shared_ptr<const HexRules::GameDefinition> definition = TrcRecord::definition();
  const HexEngine::DefaultPolicySet defaults(*definition);

  // The golden's own <units>, <control> and <sides> sections describe the position the engine ends
  // in; replaying the script from the scenario has to reach exactly that position.
  const HexRecord::Record ended =
      HexRecord::readRecord(TrcRecord::golden(), *definition, *defaults.policies().grammar);
  const HexRecord::Record script =
      HexRecord::readRecord(TrcRecord::script(), *definition, *defaults.policies().grammar);

  HexEngine::Session session = HexRecord::sessionFor(script, definition, defaults.policies());
  HexRecord::playRecord(session, script);

  EXPECT_EQ(ended.position.digest(), session.position().digest());
}
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------

// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
// The long TRC golden: the 1941 scenario through a whole game turn (label long).
// ----------------------------------------------
#include "TrcTestFixture.h"

#include "hexrecord/GoldenCompare.h"
#include "hexrecord/Record.h"

#include <gtest/gtest.h>

TEST(TrcGoldenLongTest, FullTurn)
{
  const std::filesystem::path dir = TrcTest::root() / "games" / "trc" / "golden";
  const std::shared_ptr<const HexRules::GameDefinition> definition = TrcTest::definition();
  const Trc::TrcPolicySet set(*definition);
  const HexRecord::GoldenReport report =
      HexRecord::compareWithGolden(dir / "full-turn.script.xml", dir / "full-turn.golden.xml", definition,
                                   set.policies(), *set.policies().grammar);
  EXPECT_TRUE(report.matchP) << HexRecord::formatGoldenReport(report, "trc", "full-turn");
}
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------

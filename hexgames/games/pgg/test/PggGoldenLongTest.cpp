// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
// The long PGG golden: the 1941 scenario through Game-Turn One (label long).
// ----------------------------------------------
#include "PggTestFixture.h"

#include "hexrecord/GoldenCompare.h"
#include "hexrecord/Record.h"

#include <gtest/gtest.h>

TEST(PggGoldenLongTest, FullTurn)
{
  const std::filesystem::path dir = PggTest::root() / "games" / "pgg" / "golden";
  const std::shared_ptr<const HexRules::GameDefinition> definition = PggTest::definition();
  const Pgg::PggPolicySet set(*definition);
  const HexRecord::GoldenReport report =
      HexRecord::compareWithGolden(dir / "full-turn.script.xml", dir / "full-turn.golden.xml", definition,
                                   set.policies(), *set.policies().grammar);
  EXPECT_TRUE(report.matchP) << HexRecord::formatGoldenReport(report, "pgg", "full-turn");
}
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------

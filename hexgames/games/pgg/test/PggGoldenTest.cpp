// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
// The PGG golden records: each script in games/pgg/golden replayed with the PGG policy set and
// compared byte for byte with the golden hexgames_cli --record wrote from it. Goldens are never
// edited by hand; a failure is a behaviour change to explain or to bless.
// ----------------------------------------------
#include "PggTestFixture.h"

#include "hexrecord/GoldenCompare.h"
#include "hexrecord/Record.h"

#include <gtest/gtest.h>

namespace {

  void
  expectGolden(const std::string& slug)
  {
    const std::filesystem::path dir = PggTest::root() / "games" / "pgg" / "golden";
    const std::shared_ptr<const HexRules::GameDefinition> definition = PggTest::definition();
    const Pgg::PggPolicySet set(*definition);
    const HexRecord::GoldenReport report =
        HexRecord::compareWithGolden(dir / (slug + ".script.xml"), dir / (slug + ".golden.xml"), definition,
                                     set.policies(), *set.policies().grammar);
    EXPECT_TRUE(report.matchP) << HexRecord::formatGoldenReport(report, "pgg", slug);
    return;
  }

}  // namespace

TEST(PggGoldenTest, UntriedReveal)
{
  expectGolden("untried-reveal");
}

TEST(PggGoldenTest, Overrun)
{
  expectGolden("overrun");
}

TEST(PggGoldenTest, CombatSplit)
{
  expectGolden("combat-split");
}

TEST(PggGoldenTest, Supply)
{
  expectGolden("supply");
}

TEST(PggGoldenTest, Interdiction)
{
  expectGolden("interdiction");
}
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------

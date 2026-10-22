// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
// The TRC golden records: each script in games/trc/golden replayed with the TRC policy set and
// compared byte for byte with the golden hexgames_cli --record wrote from it. Goldens are never
// edited by hand; a failure is a behaviour change to explain or to bless.
// ----------------------------------------------
#include "TrcTestFixture.h"

#include "hexrecord/GoldenCompare.h"
#include "hexrecord/Record.h"

#include <gtest/gtest.h>

namespace {

  void
  expectGolden(const std::string& slug)
  {
    const std::filesystem::path dir = TrcTest::root() / "games" / "trc" / "golden";
    const std::shared_ptr<const HexRules::GameDefinition> definition = TrcTest::definition();
    const Trc::TrcPolicySet set(*definition);
    const HexRecord::GoldenReport report =
        HexRecord::compareWithGolden(dir / (slug + ".script.xml"), dir / (slug + ".golden.xml"), definition,
                                     set.policies(), *set.policies().grammar);
    EXPECT_TRUE(report.matchP) << HexRecord::formatGoldenReport(report, "trc", slug);
    return;
  }

}  // namespace

TEST(TrcGoldenTest, RailMove)
{
  expectGolden("rail-move");
}

TEST(TrcGoldenTest, CombatCrt)
{
  expectGolden("combat-crt");
}

TEST(TrcGoldenTest, Retreat)
{
  expectGolden("retreat");
}

TEST(TrcGoldenTest, Weather)
{
  expectGolden("weather");
}

TEST(TrcGoldenTest, Supply)
{
  expectGolden("supply");
}
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------

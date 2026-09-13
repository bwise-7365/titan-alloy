// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
#include "hexrules/Ledger.h"

#include <gtest/gtest.h>

using namespace HexRules;

TEST(LedgerTest, Consistency)
{
  const std::vector<std::string> ruleIds = {"a", "b", "c", "d"};

  {
    const std::vector<LedgerEntry> ledger = {
        {"a", Implemented{"Trc::A"}},
        {"b", Common{EngineFeature::Stacking}},
        {"c", OutOfScope{"not modelled"}},
        {"d", OptionalNotImplemented{}},
    };
    const std::vector<std::string_view> claimed = {"a"};  // a policy claims rule "a" is implemented
    const LedgerReport report = checkLedger(ledger, ruleIds, claimed);
    EXPECT_TRUE(report.consistentP());
    EXPECT_EQ(1u, report.outOfScopeCount);
  }
  {
    // "d" missing from the ledger.
    const std::vector<LedgerEntry> ledger = {
        {"a", Implemented{"Trc::A"}},
        {"b", Common{EngineFeature::Stacking}},
        {"c", OutOfScope{"not modelled"}},
    };
    const LedgerReport report = checkLedger(ledger, ruleIds, {});
    EXPECT_FALSE(report.consistentP());
    ASSERT_EQ(1u, report.missingFromLedger.size());
    EXPECT_EQ("d", report.missingFromLedger[0]);
  }
  {
    // "z" names no rule in the xml.
    const std::vector<LedgerEntry> ledger = {
        {"a", Implemented{"Trc::A"}},
        {"b", Common{EngineFeature::Stacking}},
        {"c", OutOfScope{"not modelled"}},
        {"d", OptionalNotImplemented{}},
        {"z", OptionalNotImplemented{}},
    };
    const LedgerReport report = checkLedger(ledger, ruleIds, {});
    ASSERT_EQ(1u, report.unknownInLedger.size());
    EXPECT_EQ("z", report.unknownInLedger[0]);
  }
  {
    // "a" entered twice.
    const std::vector<LedgerEntry> ledger = {
        {"a", Implemented{"Trc::A"}},
        {"a", Implemented{"Trc::A2"}},
        {"b", Common{EngineFeature::Stacking}},
        {"c", OutOfScope{"not modelled"}},
        {"d", OptionalNotImplemented{}},
    };
    const std::vector<std::string_view> claimed = {"a"};
    const LedgerReport report = checkLedger(ledger, ruleIds, claimed);
    ASSERT_EQ(1u, report.duplicateEntries.size());
    EXPECT_EQ("a", report.duplicateEntries[0]);
  }
  {
    // "a" implemented but no policy claims the symbol.
    const std::vector<LedgerEntry> ledger = {
        {"a", Implemented{"Trc::A"}},
        {"b", Common{EngineFeature::Stacking}},
        {"c", OutOfScope{"not modelled"}},
        {"d", OptionalNotImplemented{}},
    };
    const LedgerReport report = checkLedger(ledger, ruleIds, {});
    ASSERT_EQ(1u, report.unclaimedImplemented.size());
    EXPECT_EQ("a", report.unclaimedImplemented[0]);
  }
}
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------

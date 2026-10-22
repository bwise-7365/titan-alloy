// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
// Every prose <rule @id> of the TRC rules document is accounted for exactly once, every rule the
// ledger calls implemented is claimed by a TRC policy, and at most eight are out of scope, each
// with its reason.
// ----------------------------------------------
#include "TrcTestFixture.h"

#include "TrcLedger.h"
#include "TrcPolicySet.h"

#include <gtest/gtest.h>

#include <string>
#include <vector>

TEST(TrcLedgerTest, EveryRuleIsAccountedFor)
{
  const std::shared_ptr<const HexRules::GameDefinition> definition = TrcTest::definition();
  const Trc::TrcPolicySet set(*definition);

  std::vector<std::string> ruleIds;
  for (const HexRules::ProseRule& rule : definition->rules->proseRules()) {
    ruleIds.push_back(rule.id.text);
  }
  ASSERT_EQ(54u, ruleIds.size());

  const std::vector<std::string_view> claimed = set.claims();
  const HexRules::LedgerReport report = HexRules::checkLedger(Trc::ledger(), ruleIds, claimed);
  EXPECT_TRUE(report.missingFromLedger.empty()) << report.missingFromLedger.front();
  EXPECT_TRUE(report.unknownInLedger.empty()) << report.unknownInLedger.front();
  EXPECT_TRUE(report.duplicateEntries.empty()) << report.duplicateEntries.front();
  EXPECT_TRUE(report.unclaimedImplemented.empty()) << report.unclaimedImplemented.front();
  EXPECT_TRUE(report.consistentP());
  EXPECT_GE(8u, report.outOfScopeCount);
}

TEST(TrcLedgerTest, OutOfScopeSaysWhy)
{
  for (const HexRules::LedgerEntry& entry : Trc::ledger()) {
    if (const HexRules::OutOfScope* out = std::get_if<HexRules::OutOfScope>(&entry.coverage)) {
      EXPECT_LT(20u, out->why.size()) << entry.ruleId;
    }
  }
}

TEST(TrcLedgerTest, EveryClaimNamesARule)
{
  const std::shared_ptr<const HexRules::GameDefinition> definition = TrcTest::definition();
  const Trc::TrcPolicySet set(*definition);
  for (std::string_view claim : set.claims()) {
    bool knownP = false;
    for (const HexRules::ProseRule& rule : definition->rules->proseRules()) {
      knownP = knownP || claim == rule.id.text;
    }
    EXPECT_TRUE(knownP) << claim;
  }
}
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------

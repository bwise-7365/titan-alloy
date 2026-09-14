// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
// Every prose <rule @id> of the PGG rules document is accounted for exactly once, every rule the
// ledger calls implemented is claimed by a PGG part or a registered rules step, and at most six are
// out of scope, each with its reason.
// ----------------------------------------------
#include "PggTestFixture.h"

#include "PggLedger.h"

#include <gtest/gtest.h>

#include <string>
#include <vector>

TEST(PggLedgerTest, EveryRuleIsAccountedFor)
{
  const std::shared_ptr<const HexRules::GameDefinition> definition = PggTest::definition();
  const Pgg::PggPolicySet set(*definition);

  std::vector<std::string> ruleIds;
  for (const HexRules::ProseRule& rule : definition->rules->proseRules()) {
    ruleIds.push_back(rule.id.text);
  }
  ASSERT_EQ(47u, ruleIds.size());

  const std::vector<std::string_view> claimed = set.claims();
  const HexRules::LedgerReport report = HexRules::checkLedger(Pgg::ledger(), ruleIds, claimed);
  EXPECT_TRUE(report.missingFromLedger.empty()) << report.missingFromLedger.front();
  EXPECT_TRUE(report.unknownInLedger.empty()) << report.unknownInLedger.front();
  EXPECT_TRUE(report.duplicateEntries.empty()) << report.duplicateEntries.front();
  EXPECT_TRUE(report.unclaimedImplemented.empty()) << report.unclaimedImplemented.front();
  EXPECT_TRUE(report.consistentP());
  EXPECT_GE(6u, report.outOfScopeCount);
}

TEST(PggLedgerTest, OutOfScopeSaysWhy)
{
  for (const HexRules::LedgerEntry& entry : Pgg::ledger()) {
    if (const HexRules::OutOfScope* out = std::get_if<HexRules::OutOfScope>(&entry.coverage)) {
      EXPECT_LT(20u, out->why.size()) << entry.ruleId;
    }
  }
}

TEST(PggLedgerTest, EveryClaimNamesARule)
{
  const std::shared_ptr<const HexRules::GameDefinition> definition = PggTest::definition();
  const Pgg::PggPolicySet set(*definition);
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

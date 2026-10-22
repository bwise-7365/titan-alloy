// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
#include "hexrules/Ledger.h"

#include <algorithm>
#include <map>

namespace HexRules {

  bool
  LedgerReport::consistentP() const
  {
    return missingFromLedger.empty() && unknownInLedger.empty() && duplicateEntries.empty() &&
           unclaimedImplemented.empty();
  }

  LedgerReport
  checkLedger(std::span<const LedgerEntry> ledger, std::span<const std::string> ruleIdsInXml,
              std::span<const std::string_view> claimedRuleIds)
  {
    LedgerReport report;
    std::map<std::string, int> seenCount;

    for (const LedgerEntry& entry : ledger) {
      const std::string id(entry.ruleId);
      const int count = ++seenCount[id];
      if (2 == count) {
        report.duplicateEntries.push_back(id);
      }

      const bool inXml = std::find(ruleIdsInXml.begin(), ruleIdsInXml.end(), id) != ruleIdsInXml.end();
      if (!inXml) {
        report.unknownInLedger.push_back(id);
      }

      if (std::holds_alternative<Implemented>(entry.coverage)) {
        const bool claimed =
            std::find(claimedRuleIds.begin(), claimedRuleIds.end(), entry.ruleId) != claimedRuleIds.end();
        if (!claimed) {
          report.unclaimedImplemented.push_back(id);
        }
      }
      if (std::holds_alternative<OutOfScope>(entry.coverage)) {
        ++report.outOfScopeCount;
      }
    }

    for (const std::string& id : ruleIdsInXml) {
      if (seenCount.end() == seenCount.find(id)) {
        report.missingFromLedger.push_back(id);
      }
    }

    return report;
  }

}  // namespace HexRules
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------

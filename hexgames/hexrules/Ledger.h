// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
// The rule coverage ledger: every prose <rule @id> of a game is accounted for in C++ -- implemented
// by a named symbol, covered by common engine behaviour, or explicitly out of scope -- and a test
// per game keeps the ledger and the XML in step in both directions.
// ----------------------------------------------
#pragma once
#include "hexmodel/Ids.h"

#include <span>
#include <string_view>
#include <variant>
#include <vector>

namespace HexRules {

  enum class EngineFeature : std::uint8_t {
    TerrainTable, HexsideTable, SixHexZoc, MovementWalk, BoundedTrace, OddsTable, Modifiers,
    AdjacentRetreat, Stacking, PhaseTree, Randomizers, Weather, Victory, Spaces, Regions, Networks
  };

  struct Implemented { std::string_view symbol; };        // "Trc::KerchStraitPolicy"
  struct Common { EngineFeature feature; };
  struct OutOfScope { std::string_view why; };
  struct OptionalNotImplemented {};
  using Coverage = std::variant<Implemented, Common, OutOfScope, OptionalNotImplemented>;

  struct LedgerEntry {
    std::string_view ruleId;
    Coverage coverage;
  };

  // Checks a ledger against the rule ids of a loaded RuleSet and the claims of the game's policies.
  // Returns the problems found (empty means consistent), each naming the rule id.
  struct LedgerReport {
    std::vector<std::string> missingFromLedger;   // rule ids in the XML with no entry
    std::vector<std::string> unknownInLedger;     // entries naming no rule
    std::vector<std::string> duplicateEntries;
    std::vector<std::string> unclaimedImplemented;  // Implemented entries no policy claims
    std::size_t outOfScopeCount = 0;
    bool consistentP() const;
  };

  LedgerReport checkLedger(std::span<const LedgerEntry> ledger, std::span<const std::string> ruleIdsInXml,
                           std::span<const std::string_view> claimedRuleIds);

}  // namespace HexRules
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------

// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
// The PGG rule coverage ledger: every prose <rule @id> of game_rules/xml/panzergruppe-guderian.xml,
// once. pgg_ledger_test keeps it and the document in step in both directions.
// ----------------------------------------------
#pragma once
#include "hexrules/Ledger.h"

#include <span>

namespace Pgg {

  std::span<const HexRules::LedgerEntry> ledger();

}  // namespace Pgg
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------

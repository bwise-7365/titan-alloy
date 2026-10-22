// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
// The TRC rule coverage ledger: one entry per prose <rule @id> of
// game_rules/xml/the-russian-campaign.xml. trc_ledger_test checks it against the document and
// against TrcPolicySet::claims() in both directions.
// ----------------------------------------------
#pragma once
#include "hexrules/Ledger.h"

#include <span>

namespace Trc {

  std::span<const HexRules::LedgerEntry> ledger();

}  // namespace Trc
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------

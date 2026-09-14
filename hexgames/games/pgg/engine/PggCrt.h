// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
// The Combat Results Table's codes (9.6, crt.txt) as values: what the defender and the attacker
// suffer, whether the result is Engaged, a split result, or a defender result marked "*" that
// Disrupts an overrun defender. Odds are reduced in the defender's favour (9.4); worse than 1-3 is
// read at 1-3 and better than 10-1 at 10-1.
// ----------------------------------------------
#pragma once
#include "hexmodel/Quantities.h"
#include "hexrules/RuleSet.h"

#include <cstdint>
#include <string>
#include <string_view>

namespace Pgg {

  enum class Loss : std::uint8_t { None, Steps, All };

  struct SideResult {
    Loss loss = Loss::None;
    int steps = 0;  // Loss::Steps only: lose that many steps, or retreat that many hexes
  };

  struct CrtResult {
    std::string code;  // the table's own code, "void" when no strength attacked, "De" when none defended
    SideResult defender;
    SideResult attacker;
    bool engagedP = false;   // Eng: each side loses one step, no retreat, no advance (9.67)
    bool splitP = false;     // D1*/A1: the defender's result first, then the attacker's (9.66)
    bool disruptsP = false;  // "*": an overrun defender that loses or retreats is Disrupted (6.61)
  };

  // Throws std::invalid_argument naming a code the table does not use.
  CrtResult crtResultOf(std::string_view code);

  const HexRules::Table& crtTable(const HexRules::RuleSet&);  // resolver "crt", 6 x 12
  HexModel::Odds clampedOdds(HexModel::Strength attack, HexModel::Strength defence);
  std::string oddsText(HexModel::Odds);
  std::size_t crtColumn(const HexRules::Table&, HexModel::Odds);  // the column whose label is the odds

}  // namespace Pgg
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------

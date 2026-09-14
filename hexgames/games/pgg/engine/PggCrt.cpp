// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
#include "PggCrt.h"

#include <stdexcept>

namespace Pgg {

  namespace {

    [[noreturn]] void
    refuse(std::string_view code)
    {
      throw std::invalid_argument("Pgg::crtResultOf: '" + std::string(code) + "' is not a PGG combat result");
    }

    // "A1", "D2*", "Ae", "De": one side's half of a result.
    SideResult
    halfOf(std::string_view half, char side, bool& disruptsP, std::string_view code)
    {
      if (half.size() < 2 || side != half[0]) {
        refuse(code);
      }
      std::string_view rest = half.substr(1);
      if (rest.ends_with("*")) {
        if ('D' != side) {
          refuse(code);
        }
        disruptsP = true;
        rest.remove_suffix(1);
      }
      if ("e" == rest) {
        return SideResult{Loss::All, 0};
      }
      if ("1" == rest || "2" == rest) {
        return SideResult{Loss::Steps, rest[0] - '0'};
      }
      refuse(code);
    }

  }  // namespace

  CrtResult
  crtResultOf(std::string_view code)
  {
    CrtResult out;
    out.code = std::string(code);
    if ("void" == code) {
      return out;
    }
    if ("Eng" == code) {
      out.engagedP = true;
      out.defender = SideResult{Loss::Steps, 1};
      out.attacker = SideResult{Loss::Steps, 1};
      return out;
    }
    const std::size_t slash = code.find('/');
    if (std::string_view::npos != slash) {
      out.splitP = true;
      out.defender = halfOf(code.substr(0, slash), 'D', out.disruptsP, code);
      out.attacker = halfOf(code.substr(slash + 1), 'A', out.disruptsP, code);
      return out;
    }
    if (code.starts_with("D")) {
      out.defender = halfOf(code, 'D', out.disruptsP, code);
    } else {
      out.attacker = halfOf(code, 'A', out.disruptsP, code);
    }
    return out;
  }

  const HexRules::Table&
  crtTable(const HexRules::RuleSet& rules)
  {
    for (const HexRules::Resolver& resolver : rules.combat().resolvers) {
      if ("crt" == resolver.id && !resolver.tables.empty()) {
        const HexRules::Table& table = resolver.tables.front();
        if (12 != table.cols.size() || 6 != table.rowLabels.size()) {
          throw std::invalid_argument("Pgg::crtTable: table '" + table.id + "' is not the 6 x 12 PGG table");
        }
        return table;
      }
    }
    throw std::invalid_argument("Pgg::crtTable: the rules document has no resolver 'crt' with a table");
  }

  HexModel::Odds
  clampedOdds(HexModel::Strength attack, HexModel::Strength defence)
  {
    const HexModel::OddsOutcome outcome =
        HexModel::makeOdds(attack, defence, HexModel::Rounding::Defender, HexModel::Odds{1, 3}, HexModel::Odds{10, 1});
    if (std::holds_alternative<HexModel::BelowMinimum>(outcome)) {
      return HexModel::Odds{1, 3};
    }
    if (std::holds_alternative<HexModel::AboveMaximum>(outcome)) {
      return HexModel::Odds{10, 1};
    }
    return std::get<HexModel::Odds>(outcome);
  }

  std::string
  oddsText(HexModel::Odds odds)
  {
    return std::to_string(odds.attacker) + "-" + std::to_string(odds.defender);
  }

  std::size_t
  crtColumn(const HexRules::Table& table, HexModel::Odds odds)
  {
    const std::string label = oddsText(odds);
    for (std::size_t c = 0; c < table.cols.size(); ++c) {
      if (label == table.cols[c]) {
        return c;
      }
    }
    throw std::invalid_argument("Pgg::crtColumn: table '" + table.id + "' has no column '" + label + "'");
  }

}  // namespace Pgg
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------

// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
// OddsTableResolver: odds, shifts and multipliers into the rules document's own combat table.
// ----------------------------------------------
#include "hexengine/Defaults.h"

#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <string>
#include <vector>

namespace HexEngine {

  namespace {

    const HexRules::Resolver&
    firstOddsTable(const HexRules::RuleSet& rules)
    {
      for (const HexRules::Resolver& resolver : rules.combat().resolvers) {
        if ("odds-table" == resolver.kind && !resolver.tables.empty()) {
          return resolver;
        }
      }
      throw std::invalid_argument("OddsTableResolver: the rules document '" + rules.gameId() +
                                   "' declares no odds-table resolver with a table");
    }

    std::string
    trimmed(const std::string& text)
    {
      std::size_t first = 0;
      while (first < text.size() && ' ' == text[first]) {
        ++first;
      }
      std::size_t last = text.size();
      while (last > first && ' ' == text[last - 1]) {
        --last;
      }
      return text.substr(first, last - first);
    }

    std::vector<std::string>
    splitOnCommas(const std::string& text)
    {
      std::vector<std::string> parts;
      std::string current;
      for (char c : text) {
        if (',' == c) {
          parts.push_back(trimmed(current));
          current.clear();
        } else {
          current += c;
        }
      }
      parts.push_back(trimmed(current));
      return parts;
    }

    HexModel::Odds
    parseOdds(const std::string& text)
    {
      const std::size_t dash = text.find('-');
      if (std::string::npos == dash) {
        throw std::invalid_argument("OddsTableResolver: '" + text + "' is not an odds ratio");
      }
      return HexModel::Odds{std::stoi(text.substr(0, dash)), std::stoi(text.substr(dash + 1))};
    }

    std::string
    oddsText(HexModel::Odds odds)
    {
      return std::to_string(odds.attacker) + "-" + std::to_string(odds.defender);
    }

    bool
    steeperP(HexModel::Odds l, HexModel::Odds r)
    {
      return l.attacker * r.defender < r.attacker * l.defender;
    }

    // Every odds ratio any column names, ascending; a column shift walks this ladder.
    std::vector<HexModel::Odds>
    oddsLadder(const HexRules::Table& table)
    {
      std::vector<HexModel::Odds> ladder;
      for (const std::string& label : table.cols) {
        for (const std::string& part : splitOnCommas(label)) {
          const HexModel::Odds odds = parseOdds(part);
          if (ladder.end() == std::find(ladder.begin(), ladder.end(), odds)) {
            ladder.push_back(odds);
          }
        }
      }
      std::sort(ladder.begin(), ladder.end(), steeperP);
      return ladder;
    }

    std::size_t
    columnOf(const HexRules::Table& table, HexModel::Odds odds)
    {
      for (std::size_t c = 0; c < table.cols.size(); ++c) {
        for (const std::string& part : splitOnCommas(table.cols[c])) {
          if (parseOdds(part) == odds) {
            return c;
          }
        }
      }
      throw std::invalid_argument("OddsTableResolver: no column for odds " + oddsText(odds));
    }

    std::size_t
    rowOf(const HexRules::Table& table, int die)
    {
      const std::string label = std::to_string(die);
      for (std::size_t r = 0; r < table.rowLabels.size(); ++r) {
        if (table.rowLabels[r] == label) {
          return r;
        }
      }
      throw std::invalid_argument("OddsTableResolver: the table has no row '" + label + "'");
    }

    HexModel::Strength
    scaled(HexModel::Strength strength, double multiplier)
    {
      // Rounded up per side, as every game in view rounds a halved or doubled factor.
      return HexModel::Strength{static_cast<int>(std::ceil(strength.value * multiplier - 1e-9))};
    }

  }  // namespace

  OddsTableResolver::OddsTableResolver(const HexRules::RuleSet& rules)
    : rules_(rules), resolver_(firstOddsTable(rules)), table_(firstOddsTable(rules).tables.front())
  {
    if (resolver_.randomizer) {
      const HexRules::RandomizerSpec& die = rules_.randomizers()[resolver_.randomizer->value];
      faces_ = die.size.value_or(6);
    }
  }

  std::span<const std::string_view>
  OddsTableResolver::claims() const
  {
    return {};
  }

  std::vector<CombatEffect>
  OddsTableResolver::effectsOf(const std::string& code, SideId attacker, SideId defender)
  {
    if ("AE" == code) {
      return {Eliminate{attacker}};
    }
    if ("AS" == code) {
      return {Surrender{attacker}};
    }
    if ("A1" == code) {
      return {StepLoss{attacker, 1}, RetreatEffect{attacker, 1}};
    }
    if ("AR" == code) {
      return {RetreatEffect{attacker, 1}};
    }
    if ("C" == code) {
      return {NoEffect{}};
    }
    if ("EX" == code) {
      return {StepLoss{defender, 1}, StepLoss{attacker, 1}, RetreatEffect{defender, 2}};
    }
    if ("DR" == code) {
      return {RetreatEffect{defender, 2}};
    }
    if ("D1" == code) {
      return {StepLoss{defender, 1}, RetreatEffect{defender, 2}};
    }
    if ("DE" == code) {
      return {Eliminate{defender}};
    }
    if ("DS" == code) {
      return {Surrender{defender}};
    }
    return {GameEffect{code}};
  }

  CombatReport
  OddsTableResolver::report(const Ctx& ctx, const CombatContext& combat, PrngStreams& streams) const
  {
    if (combat.attackers.empty() || combat.defenders.empty()) {
      throw std::invalid_argument("OddsTableResolver: a battle needs at least one unit on each side");
    }
    const SideId attackerSide = ctx.roster.unit(combat.attackers.front()).side;
    const SideId defenderSide = ctx.roster.unit(combat.defenders.front()).side;

    HexModel::Strength attack{0};
    for (UnitId unit : combat.attackers) {
      attack = attack + ctx.roster.unit(unit).front.attack.value_or(HexModel::Strength{0});
    }
    HexModel::Strength defence{0};
    for (UnitId unit : combat.defenders) {
      defence = defence + ctx.roster.unit(unit).front.defence.value_or(HexModel::Strength{0});
    }

    // ---- declared modifiers ---------------------------------------------------------------------
    int shift = 0;
    int shiftCap = 0;
    double attackerMultiplier = 1.0;
    double defenderMultiplier = 1.0;
    double defenderMultiplierCap = 1.0;
    for (ModifierId id : combat.declared) {
      if (rules_.combat().modifiers.size() <= id.value) {
        throw std::invalid_argument("OddsTableResolver: modifier index " + std::to_string(id.value) +
                                     " outside the rules");
      }
      const HexRules::Modifier& modifier = rules_.combat().modifiers[id.value];
      if ("shift" == modifier.kind) {
        const int value = static_cast<int>(modifier.value.value_or(0.0));
        shift += "defender" == modifier.appliesTo ? -value : value;
        shiftCap = std::max(shiftCap, static_cast<int>(modifier.cap.value_or(0.0)));
      } else if ("multiplier" == modifier.kind) {
        const double value = modifier.value.value_or(1.0);
        if ("defender" == modifier.appliesTo) {
          defenderMultiplier *= value;
          defenderMultiplierCap = std::max(defenderMultiplierCap, modifier.cap.value_or(value));
        } else if ("attacker" == modifier.appliesTo) {
          attackerMultiplier *= value;
        }
      }
    }
    if (0 < shiftCap) {
      shift = std::clamp(shift, -shiftCap, shiftCap);
    }

    // ---- the target hex's own terrain -------------------------------------------------------------
    double terrainMultiplier = 1.0;
    int terrainShift = 0;
    const HexRules::Terrain& hex = rules_.hexTerrain()[ctx.board.terrain(combat.target).value];
    terrainMultiplier = std::max(terrainMultiplier, hex.defenceMultiplier.value_or(1.0));
    terrainShift -= hex.shift.value_or(0);
    for (const HexModel::Feature& feature : ctx.board.features(combat.target)) {
      const HexRules::Terrain& drawn = rules_.hexTerrain()[feature.terrain.value];
      terrainMultiplier = std::max(terrainMultiplier, drawn.defenceMultiplier.value_or(1.0));
      terrainShift -= drawn.shift.value_or(0);
    }
    defenderMultiplier *= terrainMultiplier;
    defenderMultiplierCap = std::max(defenderMultiplierCap, terrainMultiplier);
    // A defender is never more than the rules' own cap: terrain doubling saturates, it does not stack.
    defenderMultiplier = std::min(defenderMultiplier, defenderMultiplierCap);
    shift += terrainShift;
    if (0 < shiftCap) {
      shift = std::clamp(shift, -shiftCap, shiftCap);
    }

    attack = scaled(attack, attackerMultiplier);
    defence = scaled(defence, defenderMultiplier);

    // ---- odds -------------------------------------------------------------------------------------
    const HexModel::Odds minimum =
        resolver_.minOdds ? parseOdds(*resolver_.minOdds) : HexModel::Odds{1, 99};
    const HexModel::Odds maximum =
        resolver_.maxOdds ? parseOdds(*resolver_.maxOdds) : HexModel::Odds{99, 1};
    const HexModel::OddsOutcome outcome =
        HexModel::makeOdds(attack, defence, resolver_.rounding, minimum, maximum);

    CombatReport out;
    if (std::holds_alternative<HexModel::BelowMinimum>(outcome)) {
      out.odds = "below-minimum";
      out.outcome = "below-minimum";
      out.effects = {Surrender{attackerSide}};
      return out;
    }
    if (std::holds_alternative<HexModel::AboveMaximum>(outcome)) {
      out.odds = "above-maximum";
      out.outcome = "above-maximum";
      out.effects = {Eliminate{defenderSide}};
      return out;
    }

    const std::vector<HexModel::Odds> ladder = oddsLadder(table_);
    HexModel::Odds odds = std::get<HexModel::Odds>(outcome);
    const auto rung = std::find(ladder.begin(), ladder.end(), odds);
    if (ladder.end() == rung) {
      throw std::invalid_argument("OddsTableResolver: no column names odds " + oddsText(odds));
    }
    const long long shifted =
        std::clamp<long long>(std::distance(ladder.begin(), rung) + shift, 0,
                               static_cast<long long>(ladder.size()) - 1);
    odds = ladder[static_cast<std::size_t>(shifted)];

    const int die = rollDie(streams, StreamTag::Combat, faces_);
    out.dice.push_back(die);
    out.odds = oddsText(odds);
    out.outcome = table_.cell(rowOf(table_, die), columnOf(table_, odds));
    out.effects = effectsOf(out.outcome, attackerSide, defenderSide);
    return out;
  }

  std::vector<CombatEffect>
  OddsTableResolver::resolve(const Ctx& ctx, const CombatContext& combat, PrngStreams& streams) const
  {
    return report(ctx, combat, streams).effects;
  }

}  // namespace HexEngine
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------

// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
#include "TrcCombat.h"

#include "hexengine/Defaults.h"

#include <algorithm>
#include <array>
#include <stdexcept>

namespace Trc {

  namespace {

    constexpr std::array<std::string_view, 1> kClaims{"combat-supply-halving"};

    const HexRules::Table&
    crtOf(const HexRules::RuleSet& rules)
    {
      for (const HexRules::Resolver& resolver : rules.combat().resolvers) {
        if ("crt" == resolver.id && !resolver.tables.empty()) {
          return resolver.tables.front();
        }
      }
      throw std::invalid_argument("TrcCombat: the rules document has no resolver 'crt' with a table");
    }

    bool
    terrainNamedP(const Ctx& ctx, HexIndex hex, const char* id)
    {
      return id == ctx.rules.hexTerrain()[ctx.board.terrain(hex).value].id;
    }

    HexIndex
    hexOf(const Ctx& ctx, UnitId unit)
    {
      return std::get<HexIndex>(*ctx.position.unit(unit).where);
    }

  }  // namespace

  TrcCombat::TrcCombat(const TrcFacts& facts, const TrcSupply& supply)
    : facts_(facts), supply_(supply), table_(crtOf(*facts.definition().rules))
  {
    if (9 != table_.cols.size() || 6 != table_.rowLabels.size()) {
      throw std::invalid_argument("TrcCombat: table '" + table_.id + "' is not the 6 x 9 TRC table");
    }
  }

  std::span<const std::string_view>
  TrcCombat::claims() const
  {
    return kClaims;
  }

  const std::vector<HexModel::Odds>&
  TrcCombat::ladder()
  {
    static const std::vector<HexModel::Odds> rungs{{1, 6}, {1, 5}, {1, 4}, {1, 3}, {1, 2}, {1, 1}, {2, 1},
                                                   {3, 1}, {4, 1}, {5, 1}, {6, 1}, {7, 1}, {8, 1}, {9, 1}};
    return rungs;
  }

  // Columns "1-5, 1-6", "1-3, 1-4", "1-2", "1-1", "2-1", "3-1", "4-1", "5-1, 6-1", "7-1, 9-1".
  std::size_t
  TrcCombat::columnOf(HexModel::Odds odds)
  {
    if (1 == odds.attacker) {
      if (5 <= odds.defender) {
        return 0;
      }
      if (3 <= odds.defender) {
        return 1;
      }
      return 2 == odds.defender ? 2 : 3;
    }
    if (4 >= odds.attacker) {
      return static_cast<std::size_t>(odds.attacker) + 2;
    }
    return 6 >= odds.attacker ? 7 : 8;
  }

  std::string
  TrcCombat::text(HexModel::Odds odds)
  {
    return std::to_string(odds.attacker) + "-" + std::to_string(odds.defender);
  }

  Strength
  TrcCombat::factorOf(const Ctx& ctx, UnitId unit) const
  {
    const int printed = ctx.roster.unit(unit).front.attack.value_or(Strength{0}).value;
    return Strength{supply_.combatSuppliedP(ctx, unit) ? printed : (printed + 1) / 2};
  }

  bool
  TrcCombat::doubledP(const Ctx& ctx, const std::vector<UnitId>& attackers, HexIndex target) const
  {
    if (terrainNamedP(ctx, target, "mountain-hex") || facts_.majorCityP(target)) {
      return true;
    }
    bool allRiverP = !attackers.empty();
    bool sameRiverP = false;
    bool allKerchP = !attackers.empty();
    for (UnitId unit : attackers) {
      const HexIndex from = hexOf(ctx, unit);
      allRiverP = allRiverP && facts_.riverHexP(from);
      sameRiverP = sameRiverP || (facts_.riverHexP(target) && facts_.riverOf(from) == facts_.riverOf(target));
      allKerchP = allKerchP && facts_.kerchP(from, target);
    }
    return (allRiverP && !sameRiverP) || allKerchP;
  }

  TrcCombat::Factors
  TrcCombat::factors(const Ctx& ctx, const std::vector<UnitId>& attackers, HexIndex target,
                      const std::vector<UnitId>& defenders) const
  {
    Factors out{Strength{0}, Strength{0}};
    for (UnitId unit : attackers) {
      out.attack = out.attack + factorOf(ctx, unit);
    }
    for (UnitId unit : defenders) {
      out.defence = out.defence + factorOf(ctx, unit);
    }
    if (doubledP(ctx, attackers, target)) {
      out.defence = Strength{2 * out.defence.value};
    }
    return out;
  }

  HexModel::OddsOutcome
  TrcCombat::odds(const Factors& factors) const
  {
    return HexModel::makeOdds(factors.attack, factors.defence, HexModel::Rounding::Defender, HexModel::Odds{1, 6},
                              HexModel::Odds{9, 1});
  }

  int
  TrcCombat::shift(const std::vector<ModifierId>& declared) const
  {
    int total = 0;
    for (ModifierId id : declared) {
      total += static_cast<int>(facts_.definition().rules->combat().modifiers[id.value].value.value_or(0.0));
    }
    return std::min(total, 3);  // 15.10
  }

  HexEngine::CombatReport
  TrcCombat::report(const Ctx& ctx, const HexEngine::CombatContext& combat, HexEngine::PrngStreams& streams) const
  {
    const Factors power = factors(ctx, combat.attackers, combat.target, combat.defenders);
    const HexModel::OddsOutcome outcome = odds(power);
    if (std::holds_alternative<HexModel::BelowMinimum>(outcome)) {
      throw std::invalid_argument("TrcCombat: " + std::to_string(power.attack.value) + " against " +
                                   std::to_string(power.defence.value) + " is worse than 1-6, which is illegal (12.5)");
    }
    const HexModel::Odds base =
        std::holds_alternative<HexModel::AboveMaximum>(outcome) ? HexModel::Odds{9, 1} : std::get<HexModel::Odds>(outcome);
    const std::vector<HexModel::Odds>& rungs = ladder();
    const long long rung = std::distance(rungs.begin(), std::find(rungs.begin(), rungs.end(), base));
    const HexModel::Odds shifted =
        rungs[static_cast<std::size_t>(std::min<long long>(rung + shift(combat.declared), 13))];

    const int die = HexEngine::rollDie(streams, HexEngine::StreamTag::Combat, 6);
    std::string code = table_.cell(static_cast<std::size_t>(die - 1), columnOf(shifted));
    bool attackersInWoodsP = true;
    for (UnitId unit : combat.attackers) {
      attackersInWoodsP = attackersInWoodsP && terrainNamedP(ctx, hexOf(ctx, unit), "woods");
    }
    if (("AR" == code && attackersInWoodsP) || ("DR" == code && terrainNamedP(ctx, combat.target, "woods"))) {
      code = "C";  // 14.2
    }

    HexEngine::CombatReport out;
    out.odds = text(shifted);
    out.outcome = code;
    out.dice.push_back(die);
    out.effects = HexEngine::OddsTableResolver::effectsOf(code, ctx.roster.unit(combat.attackers.front()).side,
                                                          ctx.roster.unit(combat.defenders.front()).side);
    return out;
  }

  std::vector<HexEngine::CombatEffect>
  TrcCombat::resolve(const Ctx& ctx, const HexEngine::CombatContext& combat, HexEngine::PrngStreams& streams) const
  {
    return report(ctx, combat, streams).effects;
  }

}  // namespace Trc
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------

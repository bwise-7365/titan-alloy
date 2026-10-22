// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
#include "PggCombat.h"

#include "PggBattle.h"
#include "PggState.h"
#include "PggUnits.h"

#include <algorithm>
#include <array>
#include <map>
#include <stdexcept>

namespace Pgg {

  namespace {

    constexpr std::array<std::string_view, 4> kClaims{"divisional-integration", "integration-order",
                                                      "order-integration-before-terrain", "order-reveal-mid-phase"};

    // A unit whose true values are 0-0 leaves play the instant it is revealed (12.3).
    bool
    noStrengthP(const Ctx& ctx, UnitId unit)
    {
      return 0 == Units::attackOf(ctx, unit) && 0 == Units::defenceOf(ctx, unit);
    }

  }  // namespace

  PggCombat::PggCombat(const PggFacts& facts, const PggSupply& supply)
    : facts_(facts), supply_(supply), table_(crtTable(*facts.definition().rules))
  {
  }

  std::span<const std::string_view>
  PggCombat::claims() const
  {
    return kClaims;
  }

  bool
  PggCombat::integratedP(const Ctx& ctx, UnitId unit) const
  {
    const std::optional<Division> division = facts_.division(unit);
    if (!division || (DivisionKind::Panzer != division->kind && DivisionKind::Motorized != division->kind &&
                      DivisionKind::DasReich != division->kind)) {
      return false;
    }
    const std::optional<HexIndex> hex = Units::hexOf(ctx.position, unit);
    const std::vector<UnitId>& members = facts_.members(*division);
    for (UnitId member : members) {
      if (Units::hexOf(ctx.position, member) != hex) {
        return false;  // 7.3: every regiment, none eliminated
      }
    }
    const bool twoRegimentsP = DivisionKind::Motorized == division->kind && 2 == members.size();
    for (UnitId other : ctx.position.unitsAt(*hex)) {
      const bool memberP = members.end() != std::find(members.begin(), members.end(), other);
      if (!memberP && !(twoRegimentsP && facts_.independentRegimentP(other))) {
        return false;  // 7.3, amendments 7.3
      }
    }
    return true;
  }

  int
  PggCombat::defenceMultiple(const Ctx& ctx, const std::vector<UnitId>& attackers, HexIndex target) const
  {
    int multiple = 1;
    multiple += facts_.majorCityP(target) ? 1 : 0;
    multiple += facts_.woodsP(target) ? 1 : 0;
    bool allAcrossRiversP = !attackers.empty();
    for (UnitId unit : attackers) {
      const std::optional<HexIndex> from = Units::hexOf(ctx.position, unit);
      const std::optional<Direction> direction = from ? facts_.directionTo(*from, target) : std::nullopt;
      allAcrossRiversP = allAcrossRiversP && direction && facts_.riverP(*from, *direction);
    }
    multiple += allAcrossRiversP ? 1 : 0;  // 9.32
    return multiple;
  }

  int
  PggCombat::adjusted(const Ctx& ctx, HexSearch::SearchScratch& scratch, UnitId unit, int strength) const
  {
    if (0 < strength && !supply_.suppliedP(ctx, scratch, unit)) {
      return std::max(1, strength / 2);  // 11.31, 11.32
    }
    return strength;
  }

  PggCombat::Assessment
  PggCombat::assess(const Ctx& ctx, const std::vector<UnitId>& listed, HexIndex target, bool overrunP) const
  {
    HexSearch::SearchScratch scratch;
    Assessment out;
    const SideId attacker = ctx.roster.unit(listed.front()).side;
    const PggState& state = stateOf(ctx.position);

    // ---- defence: integration, terrain, supply ----
    std::vector<UnitId> fighting;
    for (UnitId unit : listed) {
      if (!facts_.leaderP(unit)) {
        fighting.push_back(unit);
      }
    }
    const int multiple = defenceMultiple(ctx, fighting, target);
    int defence = 0;
    for (UnitId unit : ctx.position.unitsAt(target)) {
      if (facts_.markerP(unit)) {
        continue;
      }
      out.defenders.push_back(unit);
      const SideId side = ctx.roster.unit(unit).side;
      if (state.side(side).retreatedOnto.contains(unit) || noStrengthP(ctx, unit)) {
        continue;  // 9.75, 12.3
      }
      const int doubled = Units::defenceOf(ctx, unit) * (integratedP(ctx, unit) ? 2 : 1);
      defence += adjusted(ctx, scratch, unit, doubled * multiple);
    }

    // ---- attack: integration, supply, then each hex's Leaders ----
    std::map<std::uint32_t, int> byHex;
    for (UnitId unit : fighting) {
      out.attackers.push_back(unit);
      const int doubled = Units::attackOf(ctx, unit) * (integratedP(ctx, unit) ? 2 : 1);
      byHex[Units::hexOf(ctx.position, unit)->value] += adjusted(ctx, scratch, unit, doubled);
    }
    int attack = 0;
    for (const auto& [hex, strength] : byHex) {
      int ratings = 0;
      for (UnitId unit : ctx.position.unitsAt(HexIndex{hex})) {
        if (facts_.leaderP(unit) && attacker == ctx.roster.unit(unit).side && !state.side(attacker).disrupted.contains(unit)) {
          ratings += facts_.leaderRating(unit);
          out.attackers.push_back(unit);
        }
      }
      attack += strength + std::min(ratings, strength);  // 10.36, amendments 10.27
    }
    out.attack = Strength{overrunP ? attack / 2 : attack};  // 6.55
    out.defence = Strength{defence};
    return out;
  }

  HexEngine::CombatReport
  PggCombat::resolveBattle(const Ctx& ctx, const Assessment& assessment, HexIndex target, bool overrunP,
                           HexEngine::PrngStreams& streams) const
  {
    HexEngine::CombatReport out;
    if (0 == assessment.attack.value) {
      out.odds = "none";
      out.outcome = "void";
    } else if (0 == assessment.defence.value) {
      out.odds = "none";
      out.outcome = "De";
    } else {
      const HexModel::Odds odds = clampedOdds(assessment.attack, assessment.defence);
      const int die = HexEngine::rollDie(streams, HexEngine::StreamTag::Combat, 6);
      out.dice.push_back(die);
      out.odds = oddsText(odds);
      out.outcome = table_.cell(static_cast<std::size_t>(die - 1), crtColumn(table_, odds));
    }
    PggBattle battle;
    battle.attackers = assessment.attackers;
    battle.defenders = assessment.defenders;
    battle.target = target;
    battle.attackerSide = ctx.roster.unit(assessment.attackers.front()).side;
    battle.defenderSide = facts_.enemy(battle.attackerSide);
    battle.overrunP = overrunP;
    battle.code = out.outcome;
    out.effects.push_back(
        HexEngine::OweEffect{HexModel::makePolymorphic<HexModel::GameObligation, PggBattle>(std::move(battle))});
    return out;
  }

  HexEngine::CombatReport
  PggCombat::report(const Ctx& ctx, const HexEngine::CombatContext& combat, HexEngine::PrngStreams& streams) const
  {
    if (combat.attackers.empty()) {
      throw std::invalid_argument("PggCombat: a battle needs an attacker");
    }
    return resolveBattle(ctx, assess(ctx, combat.attackers, combat.target, false), combat.target, false, streams);
  }

  std::vector<HexEngine::CombatEffect>
  PggCombat::resolve(const Ctx& ctx, const HexEngine::CombatContext& combat, HexEngine::PrngStreams& streams) const
  {
    return report(ctx, combat, streams).effects;
  }

}  // namespace Pgg
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------

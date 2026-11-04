// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
#include "PggAgent.h"

#include "PggUnits.h"

#include <algorithm>
#include <set>

namespace Pgg {

  namespace {

    constexpr double kLeastOdds = 1.0;  // attack at 1-1 or better, as far as the agent can tell
    constexpr int kAirMarkers = 3;      // 13.1
    constexpr int kAirColumn = 40;      // 13.2: no marker east of hex-row 4000 while Smolensk is not held

  }  // namespace

  // Every unit of this side next to the target that PGG lets attack it: the checks of check-attack
  // (9.14, 6.62, 10.22, 10.35, 12.3, 6.7).
  std::vector<UnitId>
  PggAgent::attackersOn(const Ctx& ctx, HexIndex target)
  {
    const PggFacts& facts = set_.facts();
    std::vector<UnitId> out;
    for (UnitId unit : Units::onMap(ctx, facts, side_)) {
      const HexModel::UnitState& state = ctx.position.unit(unit);
      const HexIndex from = *Units::hexOf(ctx.position, unit);
      const std::optional<HexModel::Direction> direction = facts.directionTo(from, target);
      if (!direction || facts.lakeHexsideP(from, *direction) || facts.leaderP(unit) || state.flags.attackedP ||
          stateOf(ctx.position).side(side_).disrupted.contains(unit) || (state.flags.revealedP && 0 == Units::attackOf(ctx, unit))) {
        continue;
      }
      if (facts.soviet() == side_ && !set_.supply().withinRadiusP(ctx, scratch_, unit)) {
        continue;
      }
      out.push_back(unit);
    }
    return out;
  }

  std::optional<HexEngine::Command>
  PggAgent::nextAttack(const HexEngine::Session& session, const PggSeen& seen)
  {
    const Ctx ctx = session.context();
    const PggFacts& facts = set_.facts();
    const std::vector<HexAi::Piece> friends = seen.pieces(side_);
    const std::vector<HexAi::Piece> enemies = seen.pieces(facts.enemy(side_));
    std::set<HexIndex> targets;
    for (UnitId enemy : Units::onMap(ctx, facts, facts.enemy(side_))) {
      targets.insert(*Units::hexOf(ctx.position, enemy));
    }
    for (HexIndex target : targets) {
      const std::vector<UnitId>& defenders = ctx.position.unitsAt(target);
      if (memory_.targets.contains(target) ||
          std::any_of(defenders.begin(), defenders.end(), [&](UnitId unit) { return ctx.position.unit(unit).flags.defendedP; })) {
        continue;
      }
      const std::vector<UnitId> attackers = attackersOn(ctx, target);
      if (attackers.empty()) {
        continue;
      }
      if (const HexAi::AttackOrConsolidate* consolidate = std::get_if<HexAi::AttackOrConsolidate>(&tactic_)) {
        const HexAi::LocalForce force = HexAi::localForce(ctx.board, target, consolidate->attackRadius, friends, enemies);
        if (!HexAi::favourableP(force, consolidate->attackRatio)) {
          continue;
        }
      }
      double attack = 0.0;
      for (UnitId unit : attackers) {
        attack += seen.attack(unit);
      }
      double defence = 0.0;
      for (UnitId unit : defenders) {
        defence += seen.defence(unit);
      }
      defence *= set_.combat().defenceMultiple(ctx, attackers, target);
      if (attack >= kLeastOdds * defence) {
        return HexEngine::DeclareAttack{attackers, target, {}};
      }
    }
    return std::nullopt;
  }

  // The German Player's markers fall on the westernmost Soviet hexes, the nearest to his line of advance.
  std::optional<HexEngine::Command>
  PggAgent::nextInterdiction(const HexEngine::Session& session)
  {
    const Ctx ctx = session.context();
    const PggFacts& facts = set_.facts();
    const PggState& state = stateOf(ctx.position);
    if (facts.german() != side_ || kAirMarkers <= static_cast<int>(state.airInterdiction.size())) {
      return std::nullopt;
    }
    std::optional<HexIndex> best;
    for (UnitId enemy : Units::onMap(ctx, facts, facts.soviet())) {
      const HexIndex hex = *Units::hexOf(ctx.position, enemy);
      const bool markedP = state.airInterdiction.end() != std::find(state.airInterdiction.begin(), state.airInterdiction.end(), hex);
      if (markedP || memory_.targets.contains(hex) || kAirColumn < facts.column(hex)) {
        continue;
      }
      if (!best || facts.column(hex) < facts.column(*best) || (facts.column(hex) == facts.column(*best) && hex.value < best->value)) {
        best = hex;
      }
    }
    if (!best) {
      return std::nullopt;
    }
    return HexEngine::GameCommand{"interdict", {ctx.board.id(*best).text}};
  }

}  // namespace Pgg
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------

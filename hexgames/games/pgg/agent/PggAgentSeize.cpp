// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
// The seize tactic in PGG (doc/2026-11-04-seize-locations-tactic.md). The locations are the Victory Point
// hexes. The German fringe is the unheld hexes whose regions hold a German unit. The Soviets go for that
// fringe too, and for whatever the Germans hold, with the rest of their strength set out over the other
// hexes by their points. A hex no one has occupied counts as Soviet, as the victory rules score it.
// ----------------------------------------------
#include "PggAgent.h"

#include "PggSchedule.h"
#include "PggTerrain.h"
#include "PggUnits.h"

#include "hexai/Seize.h"

#include <algorithm>
#include <set>

namespace Pgg {

  namespace {

    bool
    integrablyP(const std::optional<Division>& division)
    {
      return division && (DivisionKind::Panzer == division->kind || DivisionKind::Motorized == division->kind ||
                          DivisionKind::DasReich == division->kind);
    }

  }  // namespace

  bool
  PggAgent::seizingP() const
  {
    return std::holds_alternative<HexAi::SeizeLocations>(tactic_);
  }

  void
  PggAgent::buildCityFields()
  {
    const HexRules::GameDefinition& definition = set_.facts().definition();
    const MoveClass moveClass = set_.facts().german() == side_ ? MoveClass::Motor : MoveClass::Foot;
    stepCost_ = [this, moveClass](HexIndex from, HexModel::Direction direction) {
      return terrainHalves(set_.facts(), moveClass, side_, from, direction);
    };
    for (const VictoryHex& city : set_.facts().victoryHexes()) {
      const std::vector<HexIndex> source{city.hex};
      cityFields_.emplace_back(*definition.board, source, stepCost_);
    }
    return;
  }

  // What moves together: the regiments of a division that can integrate, and every other unit alone.
  std::vector<UnitId>
  PggAgent::groupOf(const Ctx& ctx, UnitId unit) const
  {
    const std::optional<Division> division = set_.facts().division(unit);
    if (!integrablyP(division)) {
      return {unit};
    }
    std::vector<UnitId> out;
    for (UnitId member : set_.facts().members(*division)) {
      if (Units::hexOf(ctx.position, member)) {
        out.push_back(member);
      }
    }
    return out;
  }

  // Where the Germans stand; before any has entered, the entrance areas of the next to arrive.
  std::vector<HexIndex>
  PggAgent::germanPositions(const Ctx& ctx) const
  {
    const PggFacts& facts = set_.facts();
    std::vector<HexIndex> out;
    for (UnitId unit : Units::onMap(ctx, facts, facts.german())) {
      out.push_back(*Units::hexOf(ctx.position, unit));
    }
    if (!out.empty()) {
      return out;
    }
    int soonest = 0;
    bool anyP = false;
    for (UnitId unit : ctx.position.unitsIn(facts.germanArrivals())) {
      const int delay = ctx.position.unit(unit).delay.value_or(0);
      soonest = anyP ? std::min(soonest, delay) : delay;
      anyP = true;
    }
    for (UnitId unit : ctx.position.unitsIn(facts.germanArrivals())) {
      const std::optional<GermanArrival> arrival = germanArrivalOf(facts, unit);
      if (arrival && soonest == ctx.position.unit(unit).delay.value_or(0)) {
        const std::vector<HexIndex>& area = facts.area(arrival->area);
        out.insert(out.end(), area.begin(), area.end());
      }
    }
    return out;
  }

  // The targets the allocation may choose from, parallel to the cities they stand for.
  std::vector<HexAi::Target>
  PggAgent::seizeTargets(const Ctx& ctx, const PggSeen& seen, std::vector<std::size_t>& cities) const
  {
    const PggFacts& facts = set_.facts();
    const std::vector<VictoryHex>& hexes = facts.victoryHexes();
    std::vector<HexIndex> unheld;
    std::vector<HexIndex> german;
    for (const VictoryHex& city : hexes) {
      (facts.german() == ctx.position.control(city.hex) ? german : unheld).push_back(city.hex);
    }
    const std::vector<HexIndex> positions = germanPositions(ctx);
    std::vector<HexIndex> fringe = HexAi::occupiedRegions(ctx.board, unheld, positions, stepCost_);
    if (facts.soviet() == side_) {
      fringe.insert(fringe.end(), german.begin(), german.end());
    }
    const std::vector<HexAi::Piece> enemies = seen.pieces(facts.enemy(side_));
    const std::vector<double> enemyByRegion = HexAi::strengthByRegion(ctx.board, fringe, enemies, stepCost_);

    std::vector<HexAi::Target> targets;
    for (std::size_t c = 0; c < hexes.size(); ++c) {
      const auto found = std::find(fringe.begin(), fringe.end(), hexes[c].hex);
      const bool fringeP = fringe.end() != found;
      const double weight = fringeP ? enemyByRegion[static_cast<std::size_t>(found - fringe.begin())] : hexes[c].points;
      targets.push_back(HexAi::Target{hexes[c].hex, weight, fringeP});
      cities.push_back(c);
    }
    return targets;
  }

  void
  PggAgent::planSeizure(const HexEngine::Session& session, const PggSeen& seen)
  {
    if (memory_.plannedP) {
      return;
    }
    memory_.plannedP = true;
    const Ctx ctx = session.context();
    const PggFacts& facts = set_.facts();
    std::vector<std::vector<UnitId>> groups;
    std::vector<HexAi::Force> forces;
    std::set<UnitId> grouped;
    for (UnitId unit : Units::onMap(ctx, facts, side_)) {
      if (facts.leaderP(unit) || grouped.contains(unit)) {
        continue;
      }
      const std::vector<UnitId> group = groupOf(ctx, unit);
      grouped.insert(group.begin(), group.end());
      const HexIndex hex = *Units::hexOf(ctx.position, group.front());
      double strength = 0.0;
      for (UnitId member : group) {
        strength += seen.attack(member);
      }
      const bool freeP = 0 < set_.movement().remainingHalves(ctx, group.front()) &&
                         !set_.zoc().blockedForP(ctx, hex, side_, HexRules::Purpose::Movement);
      groups.push_back(group);
      forces.push_back(HexAi::Force{hex, strength, freeP});
    }
    std::vector<std::size_t> cities;
    const std::vector<HexAi::Target> targets = seizeTargets(ctx, seen, cities);
    const double share = std::get<HexAi::SeizeLocations>(tactic_).fringeShare;
    const std::vector<std::optional<std::size_t>> chosen =
        HexAi::allocate(forces, targets, share, [&](std::size_t target, HexIndex hex) { return cityFields_[cities[target]].cost(hex); });

    const bool movementP = PhaseKind::Combat != facts.phase(ctx.position.clock().phase).kind;
    for (std::size_t f = 0; f < groups.size(); ++f) {
      if (!chosen[f]) {
        continue;
      }
      const std::size_t city = cities[*chosen[f]];
      for (UnitId unit : groups[f]) {
        memory_.cityOf[unit] = city;
        const auto before = lastCityOf_.find(unit);
        if (movementP && lastCityOf_.end() != before) {
          stats_.targetsKept += before->second == city ? 1 : 0;
          stats_.targetsChanged += before->second == city ? 0 : 1;
        }
        lastCityOf_[unit] = city;
      }
    }
    return;
  }

  // One group's stack moves together. The stack with the group's first unit makes for the group's city and
  // the rest follow it, so that a division split by a retreat or an entry comes back together.
  std::optional<HexEngine::Command>
  PggAgent::seizeMove(const HexEngine::Session& session, const Ctx& ctx, UnitId unit)
  {
    const ModeId mode = set_.facts().normalMode();
    const HexIndex from = *Units::hexOf(ctx.position, unit);
    const std::vector<UnitId> group = groupOf(ctx, unit);
    std::vector<UnitId> stack;
    for (UnitId member : group) {
      if (from == Units::hexOf(ctx.position, member)) {
        stack.push_back(member);
        memory_.considered.insert(member);
      }
    }
    const auto city = memory_.cityOf.find(group.front());
    if (memory_.cityOf.end() == city) {
      return std::nullopt;
    }
    const HexIndex leadHex = *Units::hexOf(ctx.position, group.front());
    const bool leadP = leadHex == from;
    const HexAi::CostField& field = cityFields_[city->second];
    const auto distance = [&](HexIndex hex) -> std::optional<long long> {
      if (!leadP) {
        return ctx.board.distance(hex, leadHex);
      }
      const std::optional<int> cost = field.cost(hex);
      return cost ? std::optional<long long>(*cost) : std::nullopt;
    };
    const std::set<HexIndex>& refusedHexes = memory_.hexes[unit];
    const auto allowedP = [&](HexIndex hex) {
      std::vector<UnitId> there;
      for (UnitId occupant : ctx.position.unitsAt(hex)) {
        if (set_.facts().markerP(occupant)) {
          continue;
        }
        if (1 < group.size() && group.end() == std::find(group.begin(), group.end(), occupant)) {
          return false;  // 7.3: a foreign unit would break the division's integration
        }
        there.push_back(occupant);
      }
      there.insert(there.end(), stack.begin(), stack.end());
      return !refusedHexes.contains(hex) && set_.stacking().legalP(there);
    };
    const std::vector<UnitId> excess = set_.stacking().excess(ctx, from);
    const bool mustLeaveP = std::any_of(stack.begin(), stack.end(), [&](UnitId member) {
      return excess.end() != std::find(excess.begin(), excess.end(), member);
    });
    const HexEngine::Reachability reach = session.reachable(stack, mode);
    const std::optional<std::size_t> best = HexAi::chooseDestination(reach, distance, from, mustLeaveP, allowedP);
    if (!best) {
      return std::nullopt;
    }
    return HexEngine::MoveUnit{stack, mode, reach.paths[*best]};
  }

  // After a battle, the strongest attacker that the emptied hex takes nearer its city goes in, and goes on
  // only if that takes it nearer still.
  std::string
  PggAgent::seizeAdvance(const Ctx& ctx, const PggSeen& seen, const HexModel::GameChoice& choice)
  {
    if ("advance-on" == choice.verb) {
      const auto city = memory_.cityOf.find(advancing_.value_or(UnitId{}));
      if (!advancing_ || memory_.cityOf.end() == city || !lastTarget_) {
        return "stop";
      }
      const HexAi::CostField& field = cityFields_[city->second];
      std::string best = "stop";
      int bestCost = field.cost(*lastTarget_).value_or(0);
      for (const std::string& option : choice.options) {
        if ("stop" == option) {
          continue;
        }
        const std::optional<int> cost = field.cost(set_.facts().hex(option));
        if (cost && *cost < bestCost) {
          best = option;
          bestCost = *cost;
        }
      }
      return best;
    }
    if (advancedP_ || !lastTarget_) {
      advancedP_ = false;
      advancing_.reset();
      return "done";
    }
    std::optional<UnitId> strongest;
    for (const std::string& option : choice.options) {
      if ("done" == option) {
        continue;
      }
      const UnitId unit = set_.facts().counter(option);
      const auto city = memory_.cityOf.find(unit);
      const std::optional<HexIndex> hex = Units::hexOf(ctx.position, unit);
      if (memory_.cityOf.end() == city || !hex) {
        continue;
      }
      const std::optional<int> here = cityFields_[city->second].cost(*hex);
      const std::optional<int> there = cityFields_[city->second].cost(*lastTarget_);
      if (here && there && *there < *here && (!strongest || seen.attack(*strongest) < seen.attack(unit))) {
        strongest = unit;
      }
    }
    if (!strongest) {
      return "done";
    }
    advancedP_ = true;
    advancing_ = strongest;
    return ctx.roster.unit(*strongest).counter.text;
  }

}  // namespace Pgg
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------

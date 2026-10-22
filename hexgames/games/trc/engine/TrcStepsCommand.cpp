// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
// Before-command checks and after-command effects. The after-command effects run in the order the
// rules document lists them on the game-turn phase: rail capacity and markers, touched rail, the
// Warsaw trigger, air used, workers' surrender, lost leaders, then -- once a battle's last answer
// is given (13.3) -- stacking after combat (6.2) and control.
// ----------------------------------------------
#include "TrcMarkers.h"
#include "TrcState.h"
#include "TrcSteps.h"
#include "TrcUnits.h"

namespace Trc {

  namespace {

    using HexEngine::CheckCall;
    using HexEngine::StepCall;

    void
    checkAttack(const TrcParts& parts, const Ctx& ctx, const HexEngine::DeclareAttack& attack)
    {
      for (UnitId unit : attack.attackers) {
        if (!parts.mandatory.mayAttackP(ctx, unit)) {
          throw std::invalid_argument("Trc: counter '" + ctx.roster.unit(unit).counter.text +
                                      "' has attacked, moved by rail or won an automatic victory this impulse "
                                      "(9.1, 12.4, 16.2)");
        }
      }
      const SideId side = ctx.roster.unit(attack.attackers.front()).side;
      std::vector<UnitId> defenders;
      for (UnitId unit : ctx.position.unitsAt(attack.target)) {
        if (side == ctx.roster.unit(unit).side) {
          continue;
        }
        if (ctx.position.unit(unit).flags.defendedP) {
          throw std::invalid_argument("Trc: counter '" + ctx.roster.unit(unit).counter.text +
                                      "' has already been attacked this phase (12.4)");
        }
        defenders.push_back(unit);
      }
      parts.air.check(ctx, attack);
      if (defenders.empty()) {
        return;  // the engine refuses an attack on an empty hex with its own reason
      }
      const TrcCombat::Factors power = parts.combat.factors(ctx, attack.attackers, attack.target, defenders);
      if (std::holds_alternative<HexModel::BelowMinimum>(parts.combat.odds(power))) {
        throw std::invalid_argument("Trc: " + std::to_string(power.attack.value) + " against " +
                                    std::to_string(power.defence.value) + " is worse than 1-6 (12.5)");
      }
      return;
    }

    Position
    countRailMoves(const StepCall& call)
    {
      const HexEngine::MoveUnit& move = commandOf<HexEngine::MoveUnit>(call);
      Position next = call.ctx.position;
      const SideId side = call.ctx.roster.unit(move.units.front()).side;
      addCount(stateOf(next).side(side).railMoves, static_cast<int>(move.units.size()));
      for (UnitId unit : move.units) {
        Markers::mark(next, unit, Markers::kRailed);
      }
      return next;
    }

    Position
    countAirUsed(const StepCall& call)
    {
      const HexEngine::DeclareAttack& attack = commandOf<HexEngine::DeclareAttack>(call);
      Position next = call.ctx.position;
      if (!attack.modifiers.empty()) {
        const SideId side = call.ctx.roster.unit(attack.attackers.front()).side;
        addCount(stateOf(next).side(side).airUsed, static_cast<int>(attack.modifiers.size()));
      }
      return next;
    }

    // 22.2: a worker always surrenders rather than being eliminated.
    Position
    workersSurrender(const TrcParts& parts, const StepCall& call)
    {
      Position next = call.ctx.position;
      const SideId russian = parts.facts.russian();
      for (UnitId unit : call.ctx.position.unitsIn(parts.facts.pool(russian))) {
        if (parts.facts.typeP(unit, "worker")) {
          Units::remove(next, unit, parts.facts.surrendered(russian), call.sink);
        }
      }
      return next;
    }

    // 11.3: a leader is lost when he lands in his side's pool or surrendered box (a leader never
    // placed is not lost), and only once.
    Position
    noteLeaders(const TrcParts& parts, const StepCall& call)
    {
      Position next = call.ctx.position;
      for (UnitId leader : {parts.facts.hitler(), parts.facts.stalin()}) {
        const SideId side = call.ctx.roster.unit(leader).side;
        const bool lostP = Units::inSpaceP(next, leader, parts.facts.pool(side)) ||
                           Units::inSpaceP(next, leader, parts.facts.surrendered(side));
        std::optional<LeaderLost>& lost = stateOf(next).side(side).leaderLost;
        if (lostP && !lost) {
          lost = LeaderLost::Pending;
        }
      }
      return next;
    }

    // 6.2: after each combat, once nothing of the battle is owed.
    Position
    stackingAfterCombat(const TrcParts& parts, const StepCall& call)
    {
      Position next = call.ctx.position;
      if (!next.resolution().empty()) {
        return next;  // 13.3: nothing is re-evaluated until the battle's last answer
      }
      for (std::size_t h = 0; h < call.ctx.board.hexCount(); ++h) {
        const HexIndex hex{static_cast<std::uint32_t>(h)};
        if (next.unitsAt(hex).empty()) {
          continue;
        }
        const Ctx now{call.ctx.board, call.ctx.rules, call.ctx.roster, next};
        for (UnitId unit : parts.stacking.excess(now, hex)) {
          Units::remove(next, unit, parts.facts.pool(call.ctx.roster.unit(unit).side), call.sink);
        }
      }
      return next;
    }

  }  // namespace

  void
  registerTrcCommandSteps(HexEngine::StepRegistry& registry, const TrcParts& parts)
  {
    registry.addCheck("check-attack", [parts](const CheckCall& call) {
      checkAttack(parts, call.ctx, commandOf<HexEngine::DeclareAttack>(call));
      return;
    });
    registry.addCheck("check-mandatory-attacks", [parts](const CheckCall& call) {
      parts.mandatory.check(call.ctx);
      return;
    });
    registry.addEffect("count-rail-moves", [](const StepCall& call) { return countRailMoves(call); });
    registry.addEffect("note-rail-touched", [parts](const StepCall& call) {
      return parts.rail.noteTouched(call.ctx, commandOf<HexEngine::MoveUnit>(call));
    });
    registry.addEffect("note-warsaw", [parts](const StepCall& call) {
      return parts.politics.noteWarsaw(call.ctx, commandOf<HexEngine::MoveUnit>(call));
    });
    registry.addEffect("count-air-used", [](const StepCall& call) { return countAirUsed(call); });
    registry.addEffect("workers-surrender", [parts](const StepCall& call) { return workersSurrender(parts, call); });
    registry.addEffect("note-leaders", [parts](const StepCall& call) { return noteLeaders(parts, call); });
    registry.addEffect("repair-stacking-after-combat",
                       [parts](const StepCall& call) { return stackingAfterCombat(parts, call); });
    registry.addEffect("update-control", [parts](const StepCall& call) { return parts.control.update(call.ctx, call.sink); });
    return;
  }

}  // namespace Trc
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------

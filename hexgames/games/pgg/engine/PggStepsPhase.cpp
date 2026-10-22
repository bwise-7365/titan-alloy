// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
// Enter and end effects (4.2). A game turn opens by bringing waiting reinforcements a turn nearer. A
// movement phase opens with the supply check for movement (11.0) and, for the Soviet, the first-turn
// army dice (5.21), the turn's schedule owed (16.1) and the Provisional Reinforcement (14.1); it
// closes with stacking (7.1, 7.2), the phase's marks cleared and, for the Soviet, the Air Interdiction
// Markers removed (13.33). A combat phase closes with stacking. A Disruption Removal Phase opens by
// removing the side's Disruption (6.63). The Soviet Player-Turn closes by scoring recaptures (15.12);
// the German Player-Turn by recording the cities held and removing the Soviet marker (13.46). After
// Game-Turn Twelve the level of victory is declared (15.2).
// ----------------------------------------------
#include "PggState.h"
#include "PggSteps.h"
#include "PggUnits.h"

namespace Pgg {

  namespace {

    using HexEngine::StepCall;

    SideId
    phaseSide(const PggParts& parts, const StepCall& call)
    {
      const std::optional<SideId> side = parts.facts.phase(call.ctx.position.clock().phase).side;
      if (!side) {
        throw std::invalid_argument("Pgg: step '" + call.step.id + "' (does '" + call.step.does +
                                    "') needs a phase that belongs to a side");
      }
      return *side;
    }

    // The side of the phase being entered or left: the clock's for an enter step, the step's own phase
    // for an end step, which runs before the clock moves.
    Position
    movementSupply(const PggParts& parts, const StepCall& call)
    {
      const SideId side = phaseSide(parts, call);
      const HexEngine::SupplyReport report = parts.supply.trace(call.ctx, call.scratch, side);
      Position next = call.ctx.position;
      PggSideState& mine = stateOf(next).side(side);
      mine.unsupplied = std::set<UnitId>(report.unsupplied.begin(), report.unsupplied.end());
      for (UnitId unit : report.unsupplied) {
        call.sink.onEvent(HexEngine::SupplyChecked{unit, false});
      }
      mine.beyondRadius.clear();
      if (parts.facts.soviet() == side) {
        for (UnitId unit : Units::onMap(call.ctx, parts.facts, side)) {
          if (!parts.facts.leaderP(unit) && !parts.supply.withinRadiusP(call.ctx, call.scratch, unit)) {
            mine.beyondRadius.insert(unit);  // 6.55
          }
        }
      }
      return next;
    }

    Position
    repairStacking(const PggParts& parts, const StepCall& call)
    {
      Position next = call.ctx.position;
      for (std::size_t h = 0; h < call.ctx.board.hexCount(); ++h) {
        const HexIndex hex{static_cast<std::uint32_t>(h)};
        if (next.unitsAt(hex).empty()) {
          continue;
        }
        const Ctx now{call.ctx.board, call.ctx.rules, call.ctx.roster, next};
        for (UnitId unit : parts.stacking.excess(now, hex)) {
          Units::eliminate(parts.facts, next, unit, call.sink);
        }
      }
      return next;
    }

    Position
    clearMovementMarks(const PggParts& parts, const StepCall& call)
    {
      Position next = call.ctx.position;
      PggState& state = stateOf(next);
      PggSideState& mine = state.side(phaseSide(parts, call));
      mine.spent.clear();
      mine.halted.clear();
      mine.continuing.clear();
      state.passedByGermans.clear();
      for (SideId side : {parts.facts.german(), parts.facts.soviet()}) {
        state.side(side).retreatedOnto.clear();
      }
      return next;
    }

    Position
    clearRetreated(const PggParts& parts, const StepCall& call)
    {
      Position next = call.ctx.position;
      for (SideId side : {parts.facts.german(), parts.facts.soviet()}) {
        stateOf(next).side(side).retreatedOnto.clear();
      }
      return next;
    }

    Position
    removeDisruption(const PggParts& parts, const StepCall& call)
    {
      Position next = call.ctx.position;
      std::set<UnitId>& disrupted = stateOf(next).side(phaseSide(parts, call)).disrupted;
      for (UnitId unit : disrupted) {
        call.sink.onEvent(HexEngine::GameEvent{"disruption-removed", call.ctx.roster.unit(unit).counter.text});
      }
      disrupted.clear();
      return next;
    }

    Position
    clearPlayerTurn(const PggParts& parts, const StepCall& call)
    {
      Position next = call.ctx.position;
      PggState& state = stateOf(next);
      const SideId side = phaseSide(parts, call);
      state.side(side).entered.clear();
      if (parts.facts.soviet() == side) {
        state.railUnits = 0;
        state.swfThisTurn = 0;
      }
      return next;
    }

  }  // namespace

  void
  registerPggPhaseSteps(HexEngine::StepRegistry& registry, const PggParts& parts)
  {
    // ---- game turn ----
    registry.addEffect("age-arrivals", [parts](const StepCall& call) { return parts.arrivals.ageArrivals(call.ctx); });
    registry.addEffect("declare-victory", [parts](const StepCall& call) {
      return parts.victory.declare(call.ctx, call.scratch, call.sink);
    });
    // ---- movement phases ----
    registry.addEffect("check-movement-supply", [parts](const StepCall& call) { return movementSupply(parts, call); });
    registry.addEffect("roll-first-turn-armies", [parts](const StepCall& call) {
      return parts.firstTurn.rollArmies(call.ctx, call.streams, call.sink);
    });
    registry.addEffect("schedule-reinforcements", [parts](const StepCall& call) { return parts.arrivals.scheduleOwed(call.ctx); });
    registry.addEffect("roll-provisional-reinforcement", [parts](const StepCall& call) {
      return parts.arrivals.rollProvisional(call.ctx, call.streams, call.sink);
    });
    registry.addEffect("remove-air-interdiction", [parts](const StepCall& call) {
      return parts.interdiction.removeAir(call.ctx, call.sink);
    });
    registry.addEffect("clear-movement-marks", [parts](const StepCall& call) { return clearMovementMarks(parts, call); });
    // ---- movement and combat phases ----
    registry.addEffect("repair-stacking", [parts](const StepCall& call) { return repairStacking(parts, call); });
    registry.addEffect("clear-retreated", [parts](const StepCall& call) { return clearRetreated(parts, call); });
    // ---- disruption removal ----
    registry.addEffect("remove-disruption", [parts](const StepCall& call) { return removeDisruption(parts, call); });
    // ---- player turns ----
    registry.addEffect("score-recaptures", [parts](const StepCall& call) {
      return parts.victory.scoreRecaptures(call.ctx, call.sink);
    });
    registry.addEffect("record-german-cities", [parts](const StepCall& call) {
      return parts.victory.recordGermanCities(call.ctx, call.scratch);
    });
    registry.addEffect("remove-soviet-interdiction", [parts](const StepCall& call) {
      return parts.interdiction.removeSoviet(call.ctx, call.sink);
    });
    registry.addEffect("clear-player-turn", [parts](const StepCall& call) { return clearPlayerTurn(parts, call); });
    return;
  }

}  // namespace Pgg
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------

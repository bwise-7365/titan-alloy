// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
// Enter and end effects (4.1). The rules document places each: the Weather Phase opens with the
// roll, the turn marker, the reinforcement delays and Italy; a movement phase opens by clearing the
// last impulse's markers and waking a lost leader's freeze, with the Warsaw garrison and Finland
// (Axis) or replacement points (Russian); a movement phase closes by removing exposed partisans
// (Axis) and spending the freeze; a combat phase closes with the mandatory-attack surrenders, the
// end of the air commitment and (Russian second impulse) the partisans' lift; an end phase closes
// with rail conversion, then supply elimination (4.1.6), then (Russian) the minor allies'
// surrenders, then the turn's state is cleared.
// ----------------------------------------------
#include "TrcMarkers.h"
#include "TrcState.h"
#include "TrcSteps.h"
#include "TrcUnits.h"

namespace Trc {

  namespace {

    using HexEngine::StepCall;

    SideId
    phaseSide(const TrcParts& parts, const StepCall& call)
    {
      const std::optional<SideId> side = parts.facts.phase(call.ctx.position.clock().phase).side;
      if (!side) {
        throw std::invalid_argument("Trc: step '" + call.step.id + "' (does '" + call.step.does +
                                    "') needs a phase that belongs to a side");
      }
      return *side;
    }

    Position
    advanceTurnMarker(const TrcParts& parts, const StepCall& call)
    {
      Position next = call.ctx.position;
      next.setTrack(parts.facts.turnTrack(), call.ctx.position.clock().turn);
      return next;
    }

    Position
    ageReinforcements(const StepCall& call)
    {
      Position next = call.ctx.position;
      for (const UnitSpec& spec : call.ctx.roster.units()) {
        std::optional<int>& delay = next.state(spec.id).delay;
        if (delay && 0 < *delay) {
          *delay -= 1;
        }
      }
      return next;
    }

    Position
    clearImpulseMarkers(const TrcParts& parts, const StepCall& call)
    {
      Position next = call.ctx.position;
      for (UnitId unit : call.ctx.roster.ofSide(phaseSide(parts, call))) {
        Markers::unmark(next, unit, Markers::kAv);
        Markers::unmark(next, unit, Markers::kRailed);
      }
      return next;
    }

    // 11.3: "pending" becomes "active" as the nation's next movement phase opens, "spent" as it closes.
    Position
    moveLeaderFreeze(const TrcParts& parts, const StepCall& call, LeaderLost from, LeaderLost to)
    {
      Position next = call.ctx.position;
      std::optional<LeaderLost>& lost = stateOf(next).side(phaseSide(parts, call)).leaderLost;
      if (from == lost) {
        lost = to;
      }
      return next;
    }

    Position
    endAirCommitment(const TrcParts& parts, const StepCall& call)
    {
      Position next = call.ctx.position;
      stateOf(next).side(phaseSide(parts, call)).airUsed.reset();
      return next;
    }

    // 17.2: out of supply is fatal in the owner's end phase.
    Position
    eliminateUnsupplied(const TrcParts& parts, const StepCall& call)
    {
      const SideId side = phaseSide(parts, call);
      Position next = call.ctx.position;
      const HexEngine::SupplyReport report = parts.supply.trace(call.ctx, call.scratch, side);
      for (UnitId unit : report.supplied) {
        next.state(unit).flags.isolatedP = false;
      }
      for (UnitId unit : report.unsupplied) {
        call.sink.onEvent(HexEngine::SupplyChecked{unit, false});
        Units::remove(next, unit, parts.facts.pool(side), call.sink);
      }
      return next;
    }

    Position
    clearTurnState(const TrcParts& parts, const StepCall& call)
    {
      const SideId side = phaseSide(parts, call);
      Position next = call.ctx.position;
      TrcSideState& turn = stateOf(next).side(side);
      turn.railMoves.reset();
      turn.seaUsed.clear();
      turn.southEntry.reset();
      turn.replaced.clear();
      turn.replacementPoints.reset();
      turn.replacedArmour.reset();
      turn.replacedGuards.reset();
      for (UnitId unit : call.ctx.roster.ofSide(side)) {
        Markers::unmark(next, unit, Markers::kInvaded);
        Markers::unmark(next, unit, Markers::kAvFirst);
      }
      return next;
    }

  }  // namespace

  void
  registerTrcPhaseSteps(HexEngine::StepRegistry& registry, const TrcParts& parts)
  {
    // ---- the Weather Phase ----
    registry.addEffect("roll-weather", [parts](const StepCall& call) {
      return parts.weather.roll(call.ctx, call.streams, call.sink);
    });
    registry.addEffect("advance-turn-marker", [parts](const StepCall& call) { return advanceTurnMarker(parts, call); });
    registry.addEffect("age-reinforcements", [](const StepCall& call) { return ageReinforcements(call); });
    registry.addEffect("italy-surrenders", [parts](const StepCall& call) { return parts.politics.italy(call.ctx, call.sink); });
    // ---- movement phases ----
    registry.addEffect("clear-impulse-markers", [parts](const StepCall& call) { return clearImpulseMarkers(parts, call); });
    registry.addEffect("wake-leader-freeze", [parts](const StepCall& call) {
      return moveLeaderFreeze(parts, call, LeaderLost::Pending, LeaderLost::Active);
    });
    registry.addEffect("spend-leader-freeze", [parts](const StepCall& call) {
      return moveLeaderFreeze(parts, call, LeaderLost::Active, LeaderLost::Spent);
    });
    registry.addEffect("garrison-warsaw", [parts](const StepCall& call) {
      return parts.politics.garrisonWarsaw(call.ctx, call.sink);
    });
    registry.addEffect("finland-surrenders-1944", [parts](const StepCall& call) {
      return parts.politics.finland1944(call.ctx, call.sink);
    });
    registry.addEffect("grant-replacement-points", [parts](const StepCall& call) {
      return parts.arrivals.grantReplacementPoints(call.ctx, call.streams, call.sink);
    });
    registry.addEffect("remove-exposed-partisans", [parts](const StepCall& call) {
      return parts.politics.removeExposedPartisans(call.ctx, call.sink);
    });
    // ---- combat phases ----
    registry.addEffect("surrender-debtors", [parts](const StepCall& call) {
      return parts.mandatory.surrenderDebtors(call.ctx, call.sink);
    });
    registry.addEffect("end-air-commitment", [parts](const StepCall& call) { return endAirCommitment(parts, call); });
    registry.addEffect("lift-partisans", [parts](const StepCall& call) {
      return parts.politics.liftPartisans(call.ctx, call.sink);
    });
    // ---- end phases ----
    registry.addEffect("convert-rail", [parts](const StepCall& call) {
      return parts.rail.convert(call.ctx, phaseSide(parts, call), call.scratch, call.sink);
    });
    registry.addEffect("eliminate-unsupplied", [parts](const StepCall& call) { return eliminateUnsupplied(parts, call); });
    registry.addEffect("minor-ally-surrenders", [parts](const StepCall& call) {
      return parts.politics.russianEndPhase(call.ctx, call.sink);
    });
    registry.addEffect("clear-turn-state", [parts](const StepCall& call) { return clearTurnState(parts, call); });
    // ---- the Sudden Death phase ----
    registry.addEffect("record-sudden-death", [parts](const StepCall& call) {
      return parts.victory.recordSuddenDeath(call.ctx, call.sink);
    });
    return;
  }

}  // namespace Trc
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------

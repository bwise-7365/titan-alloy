// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
// TrcGame at the phase boundaries (4.1). Closing: a combat phase settles its mandatory attacks and
// ends the side's air commitments; an Axis movement phase removes exposed partisans (19.3); the
// Russian second impulse lifts every partisan (19.4); a player-turn end phase converts rail, then
// eliminates the unsupplied, then (Russian) resolves surrenders, then clears the side's turn state.
// Opening: the Weather Phase rolls, ages the waiting reinforcements and surrenders Italy on its
// date; an Axis first impulse checks Finland from 1944; a movement phase wakes a lost leader's
// freeze and the Warsaw garrison; the Russian first impulse grants replacement points; the Sudden
// Death phase records any condition met.
// ----------------------------------------------
#include "TrcGame.h"

#include "TrcFlags.h"
#include "TrcUnits.h"

namespace Trc {

  namespace {

    Ctx
    at(const Ctx& ctx, const Position& position)
    {
      return Ctx{ctx.board, ctx.rules, ctx.roster, position};
    }

  }  // namespace

  Position
  TrcGame::closeSideTurn(const Ctx& ctx, SideId side, HexSearch::SearchScratch& scratch, HexEngine::EventSink& sink) const
  {
    const TrcFacts& facts = parts_.facts;
    Position next = parts_.rail.convert(ctx, side, scratch, sink);
    const HexEngine::SupplyReport report = parts_.supply.trace(at(ctx, next), scratch, side);
    for (UnitId unit : report.supplied) {
      next.state(unit).flags.isolatedP = false;
    }
    for (UnitId unit : report.unsupplied) {
      sink.onEvent(HexEngine::SupplyChecked{unit, false});
      Units::remove(next, unit, facts.pool(side), sink);  // 17.2: fatal in the owner's end phase
    }
    if (facts.russian() == side) {
      next = parts_.politics.russianEndPhase(at(ctx, next), sink);
    }
    for (const std::string& counter : {Flags::kRailMoves, Flags::kSeaUsed, Flags::kSouthEntry, Flags::kReplaced,
                                       Flags::kReplacementPoints, std::string("replaced-armour"),
                                       std::string("replaced-guards")}) {
      next.setFlag(side, counter, std::nullopt);
    }
    for (UnitId unit : ctx.roster.ofSide(side)) {
      Flags::unmark(next, unit, Flags::kInvaded);
      Flags::unmark(next, unit, Flags::kAvFirst);
    }
    return next;
  }

  Position
  TrcGame::endPhase(const Ctx& ctx, HexSearch::SearchScratch& scratch, HexEngine::EventSink& sink) const
  {
    const TrcFacts& facts = parts_.facts;
    const PhaseInfo phase = facts.phase(ctx.position.clock().phase);
    Position next = ctx.position;
    switch (phase.kind) {
      case PhaseKind::Combat:
        next = parts_.mandatory.surrenderDebtors(ctx, sink);
        next.setFlag(*phase.side, Flags::kAirUsed, std::nullopt);
        if (facts.russian() == phase.side && Impulse::Second == phase.impulse) {
          next = parts_.politics.liftPartisans(at(ctx, next), sink);
        }
        return next;
      case PhaseKind::Move:
        if (facts.axis() == phase.side) {
          next = parts_.politics.removeExposedPartisans(ctx, sink);
        }
        if ("active" == next.flag(*phase.side, Flags::kLeaderLost).value_or("")) {
          next.setFlag(*phase.side, Flags::kLeaderLost, "spent");
        }
        return next;
      case PhaseKind::End:
        return closeSideTurn(ctx, *phase.side, scratch, sink);
      case PhaseKind::Weather:
      case PhaseKind::SuddenDeath:
        return next;
    }
    throw std::invalid_argument("TrcGame::endPhase: phase kind outside TRC's sequence");
  }

  Position
  TrcGame::startTurn(const Ctx& ctx, HexEngine::PrngStreams& streams, HexEngine::EventSink& sink) const
  {
    Position next = parts_.weather.roll(ctx, streams, sink);
    next.setTrack(parts_.facts.turnTrack(), ctx.position.clock().turn);  // the turn marker advances
    for (const UnitSpec& spec : ctx.roster.units()) {
      std::optional<int>& delay = next.state(spec.id).delay;
      if (delay && 0 < *delay) {
        *delay -= 1;
      }
    }
    return parts_.politics.italy(at(ctx, next), sink);
  }

  Position
  TrcGame::openImpulse(const Ctx& ctx, SideId side, Impulse impulse, HexEngine::PrngStreams& streams,
                       HexEngine::EventSink& sink) const
  {
    const TrcFacts& facts = parts_.facts;
    Position next = ctx.position;
    for (UnitId unit : ctx.roster.ofSide(side)) {
      Flags::unmark(next, unit, Flags::kAv);
      Flags::unmark(next, unit, Flags::kRailed);
    }
    if ("pending" == next.flag(side, Flags::kLeaderLost).value_or("")) {
      next.setFlag(side, Flags::kLeaderLost, "active");
    }
    if (facts.axis() == side) {
      next = parts_.politics.garrisonWarsaw(at(ctx, next), sink);
      if (Impulse::First == impulse) {
        next = parts_.politics.finland1944(at(ctx, next), sink);
      }
    } else if (Impulse::First == impulse) {
      next = parts_.arrivals.grantReplacementPoints(at(ctx, next), streams, sink);
    }
    return next;
  }

  Position
  TrcGame::enterPhase(const Ctx& ctx, HexEngine::PrngStreams& streams, HexEngine::EventSink& sink) const
  {
    const PhaseInfo phase = parts_.facts.phase(ctx.position.clock().phase);
    switch (phase.kind) {
      case PhaseKind::Weather:
        return startTurn(ctx, streams, sink);
      case PhaseKind::Move:
        return openImpulse(ctx, *phase.side, *phase.impulse, streams, sink);
      case PhaseKind::SuddenDeath:
        return parts_.victory.recordSuddenDeath(ctx, sink);
      case PhaseKind::Combat:
      case PhaseKind::End:
        return ctx.position;
    }
    throw std::invalid_argument("TrcGame::enterPhase: phase kind outside TRC's sequence");
  }

}  // namespace Trc
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------

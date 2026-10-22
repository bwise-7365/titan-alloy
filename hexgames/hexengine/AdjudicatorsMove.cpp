// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
// Movement, retreat, stacking repair, supply and the clock.
// ----------------------------------------------
#include "hexengine/AdjudicatorsDetail.h"

#include <algorithm>
#include <set>
#include <stdexcept>
#include <string>

namespace HexEngine::Adjudicators {

  using Detail::boxFor;
  using Detail::hexOf;
  using Detail::removeToBox;

  Position
  applyMove(const Ctx& ctx, const Policies&, const MoveUnit& command, EventSink& sink)
  {
    if (2 > command.path.size()) {
      throw std::invalid_argument("applyMove: a move needs a path of at least two hexes");
    }
    if (command.units.empty()) {
      throw std::invalid_argument("applyMove: a move needs at least one unit");
    }
    Position next = ctx.position;
    const HexIndex destination = command.path.back();
    const SideId side = ctx.roster.unit(command.units.front()).side;

    for (UnitId unit : command.units) {
      next.place(unit, destination);
      next.state(unit).flags.movedP = true;
      sink.onEvent(UnitMoved{unit, command.path});
    }

    // Control of a drawn feature passes to the last player to occupy it.
    for (std::size_t i = 1; i < command.path.size(); ++i) {
      const HexIndex hex = command.path[i];
      if (ctx.board.features(hex).empty()) {
        continue;
      }
      const std::optional<SideId> owner = next.control(hex);
      if (!owner || *owner != side) {
        next.setControl(hex, side);
        sink.onEvent(ControlChanged{hex, side});
      }
    }
    return next;
  }

  Position
  applyStackingRepair(const Ctx& ctx, const Policies& policies, EventSink& sink)
  {
    if (nullptr == policies.stacking) {
      throw std::invalid_argument("applyStackingRepair: the policy set has no StackingPolicy");
    }
    Position next = ctx.position;
    std::set<std::uint32_t> occupied;
    for (const UnitSpec& spec : ctx.roster.units()) {
      if (const std::optional<HexIndex> hex = hexOf(next, spec.id)) {
        occupied.insert(hex->value);
      }
    }
    for (std::uint32_t value : occupied) {
      const HexIndex hex{value};
      const Ctx here{ctx.board, ctx.rules, ctx.roster, next};
      for (UnitId unit : policies.stacking->excess(here, hex)) {
        removeToBox(next, unit, boxFor(ctx, ctx.roster.unit(unit).side, true), sink);
      }
    }
    return next;
  }

  Position
  applySupplyCheck(const Ctx& ctx, const Policies& policies, HexSearch::SearchScratch& scratch, SideId side,
                    EventSink& sink)
  {
    if (nullptr == policies.supply) {
      throw std::invalid_argument("applySupplyCheck: the policy set has no SupplyTrace");
    }
    Position next = ctx.position;
    const SupplyReport report = policies.supply->trace(ctx, scratch, side);
    for (UnitId unit : report.supplied) {
      next.state(unit).flags.isolatedP = false;
      sink.onEvent(SupplyChecked{unit, true});
    }
    for (UnitId unit : report.unsupplied) {
      next.state(unit).flags.isolatedP = true;
      sink.onEvent(SupplyChecked{unit, false});
    }
    return next;
  }

  Position
  advancePhase(const Ctx& ctx, const PhaseCursor::Stop& stop, EventSink& sink)
  {
    Position next = ctx.position;
    next.clock().turn = stop.turn;
    next.clock().phase = stop.phase;
    next.clock().actingSide = stop.side;
    for (const UnitSpec& spec : ctx.roster.units()) {
      HexModel::UnitFlags& flags = next.state(spec.id).flags;
      flags.movedP = false;
      flags.attackedP = false;
      flags.defendedP = false;
    }
    sink.onEvent(PhaseEntered{stop.turn, stop.phase, stop.side});
    return next;
  }

}  // namespace HexEngine::Adjudicators
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------

// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
#include "PggMovement.h"

#include "PggState.h"
#include "PggTerrain.h"
#include "PggUnits.h"

#include <algorithm>
#include <array>
#include <stdexcept>

namespace Pgg {

  namespace {

    constexpr std::array<std::string_view, 8> kClaims{
        "zoc-stop-and-leave", "leader-movement",       "soviet-forbidden-hexrows", "out-of-supply-movement",
        "german-interdiction-effect", "order-rail-cut-latency", "supply-rail-cut-blocks-supply", "rail-cut-place"};

    const HexEngine::EntryVerdict kProhibited{HexModel::Prohibited{}, false};

  }  // namespace

  PggMovement::PggMovement(const PggFacts& facts, const PggZoc& zoc) : facts_(facts), zoc_(zoc)
  {
  }

  std::span<const std::string_view>
  PggMovement::claims() const
  {
    return kClaims;
  }

  int
  PggMovement::interdictionHalves(const Ctx& ctx, SideId mover, HexIndex hex, bool railP) const
  {
    const PggState& state = stateOf(ctx.position);
    if (facts_.soviet() == mover) {
      const bool markedP = state.airInterdiction.end() != std::find(state.airInterdiction.begin(),
                                                                    state.airInterdiction.end(), hex);
      if (!markedP) {
        return 0;
      }
      return railP ? 8 : 2;  // 13.31, 13.32
    }
    return hex == state.sovietInterdiction ? 2 : 0;  // 13.43
  }

  std::optional<int>
  PggMovement::normalHalves(const Ctx& ctx, UnitId unit, HexIndex from, Direction direction) const
  {
    const SideId side = ctx.roster.unit(unit).side;
    const std::optional<HexIndex> to = ctx.board.neighbour(from, direction);
    const std::optional<int> terrain = terrainHalves(facts_, facts_.moveClass(unit), side, from, direction);
    if (!to || !terrain) {
      return std::nullopt;
    }
    if (zoc_.blockedForP(ctx, from, side, HexRules::Purpose::Movement)) {
      return std::nullopt;  // 8.13: no unit leaves an enemy zone of its own will
    }
    const int turn = ctx.position.clock().turn;
    if (facts_.soviet() == side && 6 >= turn && facts_.westmostColumnsP(*to)) {
      const PggState& state = stateOf(ctx.position);
      const auto army = state.armies.find(unit);
      const bool exceptedP = 1 == turn && state.armies.end() != army && facts_.westmostColumnsP(from) &&
                             (Army::Thirteenth == army->second || Army::Twentieth == army->second);
      if (!exceptedP) {
        return std::nullopt;  // 6.4, but for the armies of 5.23 moving out on turn 1
      }
    }
    return *terrain + interdictionHalves(ctx, side, *to, false);
  }

  std::optional<int>
  PggMovement::railHalves(const Ctx& ctx, UnitId unit, HexIndex from, Direction direction) const
  {
    const SideId side = ctx.roster.unit(unit).side;
    const std::optional<HexIndex> to = ctx.board.neighbour(from, direction);
    if (facts_.soviet() != side || !to || !facts_.railLink(from, *to)) {
      return std::nullopt;
    }
    const PggState& state = stateOf(ctx.position);
    const int turn = ctx.position.clock().turn;
    for (HexIndex end : {from, *to}) {
      const auto repaired = state.railRepaired.find(end);
      if (state.railCuts.contains(end) || (state.railRepaired.end() != repaired && turn == repaired->second)) {
        return std::nullopt;  // errata 6.37
      }
    }
    if (zoc_.blockedForP(ctx, *to, side, HexRules::Purpose::Network)) {
      return std::nullopt;  // 6.32
    }
    return 2 + interdictionHalves(ctx, side, *to, true);
  }

  HexEngine::EntryVerdict
  PggMovement::enter(const Ctx& ctx, UnitId unit, HexIndex from, Direction direction, ModeId mode) const
  {
    std::optional<int> halves;
    if (facts_.normalMode() == mode) {
      halves = normalHalves(ctx, unit, from, direction);
    } else if (facts_.railMode() == mode) {
      halves = railHalves(ctx, unit, from, direction);
    }
    if (!halves) {
      return kProhibited;
    }
    return HexEngine::EntryVerdict{MovementPoints{*halves}, false};
  }

  bool
  PggMovement::movementPhaseP(const Ctx& ctx, UnitId unit) const
  {
    if (facts_.markerP(unit)) {
      return false;
    }
    const PhaseInfo phase = facts_.phase(ctx.position.clock().phase);
    const SideId side = ctx.roster.unit(unit).side;
    if (phase.side != side) {
      return false;
    }
    switch (phase.kind) {
      case PhaseKind::Move:
        return true;
      case PhaseKind::MechanizedMove:
        return Arm::GermanInfantry != facts_.arm(unit);  // 4.2 B.3
      case PhaseKind::SetUp:
      case PhaseKind::Combat:
      case PhaseKind::DisruptionRemoval:
      case PhaseKind::SovietInterdiction:
      case PhaseKind::AirInterdiction:
      case PhaseKind::Container:
        return false;
    }
    throw std::invalid_argument("PggMovement: phase kind outside PGG's sequence");
  }

  int
  PggMovement::fullHalves(const Ctx& ctx, UnitId unit) const
  {
    const int printed = Units::allowanceOf(ctx, unit);
    const SideId side = ctx.roster.unit(unit).side;
    if (stateOf(ctx.position).side(side).unsupplied.contains(unit)) {
      return 2 * std::max(1, printed / 2);  // 11.31, 11.32
    }
    return 2 * printed;
  }

  int
  PggMovement::remainingHalves(const Ctx& ctx, UnitId unit) const
  {
    if (!movementPhaseP(ctx, unit)) {
      return 0;
    }
    const PggSideState& mine = stateOf(ctx.position).side(ctx.roster.unit(unit).side);
    if (mine.disrupted.contains(unit) || mine.halted.contains(unit)) {
      return 0;
    }
    const auto spent = mine.spent.find(unit);
    return std::max(0, fullHalves(ctx, unit) - (mine.spent.end() == spent ? 0 : spent->second));
  }

  int
  PggMovement::normalAllowance(const Ctx& ctx, UnitId unit) const
  {
    const PggSideState& mine = stateOf(ctx.position).side(ctx.roster.unit(unit).side);
    if (ctx.position.unit(unit).flags.movedP && !mine.continuing.contains(unit)) {
      return 0;  // one move a unit, unless an overrun freed it (6.51)
    }
    return remainingHalves(ctx, unit);
  }

  int
  PggMovement::railAllowance(const Ctx& ctx, UnitId unit) const
  {
    const SideId side = ctx.roster.unit(unit).side;
    if (facts_.soviet() != side || !movementPhaseP(ctx, unit) || ctx.position.unit(unit).flags.movedP) {
      return 0;
    }
    const PggState& state = stateOf(ctx.position);
    const PggSideState& mine = state.side(side);
    const std::optional<HexIndex> hex = Units::hexOf(ctx.position, unit);
    if (mine.disrupted.contains(unit) || mine.halted.contains(unit) || !hex || !facts_.railHexP(*hex)) {
      return 0;
    }
    if (kRailUnits < state.railUnits + facts_.railPoints(unit)) {
      return 0;
    }
    return kRailHalves;
  }

  Budget
  PggMovement::allowance(const Ctx& ctx, UnitId unit, ModeId mode) const
  {
    if (facts_.normalMode() == mode) {
      return MovementPoints{normalAllowance(ctx, unit)};
    }
    if (facts_.railMode() == mode) {
      return MovementPoints{railAllowance(ctx, unit)};
    }
    throw std::invalid_argument("PggMovement: mode '" + ctx.rules.movement().modes[mode.value].id +
                                "' is a command of its own (overrun), not a move");
  }

  bool
  PggMovement::stopsInP(const Ctx&, UnitId, HexIndex, ModeId) const
  {
    return false;
  }

  std::optional<int>
  PggMovement::pathHalves(const Ctx& ctx, UnitId unit, const std::vector<HexIndex>& path, ModeId mode) const
  {
    int total = 0;
    for (std::size_t i = 1; i < path.size(); ++i) {
      const std::optional<Direction> direction = facts_.directionTo(path[i - 1], path[i]);
      if (!direction) {
        return std::nullopt;
      }
      const HexEngine::EntryVerdict verdict = enter(ctx, unit, path[i - 1], *direction, mode);
      if (std::holds_alternative<HexModel::Prohibited>(verdict.cost)) {
        return std::nullopt;
      }
      total += std::get<MovementPoints>(verdict.cost).halves;
    }
    return total;
  }

  int
  PggMovement::entryHalves(const Ctx& ctx, UnitId unit, HexIndex hex) const
  {
    int halves = 2;
    if (facts_.woodsP(hex)) {
      halves = MoveClass::Motor == facts_.moveClass(unit) ? 4 : 2;
    } else if (facts_.swampP(hex)) {
      halves = 4;
    }
    return halves + interdictionHalves(ctx, ctx.roster.unit(unit).side, hex, false);
  }

}  // namespace Pgg
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------

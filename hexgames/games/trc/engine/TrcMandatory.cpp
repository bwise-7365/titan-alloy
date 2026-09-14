// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
#include "TrcMandatory.h"

#include "TrcFlags.h"
#include "TrcUnits.h"

#include <array>
#include <stdexcept>

namespace Trc {

  namespace {

    constexpr std::array<std::string_view, 1> kClaims{"av-trap"};

  }  // namespace

  TrcMandatory::TrcMandatory(const TrcFacts& facts, const TrcZoc& zoc, const TrcCombat& combat)
    : facts_(facts), zoc_(zoc), combat_(combat)
  {
  }

  std::span<const std::string_view>
  TrcMandatory::claims()
  {
    return kClaims;
  }

  // 9.1 (a unit that moved by rail does nothing else that impulse) and 16.2.
  bool
  TrcMandatory::mayAttackP(const Ctx& ctx, UnitId unit) const
  {
    const PhaseInfo phase = facts_.phase(ctx.position.clock().phase);
    if (ctx.position.unit(unit).flags.attackedP || Flags::markedP(ctx.position, unit, Flags::kRailed) ||
        Flags::markedP(ctx.position, unit, Flags::kAv)) {
      return false;
    }
    return !(Impulse::Second == phase.impulse && Flags::markedP(ctx.position, unit, Flags::kAvFirst));
  }

  std::vector<HexIndex>
  TrcMandatory::unattackedNeighbours(const Ctx& ctx, UnitId unit) const
  {
    const HexIndex hex = *Units::hexOf(ctx.position, unit);
    const SideId side = ctx.roster.unit(unit).side;
    std::vector<HexIndex> out;
    for (int d = 0; d < HexCoord::kDirections; ++d) {
      const std::optional<HexIndex> near = ctx.board.neighbour(hex, static_cast<Direction>(d));
      if (!near) {
        continue;
      }
      bool owingP = false;
      for (UnitId enemy : ctx.position.unitsAt(*near)) {
        owingP = owingP || (side != ctx.roster.unit(enemy).side && !facts_.typeP(enemy, "partisan") &&
                            !ctx.position.unit(enemy).flags.defendedP && zoc_.projectsIntoP(ctx, enemy, *near, hex));
      }
      if (owingP) {
        out.push_back(*near);
      }
    }
    return out;
  }

  bool
  TrcMandatory::owesP(const Ctx& ctx, UnitId unit) const
  {
    if (facts_.typeP(unit, "partisan") || ctx.position.unit(unit).flags.attackedP) {
      return false;
    }
    const PhaseInfo phase = facts_.phase(ctx.position.clock().phase);
    if (Impulse::First == phase.impulse && Flags::markedP(ctx.position, unit, Flags::kAv)) {
      return false;  // 16.2: may not attack now; 16.3 judges it in the second impulse
    }
    return !unattackedNeighbours(ctx, unit).empty();
  }

  bool
  TrcMandatory::canAttackP(const Ctx& ctx, UnitId unit) const
  {
    if (!mayAttackP(ctx, unit)) {
      return false;
    }
    const SideId side = ctx.roster.unit(unit).side;
    for (HexIndex target : unattackedNeighbours(ctx, unit)) {
      std::vector<UnitId> defenders;
      for (UnitId enemy : ctx.position.unitsAt(target)) {
        if (side != ctx.roster.unit(enemy).side) {
          defenders.push_back(enemy);
        }
      }
      const HexModel::OddsOutcome odds = combat_.odds(combat_.factors(ctx, {unit}, target, defenders));
      if (!std::holds_alternative<HexModel::BelowMinimum>(odds)) {
        return true;
      }
    }
    return false;
  }

  void
  TrcMandatory::check(const Ctx& ctx) const
  {
    const std::optional<SideId> acting = ctx.position.clock().actingSide;
    for (UnitId unit : Units::onMap(ctx, *acting)) {
      if (owesP(ctx, unit) && canAttackP(ctx, unit)) {
        throw std::invalid_argument("TrcMandatory: counter '" + ctx.roster.unit(unit).counter.text +
                                     "' is in an enemy zone of control and must attack before the phase ends (12.1)");
      }
    }
    return;
  }

  Position
  TrcMandatory::surrenderDebtors(const Ctx& ctx, HexEngine::EventSink& sink) const
  {
    Position next = ctx.position;
    const std::optional<SideId> acting = ctx.position.clock().actingSide;
    for (UnitId unit : Units::onMap(ctx, *acting)) {
      if (owesP(ctx, unit)) {
        Units::remove(next, unit, facts_.surrendered(*acting), sink);  // 12.5, 16.3
      }
    }
    return next;
  }

}  // namespace Trc
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------

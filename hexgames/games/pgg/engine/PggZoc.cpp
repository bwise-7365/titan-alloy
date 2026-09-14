// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
#include "PggZoc.h"

#include "PggState.h"
#include "PggUnits.h"

#include <array>

namespace Pgg {

  namespace {

    constexpr std::array<std::string_view, 3> kClaims{"zoc-disrupted-none", "zoc-mutual", "untried-zoc"};

  }  // namespace

  PggZoc::PggZoc(const PggFacts& facts) : facts_(facts)
  {
  }

  std::span<const std::string_view>
  PggZoc::claims() const
  {
    return kClaims;
  }

  bool
  PggZoc::projectsIntoP(const Ctx& ctx, UnitId unit, HexIndex from, HexIndex to) const
  {
    if (facts_.markerP(unit)) {
      return false;
    }
    const SideId side = ctx.roster.unit(unit).side;
    if (stateOf(ctx.position).side(side).disrupted.contains(unit)) {
      return false;  // 8.11
    }
    if (from == to) {
      return true;
    }
    const std::optional<Direction> direction = facts_.directionTo(from, to);
    return direction && !facts_.lakeHexsideP(from, *direction);  // 8.17
  }

  bool
  PggZoc::enemyZocP(const Ctx& ctx, HexIndex hex, SideId side) const
  {
    const auto projectsFromP = [&](HexIndex from) {
      for (UnitId occupant : ctx.position.unitsAt(from)) {
        if (side != ctx.roster.unit(occupant).side && projectsIntoP(ctx, occupant, from, hex)) {
          return true;
        }
      }
      return false;
    };
    if (projectsFromP(hex)) {
      return true;
    }
    for (int d = 0; d < HexCoord::kDirections; ++d) {
      const std::optional<HexIndex> near = ctx.board.neighbour(hex, static_cast<Direction>(d));
      if (near && projectsFromP(*near)) {
        return true;
      }
    }
    return false;
  }

  bool
  PggZoc::blockedForP(const Ctx& ctx, HexIndex hex, SideId mover, HexRules::Purpose purpose) const
  {
    switch (purpose) {
      case HexRules::Purpose::Movement:
      case HexRules::Purpose::Network:
      case HexRules::Purpose::Zoc:
      case HexRules::Purpose::Control:
      case HexRules::Purpose::Grouping:
        return enemyZocP(ctx, hex, mover);
      case HexRules::Purpose::Supply:
      case HexRules::Purpose::Retreat:
        return enemyZocP(ctx, hex, mover) && !Units::friendlyAtP(ctx, hex, mover);  // 8.15
    }
    throw std::invalid_argument("PggZoc::blockedForP: purpose outside the rules document's list");
  }

}  // namespace Pgg
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------

// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
#include "TrcZoc.h"

#include <array>

namespace Trc {

  namespace {

    constexpr std::array<std::string_view, 1> kClaims{"zoc-partisan"};

  }  // namespace

  TrcZoc::TrcZoc(const TrcFacts& facts) : facts_(facts)
  {
  }

  std::span<const std::string_view>
  TrcZoc::claims() const
  {
    return kClaims;
  }

  bool
  TrcZoc::adjacentUnblockedP(const Ctx& ctx, HexIndex from, HexIndex to) const
  {
    for (int d = 0; d < HexCoord::kDirections; ++d) {
      const Direction direction = static_cast<Direction>(d);
      if (ctx.board.neighbour(from, direction) == to) {
        return !facts_.blockedHexsideP(from, direction) && !facts_.kerchP(from, to) && !facts_.waterP(to);
      }
    }
    return false;
  }

  bool
  TrcZoc::projectsIntoP(const Ctx& ctx, UnitId unit, HexIndex from, HexIndex to) const
  {
    const std::string zoc = ctx.rules.unitTypes()[ctx.roster.unit(unit).type.value].zoc.value_or("full");
    if ("none" == zoc) {
      return false;
    }
    if (from == to) {
      return true;
    }
    if ("own-hex" == zoc) {
      return false;
    }
    return adjacentUnblockedP(ctx, from, to);
  }

  bool
  TrcZoc::enemyZocP(const Ctx& ctx, HexIndex hex, SideId side, bool partisansCountP) const
  {
    const SideId enemy = facts_.enemy(side);
    const auto projectsFromP = [&](HexIndex from) {
      for (UnitId occupant : ctx.position.unitsAt(from)) {
        if (enemy != ctx.roster.unit(occupant).side) {
          continue;
        }
        if (!partisansCountP && facts_.typeP(occupant, "partisan")) {
          continue;
        }
        if (projectsIntoP(ctx, occupant, from, hex)) {
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

  // Movement, retreat and the rail network feel a partisan's zone; supply, control and the
  // mandatory-attack test (Purpose::Zoc) do not (19.2).
  bool
  TrcZoc::blockedForP(const Ctx& ctx, HexIndex hex, SideId mover, HexRules::Purpose purpose) const
  {
    switch (purpose) {
      case HexRules::Purpose::Movement:
      case HexRules::Purpose::Retreat:
      case HexRules::Purpose::Network:
        return enemyZocP(ctx, hex, mover, true);
      case HexRules::Purpose::Supply:
      case HexRules::Purpose::Control:
      case HexRules::Purpose::Zoc:
        return enemyZocP(ctx, hex, mover, false);
      case HexRules::Purpose::Grouping:
        return false;
    }
    throw std::invalid_argument("TrcZoc::blockedForP: purpose outside the rules document's list");
  }

}  // namespace Trc
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------

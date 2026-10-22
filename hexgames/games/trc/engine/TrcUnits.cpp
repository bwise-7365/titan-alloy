// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
#include "TrcUnits.h"

namespace Trc::Units {

  std::optional<HexIndex>
  hexOf(const Position& position, UnitId unit)
  {
    const std::optional<Location>& where = position.unit(unit).where;
    if (!where || !std::holds_alternative<HexIndex>(*where)) {
      return std::nullopt;
    }
    return std::get<HexIndex>(*where);
  }

  bool
  inSpaceP(const Position& position, UnitId unit, SpaceId space)
  {
    const std::optional<Location>& where = position.unit(unit).where;
    return where && std::holds_alternative<SpaceId>(*where) && space == std::get<SpaceId>(*where);
  }

  std::vector<UnitId>
  onMap(const Ctx& ctx, SideId side)
  {
    std::vector<UnitId> out;
    for (UnitId unit : ctx.roster.ofSide(side)) {
      if (hexOf(ctx.position, unit)) {
        out.push_back(unit);
      }
    }
    return out;
  }

  bool
  enemyAtP(const Ctx& ctx, HexIndex hex, SideId side)
  {
    for (UnitId occupant : ctx.position.unitsAt(hex)) {
      if (side != ctx.roster.unit(occupant).side) {
        return true;
      }
    }
    return false;
  }

  void
  remove(Position& position, UnitId unit, std::optional<SpaceId> box, HexEngine::EventSink& sink)
  {
    if (box) {
      position.place(unit, *box);
    } else {
      position.remove(unit);
    }
    sink.onEvent(HexEngine::UnitEliminated{unit, box});
    return;
  }

  void
  place(Position& position, UnitId unit, HexIndex hex, HexEngine::EventSink& sink)
  {
    position.place(unit, hex);
    sink.onEvent(HexEngine::UnitPlaced{unit, hex});
    return;
  }

  bool
  withinP(const Ctx& ctx, HexIndex a, HexIndex b, int range)
  {
    return range >= ctx.board.distance(a, b);
  }

}  // namespace Trc::Units
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------

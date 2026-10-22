// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
// Small position questions and mutations the TRC adjudicators share.
// ----------------------------------------------
#pragma once
#include "TrcFacts.h"

namespace Trc::Units {

  std::optional<HexIndex> hexOf(const Position&, UnitId);
  bool inSpaceP(const Position&, UnitId, SpaceId);
  // The side's units standing on the map, in roster order.
  std::vector<UnitId> onMap(const Ctx&, SideId);
  bool enemyAtP(const Ctx&, HexIndex, SideId side);
  // Moves a unit into a box (or out of the game when nullopt) and reports it.
  void remove(Position&, UnitId, std::optional<SpaceId>, HexEngine::EventSink&);
  void place(Position&, UnitId, HexIndex, HexEngine::EventSink&);
  // The hexes around `hex` within `range`, the hex itself included.
  bool withinP(const Ctx&, HexIndex a, HexIndex b, int range);

}  // namespace Trc::Units
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------

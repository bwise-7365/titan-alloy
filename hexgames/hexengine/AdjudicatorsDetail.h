// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
// The handful of steps the adjudicators share: where an eliminated or surrendered counter goes,
// what a unit's hex is, and the fixed order a defender's own losses are taken in.
// ----------------------------------------------
#pragma once
#include "hexengine/Adjudicators.h"

#include <optional>
#include <vector>

namespace HexEngine::Adjudicators::Detail {

  // The hex a unit stands on, or nullopt when it is off the map.
  std::optional<HexIndex> hexOf(const Position&, UnitId);

  // The side's own box for a counter that leaves play: a space the rules give that side whose
  // @returns matches (a replacement pool when it may come back, a surrendered box when it may not).
  std::optional<SpaceId> boxFor(const Ctx&, SideId, bool returnsP);

  // Takes one step from a unit, reporting it; a unit with no step left leaves play for `box`.
  void loseStep(const Ctx&, Position&, UnitId, std::optional<SpaceId> box, EventSink&);

  // Removes a unit from play into `box`, or out of the game when there is none.
  void removeToBox(Position&, UnitId, std::optional<SpaceId> box, EventSink&);

  // The units of `side` taking part in the battle, weakest first, then by counter id: the order the
  // engine settles a loss the defender owes, so that a replay never depends on a stack's order.
  std::vector<UnitId> lossOrder(const Ctx&, const Position&, const std::vector<UnitId>& involved, SideId);

}  // namespace HexEngine::Adjudicators::Detail
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------

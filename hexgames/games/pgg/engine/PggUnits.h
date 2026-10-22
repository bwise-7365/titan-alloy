// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
// Small helpers every PGG part shares: where a unit is, the face it shows, and the three ways a
// counter leaves or changes in combat -- a step lost (9.61-9.63: a German counter turns over, a
// German infantry division's second counter replaces its first, a Soviet unit is gone), elimination
// (a Soviet division to the dead pile face down, 12.4) and revelation (12.2).
// ----------------------------------------------
#pragma once
#include "PggFacts.h"
#include "PggState.h"

#include "hexengine/Event.h"

#include <optional>
#include <vector>

namespace Pgg::Units {

  std::optional<HexIndex> hexOf(const Position&, UnitId);
  bool inSpaceP(const Position&, UnitId, SpaceId);
  // The side's units on the map, markers excluded, in roster order.
  std::vector<UnitId> onMap(const Ctx&, const PggFacts&, SideId);
  bool enemyAtP(const Ctx&, HexIndex, SideId);
  bool friendlyAtP(const Ctx&, HexIndex, SideId);

  // The strengths of the face in play: a German counter's back once it has lost a step.
  const Strengths& face(const Ctx&, UnitId);
  int attackOf(const Ctx&, UnitId);   // 0 when the face prints none (a Leader)
  int defenceOf(const Ctx&, UnitId);
  int allowanceOf(const Ctx&, UnitId);  // whole movement points

  void place(Position&, UnitId, HexIndex, HexEngine::EventSink&);
  void reveal(Position&, UnitId, HexEngine::EventSink&);
  void eliminate(const PggFacts&, Position&, UnitId, HexEngine::EventSink&);
  void loseStep(const PggFacts&, Position&, UnitId, HexEngine::EventSink&);

}  // namespace Pgg::Units
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------

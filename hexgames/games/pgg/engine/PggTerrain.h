// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
// The Terrain Effects Chart (6.7) as one function, shared by movement and by the German twenty-point
// supply trace (11.12): the cost, in half movement points, of crossing one hexside into a hex.
// Along a road the road's cost replaces the terrain's, unless a River hexside lies between the two
// hexes (11.13: roads do not cross rivers for movement); otherwise Clear and cities cost 1, Forest 1
// on foot and 2 for the rest, Swamp 2, and a River hexside adds 2 for a German unit and 1 for a
// Soviet one. A Lake hex or Lake hexside may not be entered (nullopt).
// ----------------------------------------------
#pragma once
#include "PggFacts.h"

#include <optional>

namespace Pgg {

  std::optional<int> terrainHalves(const PggFacts&, MoveClass, SideId mover, HexIndex from, Direction);

}  // namespace Pgg
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------

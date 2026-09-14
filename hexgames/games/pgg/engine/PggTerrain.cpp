// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
#include "PggTerrain.h"

namespace Pgg {

  std::optional<int>
  terrainHalves(const PggFacts& facts, MoveClass mover, SideId side, HexIndex from, Direction direction)
  {
    const std::optional<HexIndex> to = facts.definition().board->neighbour(from, direction);
    if (!to || facts.lakeHexsideP(from, direction)) {
      return std::nullopt;
    }
    const bool riverP = facts.riverP(from, direction);
    if (!riverP && facts.roadP(from, *to)) {
      return MoveClass::Foot == mover ? 2 : 1;
    }
    int halves = 2;
    if (facts.woodsP(*to)) {
      halves = MoveClass::Motor == mover ? 4 : 2;
    } else if (facts.swampP(*to)) {
      halves = 4;
    }
    if (riverP) {
      halves += facts.german() == side ? 4 : 2;
    }
    return halves;
  }

}  // namespace Pgg
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------

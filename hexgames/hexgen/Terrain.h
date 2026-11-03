// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
// Terrain by role and quota (HexKrieg TerrainAssignment). With N hexes and a role's percent p, the
// role takes floor(p N / 100) hexes, except the city role, which takes cityCount(N, p):
//   low    the lowest by height;
//   high   the highest by height (assigned after low, so it wins an overlap);
//   cover  of the hexes still rest, those with the most cover;
//   city   a shuffle of every hex not low, the first cityCount of it, whatever it was.
// Sorts are stable, so equal values keep hex order. Altitude, if asked for, is the height scaled to
// metres about the waterline, midway between the highest low hex and the lowest other one.
// ----------------------------------------------
#pragma once
#include "hexgen/GenMap.h"
#include "hexgen/Settings.h"

#include <random>

namespace HexGen {

  // round(softMin(p N / 100, 200)), softMin(x, y) = x y / sqrt(x^2 + y^2) (0 if either is 0), so
  // that a large map has at most about 200 cities (HexKrieg HexGraph.cityCount).
  int cityCount(int hexes, int percent);

  // Fills m.terrain from m.height and m.cover; cityGen draws the city shuffle.
  void assignTerrain(GenMap&, const Settings&, std::mt19937_64& cityGen);

  // Fills m.alt from m.height (HexKrieg TerrainAssignment.assignWithHeight).
  void calibrateAltitude(GenMap&, const Settings&);

}  // namespace HexGen
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------

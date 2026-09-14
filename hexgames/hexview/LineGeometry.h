// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
// [PROPOSED contract, M10] The line geometry settled in the reference renderer on 2026-09-14 (PLAN.md
// decision log "Smoothed roads and railways", "Smoothed rivers", "Visual parity"), as pure functions
// over pixels so that the Qt6 and HTML front ends draw exactly the same lines:
//   - link networks (roads, railways) are smoothed after panj/tempest: through a hex with two
//     neighbours on the network, hexside midpoint to midpoint; at an end or junction, midpoint to
//     centre; joined into polylines from one end or junction to the next;
//   - rivers (and any hexside line a MapStyle names) have every corner rounded;
//   - political boundaries and all other hexside lines stay straight, exactly along the hexsides.
// Output order is stable (document order of the inputs) so SVG goldens compare byte for byte.
// ----------------------------------------------
#pragma once
#include "hexview/MapFrame.h"
#include "hexview/Scene.h"

#include <utility>
#include <vector>

namespace HexView {

  using Polyline = std::vector<Pixel>;

  // A link network, given as its chains of neighbouring printed hexes, drawn smoothed. A loop of
  // pass-through hexes closes on itself. Throws std::invalid_argument naming a hex no grid prints.
  std::vector<Polyline> smoothedLinks(const std::vector<std::vector<HexId>>& chains, const MapFrame&);

  // Hexsides of one line joined into chains of hex corners, each from an end or junction to the next;
  // a closed loop repeats its first corner last. Corners computed from neighbouring hexes are matched
  // with a tolerance, since their coordinates differ by float noise.
  std::vector<Polyline> cornerChains(const std::vector<std::pair<Pixel, Pixel>>& hexsides);

  // A corner chain with its corners rounded: straight to the first side's midpoint, a quadratic curve
  // round each interior corner to the next side's midpoint, straight to the last corner; a closed
  // chain is rounded all the way round. Ends and junctions stay on their corners.
  std::vector<PathCommand> roundedCorners(const Polyline& chain);

  // A corner chain drawn straight along its hexsides.
  std::vector<PathCommand> straightChain(const Polyline& chain);

}  // namespace HexView
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------

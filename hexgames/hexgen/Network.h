// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
// The road network (HexKrieg TerrainBuilder): the cities are joined by a minimum spanning tree over
// their cheapest-path costs (CostGraph) plus a closed tour through them by furthest insertion, and
// each pair so chosen is laid along its cheapest path. Cities no road can join (on two land masses)
// are not joined, so each land mass gets its own tree and tour; HexKrieg's water costs 10000 and
// joins them across it.
// Then, on each map edge, up to `roads` hexes of the outermost row or column (not a corner, and a
// terrain a road may enter) are chosen at random, each at least length / (2 roads) from the others,
// so fewer may be found; the first `cities` of them become cities; each, in order along the edge,
// is joined by its cheapest path to the nearest hex already on a road, unless it is on one.
// ----------------------------------------------
#pragma once
#include "hexgen/GenMap.h"
#include "hexgen/Settings.h"

#include <cstdint>
#include <limits>
#include <random>
#include <utility>
#include <vector>

namespace HexGen {

  inline constexpr std::int64_t kNoPath = std::numeric_limits<std::int64_t>::max();

  // Lays the network between the hexes of the city role.
  void buildRoads(GenMap&, const Settings&);

  // Lays one edge's roads; returns the edge hexes chosen, in order along the edge.
  std::vector<int> addEdgeRoads(GenMap&, const Settings&, const SideSetting&, std::mt19937_64&);

  // The two pieces of the network, on a symmetric cost matrix where kNoPath marks no path. Prim's
  // tree grown from the lowest unjoined node, so a matrix with no paths between groups gives one tree
  // per group; ties go to the lower index. Pairs are (lower, higher).
  std::vector<std::pair<int, int>> spanningForest(const std::vector<std::vector<std::int64_t>>& cost);
  // The closed tour through `members` (all joined to each other) by furthest insertion: start with
  // the costliest pair; repeatedly insert the outsider farthest from the tour where it adds least.
  std::vector<int> furthestInsertionTour(const std::vector<std::vector<std::int64_t>>& cost,
                                         const std::vector<int>& members);

}  // namespace HexGen
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------

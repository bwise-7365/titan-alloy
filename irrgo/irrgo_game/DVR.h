// Copyright Ben Paul Wise. All Rights Reserved.
#pragma once
#include "Game.h"
#include <vector>

namespace IrrGo {

// Discrete Voronoi Region for one color, bounded by a BFS radius.
// A node belongs to DVR(color, radius) iff it is reachable from a stone of
// that color in at most `radius` hops AND it is strictly closer to that color
// than to any stone of the opposite color (ties belong to neither).
class DVR {
public:
    DVR(const Game& game, Color color, int radius);

    int size() const { return static_cast<int>(nodes_.size()); }
    const std::vector<int>& nodes() const { return nodes_; }

private:
    std::vector<int> nodes_;
};

} // namespace IrrGo
// Copyright Ben Paul Wise. All Rights Reserved.

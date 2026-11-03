// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
#include "hexgen/GenMap.h"

#include <algorithm>
#include <optional>
#include <stdexcept>
#include <string>

namespace HexGen {

  GenMap::GenMap(const Settings& s)
      : grid_(gridSpecOf(s)), cols_(s.cols), rows_(s.rows),
        next_(static_cast<std::size_t>(s.cols * s.rows)), roadDegree_(static_cast<std::size_t>(s.cols * s.rows), 0)
  {
    for (int hex = 0; hex < size(); ++hex) {
      const HexCoord::HexCentre here = centre(hex);
      for (int d = 0; d < HexCoord::kDirections; ++d) {
        const std::optional<HexCoord::GridIndex> there =
            grid_.indexOf(here.neighbour(static_cast<HexCoord::Direction>(d)));
        next_[static_cast<std::size_t>(hex)][static_cast<std::size_t>(d)] =
            there ? indexOf(there->col, there->row) : kNoHex;
      }
    }
  }

  HexCoord::HexCentre
  GenMap::centre(int hex) const
  {
    return grid_.centreOf(HexCoord::GridIndex{hex % cols_, hex / cols_});
  }

  void
  GenMap::addRoad(int a, int b)
  {
    const std::array<int, 6>& around = neighbours(a);
    if (around.end() == std::find(around.begin(), around.end(), b)) {
      throw std::invalid_argument("GenMap::addRoad: hexes " + std::to_string(a) + " and " +
                                  std::to_string(b) + " are not neighbours");
    }
    if (roads_.insert(a < b ? std::pair{a, b} : std::pair{b, a}).second) {
      ++roadDegree_[static_cast<std::size_t>(a)];
      ++roadDegree_[static_cast<std::size_t>(b)];
    }
    return;
  }

}  // namespace HexGen
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------

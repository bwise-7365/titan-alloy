// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
// The generated map as a HexSearch::SearchGraph for laying roads: an arc from each hex to each
// neighbour on the grid, arc index hex * 6 + direction. A step costs the sum of its two hexes'
// road-cost in hundredths (an integer, so equal paths tie exactly and searches are repeatable); a
// step into or out of a terrain with no road-cost cannot be taken. Costs are read from the map's
// current terrain, so a hex made a city later costs what a city costs.
// ----------------------------------------------
#pragma once
#include "hexgen/GenMap.h"
#include "hexgen/Settings.h"

#include "hexsearch/Search.h"

#include <array>
#include <climits>
#include <cmath>
#include <cstddef>
#include <functional>
#include <optional>
#include <vector>

namespace HexGen {

  class CostGraph {
  public:
    CostGraph(const GenMap& m, const Settings& s) : map_(m)
    {
      for (const TerrainSetting& t : s.terrains) {
        hundredths_.push_back(t.roadCost ? std::optional<int>(static_cast<int>(std::lround(*t.roadCost * 100.0)))
                                         : std::nullopt);
      }
    }

    std::size_t nodeCount() const { return static_cast<std::size_t>(map_.size()); }

    void
    forEachArc(HexSearch::NodeIndex n, const std::function<void(HexSearch::NodeIndex, std::size_t)>& visit) const
    {
      const std::array<int, 6>& around = map_.neighbours(static_cast<int>(n));
      for (std::size_t d = 0; d < around.size(); ++d) {
        if (kNoHex != around[d]) {
          visit(static_cast<HexSearch::NodeIndex>(around[d]), n * 6 + d);
        }
      }
      return;
    }

    bool roadsEnterP(int hex) const { return hundredths_[map_.terrain[static_cast<std::size_t>(hex)]].has_value(); }

    std::optional<HexSearch::CostHalves>
    arcCost(HexSearch::NodeIndex from, std::size_t arc) const
    {
      const int to = map_.neighbours(static_cast<int>(from))[arc % 6];
      const std::optional<int> a = hundredths_[map_.terrain[from]];
      const std::optional<int> b = hundredths_[map_.terrain[static_cast<std::size_t>(to)]];
      if (!a || !b) {
        return std::nullopt;
      }
      return HexSearch::CostHalves{*a + *b};
    }

    // Every hex a road can reach from `from`, with its cheapest path. The ceiling leaves room for one
    // more step (at most 2 * kMaxRoadCost hundredths), so no sum overflows.
    HexSearch::Field<HexSearch::CostHalves>
    searchFrom(int from, HexSearch::SearchScratch& scratch) const
    {
      const std::array<HexSearch::Source, 1> source{HexSearch::Source{static_cast<HexSearch::NodeIndex>(from)}};
      return HexSearch::Algorithms::dijkstraBounded(
          *this, scratch, source, [this](HexSearch::NodeIndex n, std::size_t arc) { return arcCost(n, arc); },
          HexSearch::CostHalves{INT_MAX - 40000}, nullptr);
    }

  private:
    const GenMap& map_;
    std::vector<std::optional<int>> hundredths_;  // by terrain index
  };

}  // namespace HexGen
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------

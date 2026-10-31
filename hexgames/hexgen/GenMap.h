// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
// The one model the generator's steps work on: the sheet's grid and, per hex, the fields and the
// terrain the steps fill in, with the road steps between neighbouring hexes. Hexes are numbered
// row-major (row * cols + col), the order Grid::ids() lists them in; geometry goes through the
// grid's ABC centres, and (col, row) appears only in that numbering.
// ----------------------------------------------
#pragma once
#include "hexgen/Settings.h"

#include "hexcoord/Grid.h"

#include <array>
#include <cstddef>
#include <set>
#include <utility>
#include <vector>

namespace HexGen {

  inline constexpr int kNoHex = -1;

  class GenMap {
  public:
    explicit GenMap(const Settings&);

    const HexCoord::Grid& grid() const { return grid_; }
    int cols() const { return cols_; }
    int rows() const { return rows_; }
    int size() const { return cols_ * rows_; }
    int indexOf(int col, int row) const { return row * cols_ + col; }
    HexCoord::HexCentre centre(int hex) const;
    HexCoord::Pixel pixel(int hex) const { return grid_.pixelOf(centre(hex)); }
    // The neighbour across each hexside, Direction order D0..D5; kNoHex off the grid.
    const std::array<int, 6>& neighbours(int hex) const { return next_[static_cast<std::size_t>(hex)]; }

    std::vector<double> height;       // the height field at each hex centre
    std::vector<double> cover;        // the cover field, which places the cover role
    std::vector<std::size_t> terrain;  // index into Settings::terrains
    std::vector<double> alt;          // metres; empty unless the settings ask for altitude

    // Road steps between neighbouring hexes, each as (lower index, higher index).
    const std::set<std::pair<int, int>>& roads() const { return roads_; }
    void addRoad(int a, int b);
    bool onRoadP(int hex) const { return 0 < roadDegree_[static_cast<std::size_t>(hex)]; }

  private:
    HexCoord::Grid grid_;
    int cols_;
    int rows_;
    std::vector<std::array<int, 6>> next_;
    std::set<std::pair<int, int>> roads_;
    std::vector<int> roadDegree_;
  };

}  // namespace HexGen
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------

// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
#pragma once
// Scratch -- the "hand-scratched" line of irrgo's Latrunculi board (irrgo/irregular_grids/
// irregular_grid.cpp), for the editor's presentation of roads, railways and rivers. Presentation only:
// the document never sees it.
//
// Model, after irrgo:
//   1. The polyline is sampled samplesPerUnit times per unit of its length, plus a closing sample.
//   2. Each interior sample gets an independent perpendicular deviation roughness * (U(0,1) - 0.5) * unit:
//      up to +/-0.5 unit at roughness = 1. The two end samples are pinned to the exact line.
//   3. The deviations are relaxed by Gauss-Seidel sweeps toward the fixed point of
//          s(k) = (1 - smoothing) * n(k) + smoothing * (s(k-1) + s(k+1)) / 2
//      until the largest change in a sweep is below 1e-4 unit.
//   smoothing = 0 keeps the raw noise; smoothing = 1 gives back the straight line; roughness = 0 is
//   always the line itself. The same seed gives the same wobble, so a redraw does not shimmer.

#include "hexcoord/Grid.h"

#include <cstdint>
#include <vector>

namespace HexView {

  struct ScratchSpec {
    double roughness;       // [0, 1]
    double smoothing;       // [0, 1]
    double unit;            // the length the noise is measured in (a hex's circumradius), > 0
    int samplesPerUnit;     // >= 1
    std::uint64_t seed;
  };

  // The scratched polyline; the input itself when roughness is 0. Throws std::invalid_argument on a
  // spec out of range or a polyline of fewer than two points, std::runtime_error if the relaxation does
  // not converge.
  std::vector<HexCoord::Pixel> scratch(const std::vector<HexCoord::Pixel>& polyline, const ScratchSpec&);

  // irrgo's canonical uniform: the top 53 bits of one mt19937_64 draw, identical on every platform.
  double canonicalUniform(std::uint64_t draw);

}  // namespace HexView
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------

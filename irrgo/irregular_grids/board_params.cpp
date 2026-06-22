// Copyright Ben Paul Wise. All Rights Reserved.

#include "board_params.h"

#include <cmath>
#include <stdexcept>

namespace games::board {

void validate(const BoardParams& params) {
    if (params.rows < kMinRowsCols || params.rows > kMaxRowsCols) {
        throw std::invalid_argument("board rows out of range [kMinRowsCols, kMaxRowsCols]");
    }
    if (params.columns < kMinRowsCols || params.columns > kMaxRowsCols) {
        throw std::invalid_argument("board columns out of range [kMinRowsCols, kMaxRowsCols]");
    }
    requireUnit(params.stone_fraction, "stone_fraction");
    requireUnit(params.roughness, "roughness");
    requireUnit(params.smoothing, "smoothing");
}

int stones_per_side(const BoardParams& params) {
    validate(params);
    const double squares =
        static_cast<double>(params.rows) * static_cast<double>(params.columns);
    return static_cast<int>(std::lround(params.stone_fraction * squares));
}

GridSpec to_grid_spec(const BoardParams& params) {
    validate(params);
    GridSpec spec;
    spec.rows = params.rows;
    spec.columns = params.columns;
    spec.roughness = params.roughness;
    spec.smoothing = params.smoothing;
    return spec;
}

}  // namespace games::board
// Copyright Ben Paul Wise. All Rights Reserved.

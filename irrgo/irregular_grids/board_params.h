// Copyright Ben Paul Wise. All Rights Reserved.
#pragma once

#include "irregular_grid.h"  // GridSpec

namespace games::board {

// Game-board (not rendering) parameters, gathered in one place.
//
// The renderer itself (GridSpec) accepts any positive size; the game-level size
// limits and the stone-count policy live here.

// Allowed board-size range, in squares, for this Latrunculi-style game.
inline constexpr int kMinRowsCols = 6;
inline constexpr int kMaxRowsCols = 12;

// Default share of the board each side fills (20 stones on a nominal 8x8).
inline constexpr double kDefaultStoneFraction = 20.0 / 64.0;

// High-level board parameters chosen by the caller. rows and columns must lie in
// [kMinRowsCols, kMaxRowsCols]; stone_fraction, roughness and smoothing in [0,1].
struct BoardParams {
    int rows = 8;
    int columns = 10;
    double stone_fraction = kDefaultStoneFraction;
    double roughness = 0.1;              // grid-line roughness
    double smoothing = 0.95;             // grid-line smoothing
};

// Throws std::invalid_argument if any field is out of range.
void validate(const BoardParams& params);

// Stones placed per side: round(stone_fraction * rows * columns). Validates.
int stones_per_side(const BoardParams& params);

// The renderer's GridSpec for these parameters. Validates.
GridSpec to_grid_spec(const BoardParams& params);

}  // namespace games::board
// Copyright Ben Paul Wise. All Rights Reserved.

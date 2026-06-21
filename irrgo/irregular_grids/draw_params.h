// Copyright Ben Paul Wise. All Rights Reserved.
#pragma once

#include <string>

#include "irregular_grid.h"  // RenderConfig, SvgStyle, DiscSpec, BoardSpec

namespace games::board {

// SVG / drawing parameters, gathered in one place. These factories are the
// canonical source for the look of the output; the struct defaults in
// irregular_grid.hpp are kept consistent with them and serve the convenience
// `= {}` overloads.

// Fill colours for the two opposing sides.
struct PieceColors {
    std::string side_a = "#2aa85a";  // pastel green
    std::string side_b = "#c76fe8";  // purplish
};

// Pixel scale, sampling and RNG seed.
RenderConfig default_render_config();

// Background, ink, strokes, margins and label sizing.
SvgStyle default_svg_style();

// The disc stamped into each board cell (radius <= 0.5 so one fits per square).
DiscSpec board_disc_spec();

// The large disc used for the standalone disc.svg demo.
DiscSpec standalone_disc_spec();

// The two side colours.
PieceColors piece_colors();

// Sets every drawing-related field of `board` (disc shape, piece outline, outer
// box, and X-mark) from the defaults. Leaves board.grid and board.pieces alone.
void apply_draw_defaults(BoardSpec& board);

}  // namespace games::board
// Copyright Ben Paul Wise. All Rights Reserved.

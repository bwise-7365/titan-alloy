// Copyright Ben Paul Wise. All Rights Reserved.
#pragma once

#include <cstdint>
#include <string>

#include "AbsGame.h"  // AbsGame::makeSeed for the default RNG seed
#include "utils.h"    // AbsGame::dSeed default seed value

namespace games::board {

// The geometry/board value types these factories return or fill in. They are
// defined in irregular_grid.h, which includes this header for RenderConfig /
// SvgStyle; forward declarations here keep that dependency one-directional.
struct DiscSpec;
struct BoardSpec;

// Default drawing parameters (square-width units unless noted). Single source of
// truth: the struct defaults below (and on BoardSpec in irregular_grid.h) and the
// apply_draw_defaults() factory all reference these, so they cannot drift apart.
inline constexpr const char* kBlackInk = "#000000";
inline constexpr double kStrokeWidthUnits = 0.0375;
inline constexpr double kMarginUnits = 1.0;
inline constexpr double kLabelGapUnits = 0.30;
inline constexpr double kLabelFontUnits = 0.32;
inline constexpr double kOutlineWidthUnits = 0.02;
inline constexpr double kOuterMarginUnits = 0.25;
inline constexpr double kMarkLengthFraction = 0.6;
inline constexpr double kMarkStrokeWidthUnits = 0.0375;

// Resolution and the pixel scale used only at SVG-render time.
struct RenderConfig {
    double square_size = 100.0;     // pixels per square (SVG output only)
    int points_per_edge = 10;     // samples per square along each line
    // Deterministic across platforms given a non-zero seed; 0 means "clock-derived".
    std::uint64_t seed = AbsGame::makeSeed(AbsGame::dSeed);
};

// Cosmetic SVG settings. An empty background string omits the background rect.
struct SvgStyle {
    std::string background = "#8C8E7E";
    std::string ink = "#000060";
    double stroke_width_units = kStrokeWidthUnits;  // square-width units
    double margin_units = kMarginUnits;             // gutter on every side
    double label_gap_units = kLabelGapUnits;        // board edge -> label centre
    double label_font_units = kLabelFontUnits;      // coordinate label height
};

// SVG / drawing parameter factories, gathered in one place. They are the
// canonical source for the look of the output; the struct defaults above are kept
// consistent with them and serve the convenience `= {}` overloads.

// Fill colors for the two opposing sides.
struct PieceColors {
    //std::string side_a = "#2aa85a";  // pastel green
    //std::string side_b = "#c76fe8";  // purplish
    // these might go well with a nearly white background like "FFFFEE" or "#FFFFF4";

    std::string side_a = "#FAE5BE"; // pale beige
    std::string side_b = "#852532"; // brick-red
    // these might go well with a nearly white "#FFFFF4" or complementary #8C8E7E
};

// Pixel scale, sampling and RNG seed.
RenderConfig default_render_config();

// Background, ink, strokes, margins and label sizing.
SvgStyle default_svg_style();

// The disc stamped into each board cell (radius <= 0.5 so one fits per square).
DiscSpec board_disc_spec();

// The large disc used for the standalone disc.svg demo.
DiscSpec standalone_disc_spec();

// The two side colors.
PieceColors piece_colors();

// Sets every drawing-related field of `board` (disc shape, piece outline, outer
// box, and X-mark) from the defaults. Leaves board.grid and board.pieces alone.
void apply_draw_defaults(BoardSpec& board);

}  // namespace games::board

// Copyright Ben Paul Wise. All Rights Reserved.

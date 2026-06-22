// Copyright Ben Paul Wise. All Rights Reserved.

#include "draw_params.h"
#include "irregular_grid.h"  // complete DiscSpec / BoardSpec for the factory bodies

namespace games::board {

namespace {
// Disc shape shared by the board pieces and the standalone demo disc; only the
// radius differs between the two.
constexpr double kDiscRoughness = 0.25;
constexpr double kDiscSmoothing = 0.95;
constexpr int kDiscPointCount = 90;
constexpr double kBoardDiscRadius = 0.4;       // fits one disc per 1x1 cell
constexpr double kStandaloneDiscRadius = 2.0;  // the large standalone demo disc
}  // namespace

RenderConfig default_render_config() {
    // Struct defaults already hold square_size, points_per_edge and the seed.
    return RenderConfig{};
}

SvgStyle default_svg_style() {
    // Struct defaults already hold colours, strokes, margins and label sizing.
    return SvgStyle{};
}

DiscSpec board_disc_spec() {
    return DiscSpec{kBoardDiscRadius, kDiscRoughness, kDiscSmoothing, kDiscPointCount};
}

DiscSpec standalone_disc_spec() {
    return DiscSpec{kStandaloneDiscRadius, kDiscRoughness, kDiscSmoothing, kDiscPointCount};
}

PieceColors piece_colors() {
    return PieceColors{};
}

void apply_draw_defaults(BoardSpec& board) {
    board.disc = board_disc_spec();
    board.outline = kBlackInk;
    board.outline_width_units = kOutlineWidthUnits;
    board.outer_margin_units = kOuterMarginUnits;
    board.mark_color = kBlackInk;
    board.mark_length_fraction = kMarkLengthFraction;
    board.mark_stroke_width_units = kMarkStrokeWidthUnits;
}

}  // namespace games::board
// Copyright Ben Paul Wise. All Rights Reserved.

// Copyright Ben Paul Wise. All Rights Reserved.

#include "draw_params.hpp"

namespace games::board {

RenderConfig default_render_config() {
    // Struct defaults already hold square_size, points_per_edge and the seed.
    return RenderConfig{};
}

SvgStyle default_svg_style() {
    // Struct defaults already hold colours, strokes, margins and label sizing.
    return SvgStyle{};
}

DiscSpec board_disc_spec() {
    return DiscSpec{0.4, 0.25, 0.95, 90};
}

DiscSpec standalone_disc_spec() {
    return DiscSpec{2.0, 0.25, 0.95, 90};
}

PieceColors piece_colors() {
    return PieceColors{};
}

void apply_draw_defaults(BoardSpec& board) {
    board.disc = board_disc_spec();
    board.outline = "#000000";
    board.outline_width_units = 0.02;
    board.outer_margin_units = 0.25;
    board.mark_color = "#000000";
    board.mark_length_fraction = 0.6;
    board.mark_stroke_width_units = 0.0375;
}

}  // namespace games::board
// Copyright Ben Paul Wise. All Rights Reserved.

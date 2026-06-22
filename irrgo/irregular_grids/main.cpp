// Copyright Ben Paul Wise. All Rights Reserved.
//
// Command-line driver for the irregular-grid generator. Writes three standalone
// SVG documents -- grid.svg, disc.svg and board.svg -- to the current directory.
// All parameters come from the board_params / draw_params modules.
// This just does the setup.

#include <cstdlib>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

#include "board_params.h"
#include "draw_params.h"
#include "irregular_grid.h"

namespace {

void write_file(const std::string& path, const std::string& contents) {
    std::ofstream file(path, std::ios::binary);
    if (!file) {
        throw std::runtime_error("could not open output file: " + path);
    }
    file << contents;
    if (!file) {
        throw std::runtime_error("failed while writing: " + path);
    }
    std::cerr << "wrote " << path << '\n';
}

}  // namespace

int main() {
    using namespace games::board;

    try {
        // Two independent seeds. A non-zero value is reproducible; 0 means
        // "derive a fresh one from the clock" (see AbsGame::makeSeed).

        // 8x10 Board seed: 1600304105177925591  Render seed: 6210208043131634036
        // 8x10 Board seed: 1289915031459806163  Render seed: 5901601049887194067
        const std::uint64_t render_seed = AbsGame::makeSeed(603205);
        const std::uint64_t board_seed = AbsGame::makeSeed(144532);

        const BoardParams params;  // rows/columns/stone_fraction/roughness/smoothing
        RenderConfig config = default_render_config();
        config.seed = render_seed;
        const SvgStyle style = default_svg_style();

        // The plain grid and a standalone demo disc.
        write_file("grid.svg", generate_svg(to_grid_spec(params), config, style));
        write_file("disc.svg", generate_disc_svg(standalone_disc_spec(), config, style));

        // The populated board: grid + the two piece sets, with the X on every
        // immobilised disc.
        const PieceColors colors = piece_colors();
        const int per_side = stones_per_side(params);

        BoardSpec board;
        board.grid = to_grid_spec(params);
        board.seed = board_seed;
        board.pieces = {
            PieceSet{colors.side_a, per_side},
            PieceSet{colors.side_b, per_side},
        };
        apply_draw_defaults(board);  // disc, outline, outer box, X-mark

        write_file("board.svg", generate_board_svg(board, config, style));

        // An EXPLICIT position (what a game renders from its own state): pieces at
        // chosen squares, two of them immobilised (shown with the "X").
        const int cols = params.columns;
        const auto sq = [cols](int row, int col) { return row * cols + col; };
        const std::vector<PlacedPiece> position = {
            {sq(1, 1), colors.side_a, false},
            {sq(1, 2), colors.side_b, true},   // immobilised (flanked left/right)
            {sq(1, 3), colors.side_a, false},
            {sq(3, 4), colors.side_a, false},
            {sq(4, 4), colors.side_b, false},
            {sq(6, 7), colors.side_b, false},
            {sq(6, 8), colors.side_a, true},   // immobilised
            {sq(6, 9), colors.side_b, false},
        };
        write_file("position.svg", generate_position_svg(board, position, config, style));

        // Chess-like notation (column letter + row, 1 at bottom), labelled by side
        // (A = side_a, B = side_b). This is the static position.svg demo, NOT a
        // game move log -- the Latrunculi self-play log is latrunculi_selfplay.
        std::cerr << "position.svg pieces (static demo on " << params.rows << "x"
                  << params.columns << " board):\n";
        for (const PlacedPiece& piece : position) {
            const char side = (piece.fill == colors.side_a) ? 'A' : 'B';
            std::cerr << "  " << side << ": square " << piece.square << " = "
                      << square_to_notation(piece.square, params.rows, params.columns)
                      << (piece.immobilized ? " (X)" : "") << '\n';
        }
    } catch (const std::exception& error) {
        std::cerr << "error: " << error.what() << '\n';
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
// Copyright Ben Paul Wise. All Rights Reserved.

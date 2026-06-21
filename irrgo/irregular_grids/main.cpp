// Copyright Ben Paul Wise. All Rights Reserved.
//
// Command-line driver for the irregular-grid generator. Builds a hand-scratched
// grid and writes a standalone SVG document to stdout (or to a file given as the
// last argument). The library itself performs no I/O; this is the I/O boundary.
//
// Usage:
//   irregular_grid_demo [rows] [columns] [roughness] [smoothing] [out.svg]
//
// All arguments are optional and positional. Missing trailing arguments keep
// their defaults. roughness and smoothing must lie in [0,1]; rows and columns
// must be >= 1 (the library validates and throws otherwise).

#include "irregular_grid.hpp"

#include <cstdlib>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>

int main(int argc, char** argv) {
    // create a spec with default values which will be overwritten soon.
    games::board::GridSpec spec{8, 8, 20, 0.5};

    // rows and columns can be independently varied in the 6-12 range
    spec.rows = 8; // 6 - 12;
    spec.columns = 10; // 6 - 12;
    spec.stonesPerSide =  (int)(3.0*spec.rows*spec.columns/16.0);
    spec.roughness = 0.1;
    spec.smoothing = 0.95;
    std::string outPath = "grid.svg";

        const std::string svg = games::board::generate_svg(spec);

        if (outPath.empty()) {
            std::cout << svg;
        } else {
            std::ofstream file(outPath, std::ios::binary);
            if (!file) {
                throw std::runtime_error("could not open output file: " + outPath);
            }
            file << svg;
            if (!file) {
                throw std::runtime_error("failed while writing: " + outPath);
            }
            std::cerr << "wrote " << outPath << '\n';
        }

    // A slightly irregular filled disc, by the same noise-then-smooth idea.
    const games::board::DiscSpec disc{2.0, 0.25, 0.95, 360/4};
    const std::string disc_svg = games::board::generate_disc_svg(disc);

    {
        const std::string disc_path = "disc.svg";
        std::ofstream file(disc_path, std::ios::binary);
        if (!file) {
            throw std::runtime_error("could not open output file: " + disc_path);
        }
        file << disc_svg;
        if (!file) {
            throw std::runtime_error("failed while writing: " + disc_path);
        }
        std::cerr << "wrote " << disc_path << '\n';
    }

    // A populated board: the 8x8 grid above plus 20 pastel-green and 20 purplish
    // discs, each generated separately and dropped in the centre of a distinct
    // random square. 40 discs on 64 squares leaves 24 empty. Disc shape reuses
    // the preferred roughness/smoothing/point_count; radius 0.4 fits one per cell.
    games::board::BoardSpec board;
    board.grid = spec;
    board.disc = games::board::DiscSpec{0.4, 0.25, 0.95, 90};
    board.pieces = {
        games::board::PieceSet{"#2aa85a", spec.stonesPerSide},  // pastel green
        games::board::PieceSet{"#c76fe8", spec.stonesPerSide},  // purplish
    };
    board.outline = "#000000";          // thin black outline
    board.outline_width_units = 0.02;

    const std::string board_svg = games::board::generate_board_svg(board);
    {
        const std::string board_path = "board.svg";
        std::ofstream file(board_path, std::ios::binary);
        if (!file) {
            throw std::runtime_error("could not open output file: " + board_path);
        }
        file << board_svg;
        if (!file) {
            throw std::runtime_error("failed while writing: " + board_path);
        }
        std::cerr << "wrote " << board_path << '\n';
    }

    return EXIT_SUCCESS;
}
// Copyright Ben Paul Wise. All Rights Reserved.

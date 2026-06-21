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
    games::board::GridSpec spec{4, 4, 0.6, 0.5};

    spec.rows = 8;
    spec.columns = 8;
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

    return EXIT_SUCCESS;
}
// Copyright Ben Paul Wise. All Rights Reserved.

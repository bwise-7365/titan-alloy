#ifndef GAMES_BOARD_IRREGULAR_GRID_HPP
#define GAMES_BOARD_IRREGULAR_GRID_HPP

#include <cstdint>
#include <string>
#include <vector>

// Generates "hand-scratched" game-board grids.
//
// Model:
//   1. Perfect grid in square-width units (one square is 1 x 1): vertical lines
//      at x = 0..columns, horizontal lines at y = 0..rows.
//   2. Each line is sampled points_per_edge times per square it spans, plus one
//      closing sample. Each interior sample gets an independent perpendicular
//      deviation roughness * (U(0,1) - 0.5): up to +/-0.5 square widths at
//      roughness = 1.
//   3. Each line is relaxed independently toward the fixed point of
//          s(k) = (1 - smoothing) * n(k) + smoothing * (s(k-1) + s(k+1)) / 2
//      with both endpoints pinned to the exact perfect value. Gauss-Seidel
//      sweeps run until the largest |change| in a sweep falls below 1/1000 of a
//      square width.
//
//   smoothing = 0 reproduces the raw noise; smoothing = 1 collapses each line to
//   a straight segment regardless of roughness; roughness = 0 is always straight.
//
// All geometry is returned in square-width units (one square == 1.0). Pixels are
// introduced only by to_svg via RenderConfig::square_size. The library performs
// no I/O and keeps no global state.

namespace games::board {

struct Point {
    double x;
    double y;
};

struct Polyline {
    std::vector<Point> points;  // in square-width units
    std::string label;          // e.g. "vertical-3", "horizontal-0"
};

// The four conceptual parameters. roughness and smoothing must lie in [0,1];
// rows and columns must be >= 1. Out-of-range values throw std::invalid_argument.
struct GridSpec {
    int rows;
    int columns;
    double roughness;
    double smoothing;
};

// Resolution and the pixel scale used only at SVG-render time.
struct RenderConfig {
    double square_size = 80.0;     // pixels per square (SVG output only)
    int points_per_edge = 10;     // samples per square along each line
    std::uint64_t seed = 1;        // deterministic across platforms
};

// Cosmetic SVG settings. An empty background string omits the background rect.
struct SvgStyle {
    std::string background = "#ffffee"; //"#f4ecd8";
    std::string ink = "#000060"; //"#3a2f1a";
    double stroke_width_units = 0.0375;  // square-width units
    double margin_units = 0.5;         // square-width units
};

struct GridGeometry {
    std::vector<Polyline> lines;
    double width_units = 0.0;
    double height_units = 0.0;
};

// Builds the relaxed line geometry (square-width units).
// Throws std::invalid_argument on bad parameters, std::runtime_error if the
// relaxation fails to converge within its internal sweep cap.
GridGeometry build_grid(const GridSpec& spec, const RenderConfig& config);

// Serialises geometry to a standalone SVG document string.
std::string to_svg(const GridGeometry& geometry,
                   const RenderConfig& config,
                   const SvgStyle& style);

// Convenience: build_grid followed by to_svg.
std::string generate_svg(const GridSpec& spec,
                         const RenderConfig& config = {},
                         const SvgStyle& style = {});

}  // namespace games::board

#endif  // GAMES_BOARD_IRREGULAR_GRID_HPP

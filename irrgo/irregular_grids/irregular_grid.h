// Copyright Ben Paul Wise. All Rights Reserved.

#ifndef GAMES_BOARD_IRREGULAR_GRID_HPP
#define GAMES_BOARD_IRREGULAR_GRID_HPP

#include <cstdint>
#include <stdexcept>
#include <string>
#include <vector>

#include "AbsGame.h"  // AbsGame::makeSeed for the default RNG seed
#include "utils.h"
#include "draw_params.h"  // RenderConfig, SvgStyle and the drawing constants

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
// Also generates "hand-scratched" filled discs by the same idea applied to a
// closed loop instead of an open line:
//   1. A perfect circle of the given radius, sampled at point_count evenly
//      spaced angles.
//   2. Each sample's radius is perturbed by roughness * radius * (U(0,1) - 0.5):
//      up to +/-0.5 * radius at roughness = 1. Unlike a line, every sample is
//      perturbed (a loop has no endpoints to pin).
//   3. The radii are relaxed toward the same fixed point, but with periodic
//      neighbours, so the loop stays closed. smoothing = 1 collapses the loop to
//      a perfect circle of the mean radius; roughness = 0 is always a circle.
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

// Throws std::invalid_argument(name + " must be in [0,1]") unless value is in
// [0,1]. Shared by the renderer's and board_params' parameter validation.
inline void requireUnit(double value, const char* name) {
    if (!(value >= 0.0 && value <= 1.0)) {
        throw std::invalid_argument(std::string(name) + " must be in [0,1]");
    }
}

// The renderer's view of a grid. rows and columns must be >= 1; roughness and
// smoothing must lie in [0,1]. Out-of-range values throw std::invalid_argument.
// Game-level size limits live in board_params.h, not here.
struct GridSpec {
    int rows = 8;
    int columns = 8;
    double roughness = 0.1;
    double smoothing = 0.95;
};

struct GridGeometry {
    std::vector<Polyline> lines;
    double width_units = 0.0;
    double height_units = 0.0;
};

// The conceptual parameters for an irregular disc. radius must be > 0;
// point_count must be >= 3; roughness and smoothing must lie in [0,1].
// Out-of-range values throw std::invalid_argument.
struct DiscSpec {
    double radius;       // perfect radius, square-width units
    double roughness;    // noise magnitude as a fraction of the radius
    double smoothing;    // relaxation strength toward a perfect circle
    int point_count;     // evenly spaced samples around the loop
};

struct DiscGeometry {
    Polyline boundary;        // closed loop of boundary points, square-width units
    Point center{0.0, 0.0};   // in square-width units
    double radius_units = 0.0;
    double width_units = 0.0;  // bounding box, for SVG sizing
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

// Builds the relaxed disc boundary (square-width units). config.points_per_edge
// is unused here; resolution comes from DiscSpec::point_count.
// Throws std::invalid_argument on bad parameters, std::runtime_error if the
// relaxation fails to converge within its internal sweep cap.
DiscGeometry build_disc(const DiscSpec& spec, const RenderConfig& config);

// Serialises a disc to a standalone SVG document as a single filled polygon.
std::string to_svg(const DiscGeometry& geometry,
                   const RenderConfig& config,
                   const SvgStyle& style);

// Convenience: build_disc followed by to_svg.
std::string generate_disc_svg(const DiscSpec& spec,
                              const RenderConfig& config = {},
                              const SvgStyle& style = {});

// One color of piece and how many to place on the board.
struct PieceSet {
    std::string fill;   // disc fill color, e.g. "#2aa85a"
    int count;          // number of discs of this color (>= 0)
};

// A populated board: the grid, the disc shape shared by every piece, and the
// colored piece sets to scatter across the squares. Each disc is generated
// separately (independent noise) and placed in the centre of a distinct,
// randomly chosen square; no square receives two discs. Every disc carries a
// thin outline. To fit one disc per 1x1 cell, disc.radius must be <= 0.5.
struct BoardSpec {
    GridSpec grid;
    DiscSpec disc;                    // radius/roughness/smoothing/point_count, shared
    std::vector<PieceSet> pieces;
    // The board-configuration seed: fixes the random piece placement, independent
    // of RenderConfig::seed (which drives the hand-scratched line/disc/X noise).
    std::uint64_t seed = AbsGame::makeSeed(2654435761l);
    std::string outline = kBlackInk;  // outline drawn on every disc
    double outline_width_units = kOutlineWidthUnits;
    // The outer black box framing the whole diagram (board + labels): its inset
    // in square-widths from the canvas edge. Must be in [0, margin_units); the
    // gap to the inner border is then margin_units - outer_margin_units, equal on
    // all four sides.
    double outer_margin_units = kOuterMarginUnits;
    // The "X" stamped on every immobilised disc.
    std::string mark_color = kBlackInk;
    double mark_length_fraction = kMarkLengthFraction;       // fraction of disc diameter
    double mark_stroke_width_units = kMarkStrokeWidthUnits;  // X line weight, square-width units
};

// Builds the grid plus all requested discs in one composite SVG document. Two
// independent seeds drive it deterministically across platforms: board.seed
// fixes the piece placement, config.seed fixes the hand-scratched line/disc/X
// noise.
// Throws std::invalid_argument if the pieces cannot fit (more discs than
// squares) or disc.radius > 0.5; propagates the build/relax exceptions.
std::string generate_board_svg(const BoardSpec& board,
                               const RenderConfig& config = {},
                               const SvgStyle& style = {});

// One disc placed at an explicit square (square = row * columns + column),
// with its own fill colour and an optional immobilised "X" marker.
struct PlacedPiece {
    int square;
    std::string fill;
    bool immobilized = false;
};

// Renders an EXPLICIT position: the grid/frame/labels from `board`, plus a disc
// at each PlacedPiece's square (Pieces layer) and an "X" on each immobilised one
// (Markers layer). Unlike generate_board_svg, placement is caller-specified, so
// this is what a game uses to draw its current state. board.pieces and
// board.seed are ignored; board.grid/disc/outline/outer_margin/mark_* supply the
// look. Each disc's noise is seeded by its square index, so a square's wobble is
// stable across renders as pieces move.
// Throws std::invalid_argument on bad parameters or an out-of-range square.
std::string generate_position_svg(const BoardSpec& board,
                                  const std::vector<PlacedPiece>& pieces,
                                  const RenderConfig& config = {},
                                  const SvgStyle& style = {});

// Chess-like coordinate notation matching the SVG edge labels (for move logs):
// the column letter is 'A' + column (left to right) and the row number counts
// with 1 at the BOTTOM (row 0 is the top). On an 8-row board, the square at
// (row 1, col 2) is "C7" and (row 6, col 8) is "I2". `square = row*columns + col`.
// Requires 1 <= columns <= 26. Throws std::invalid_argument on bad input.
std::string square_to_notation(int square, int rows, int columns);
int notation_to_square(const std::string& notation, int rows, int columns);

}  // namespace games::board

#endif  // GAMES_BOARD_IRREGULAR_GRID_HPP
// Copyright Ben Paul Wise. All Rights Reserved.
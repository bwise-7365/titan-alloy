// Copyright Ben Paul Wise. All Rights Reserved.

#include "irregular_grid.h"

#include <cmath>
#include <cstddef>
#include <iomanip>
#include <locale>
#include <ostream>
#include <random>
#include <sstream>
#include <stdexcept>
#include <string_view>
#include <utility>

#include <cstdio>  // printf seed diagnostic

namespace games::board {

namespace {

constexpr double kTolerance = 1E-4;  // fraction of a square width
constexpr int kMaxSweeps = 250;
constexpr double kPi = 3.14159265358979323846;  // std::numbers::pi needs C++20 <numbers>
constexpr double kTwoPow53 = 9007199254740992.0;  // 2^53, the double mantissa span
constexpr double kDiagonalAngleRad = kPi / 4.0;   // the X arms run at +/-45 degrees

void validate(const GridSpec& spec) {
    if (spec.rows < 1) {
        throw std::invalid_argument("rows must be >= 1");
    }
    if (spec.columns < 1) {
        throw std::invalid_argument("columns must be >= 1");
    }
    requireUnit(spec.roughness, "roughness");
    requireUnit(spec.smoothing, "smoothing");
}

void validate(const RenderConfig& config) {
    if (!(config.square_size > 0.0)) {
        throw std::invalid_argument("square_size must be > 0");
    }
    if (config.points_per_edge < 1) {
        throw std::invalid_argument("points_per_edge must be >= 1");
    }
}

void validate(const DiscSpec& spec) {
    if (!(spec.radius > 0.0)) {
        throw std::invalid_argument("radius must be > 0");
    }
    if (spec.point_count < 3) {
        throw std::invalid_argument("point_count must be >= 3");
    }
    requireUnit(spec.roughness, "roughness");
    requireUnit(spec.smoothing, "smoothing");
}

// 53-bit mantissa uniform in [0,1). Built directly from the standardised
// mt19937_64 engine so the sequence is identical on every platform, unlike
// std::uniform_real_distribution, whose algorithm is implementation-defined.
double canonical_uniform(std::mt19937_64& engine) {
    const std::uint64_t bits = engine() >> 11;  // top 53 bits
    return static_cast<double>(bits) * (1.0 / kTwoPow53);
}

// Noisy perpendicular coordinate for one line. Endpoints carry the exact perfect
// value (they are never updated); interior samples are perturbed.
std::vector<double> make_noise(double perfect_value, int sample_count,
                               double roughness, std::mt19937_64& engine) {
    if (sample_count < 2) {
        throw std::invalid_argument("a line needs at least 2 samples");
    }
    std::vector<double> noise(static_cast<std::size_t>(sample_count), perfect_value);
    for (int k = 1; k < sample_count - 1; ++k) {
        const double deviation = roughness * (canonical_uniform(engine) - 0.5);
        noise[static_cast<std::size_t>(k)] = perfect_value + deviation;
    }
    return noise;
}

// Noisy radius for each evenly spaced sample around a loop. Unlike a line, the
// loop has no endpoints to pin, so every sample is perturbed. The deviation
// magnitude scales with the radius: +/-0.5 * radius at roughness = 1.
std::vector<double> make_radial_noise(double radius, int point_count,
                                      double roughness, std::mt19937_64& engine) {
    if (point_count < 3) {
        throw std::invalid_argument("a disc needs at least 3 samples");
    }
    std::vector<double> noise(static_cast<std::size_t>(point_count));
    for (int k = 0; k < point_count; ++k) {
        const double deviation = roughness * radius * (canonical_uniform(engine) - 0.5);
        noise[static_cast<std::size_t>(k)] = radius + deviation;
    }
    return noise;
}

// Relaxes one sequence of samples to the fixed point
//     s(k) = (1 - smoothing) * noise(k) + smoothing * (s(left) + s(right)) / 2
// by Gauss-Seidel sweeps, and returns a fresh vector. `closed` selects the
// topology: an open line keeps its two endpoints pinned at their noise value
// (so a line's ends stay on the perfect grid); a closed loop relaxes every
// sample and wraps its neighbours (so a disc boundary stays a loop). Throws
// rather than returning a half-relaxed result if it fails to converge.
std::vector<double> relax(const std::vector<double>& noise, double smoothing,
                          bool closed) {
    const std::size_t n = noise.size();
    std::vector<double> s = noise;  // open-line endpoints stay pinned at noise

    const std::size_t first = closed ? 0 : 1;
    const std::size_t last = closed ? n : n - 1;  // exclusive upper bound

    for (int sweep = 0; sweep < kMaxSweeps; ++sweep) {
        double max_change = 0.0;
        for (std::size_t k = first; k < last; ++k) {
            const std::size_t left = (k == 0) ? n - 1 : k - 1;
            const std::size_t right = (k + 1 == n) ? 0 : k + 1;
            const double neighbour_average = 0.5 * (s[left] + s[right]);
            const double updated = (1.0 - smoothing) * noise[k] + smoothing * neighbour_average;
            const double change = std::fabs(updated - s[k]);
            if (change > max_change) {
                max_change = change;
            }
            s[k] = updated;
        }
        if (max_change < kTolerance) {
            return s;
        }
    }
    throw std::runtime_error("smoothing did not converge within max_sweeps; "
                             "raise the sweep cap or the tolerance");
}

// Evenly spaced coordinates 0, 1/ppe, 2/ppe, ... along the line's own direction.
std::vector<double> along_axis(int sample_count, int points_per_edge) {
    std::vector<double> axis(static_cast<std::size_t>(sample_count));
    for (int k = 0; k < sample_count; ++k) {
        axis[static_cast<std::size_t>(k)] =
            static_cast<double>(k) / static_cast<double>(points_per_edge);
    }
    return axis;
}

Polyline zip_polyline(const std::vector<double>& xs, const std::vector<double>& ys,
                      std::string label) {
    if (xs.size() != ys.size()) {
        throw std::invalid_argument("xs and ys must have equal length");
    }
    Polyline line;
    line.label = std::move(label);
    line.points.reserve(xs.size());
    for (std::size_t k = 0; k < xs.size(); ++k) {
        line.points.push_back(Point{xs[k], ys[k]});
    }
    return line;
}

// Writes an SVG points list "x0,y0 x1,y1 ..." in pixels, applying the shared
// units-to-pixels transform. The optional (tx, ty) translation (square-width
// units) is added before scaling, so a template/origin-centred shape can be
// stamped at a board position without first copying and translating its points.
void emit_points(std::ostream& os, const std::vector<Point>& points,
                 double scale, double margin, double tx = 0.0, double ty = 0.0) {
    bool first = true;
    for (const Point& p : points) {
        if (!first) {
            os << ' ';
        }
        first = false;
        os << ((p.x + tx) * scale + margin) << ',' << ((p.y + ty) * scale + margin);
    }
}

// Opens an SVG document's root <svg> tag. When with_layers is true, also declares
// the Inkscape namespace so child <g inkscape:groupmode="layer"> elements are
// valid. Sets fixed/precision(2) (the default for coordinates) on the stream.
void emit_svg_open(std::ostream& os, double width, double height,
                   bool with_layers = false) {
    os << std::fixed << std::setprecision(2);
    os << "<svg xmlns=\"http://www.w3.org/2000/svg\"";
    if (with_layers) {
        os << " xmlns:inkscape=\"http://www.inkscape.org/namespaces/inkscape\"";
    }
    os << " width=\"" << width << "\" height=\"" << height
       << "\" viewBox=\"0 0 " << width << ' ' << height << "\">\n";
}

// Emits the full-canvas background <rect> (skipped when fill is empty).
void emit_background_rect(std::ostream& os, double width, double height,
                          const std::string& fill) {
    if (fill.empty()) {
        return;
    }
    os << "  <rect x=\"0\" y=\"0\" width=\"" << width << "\" height=\"" << height
       << "\" fill=\"" << fill << "\"/>\n";
}

// Opens an Inkscape layer group (a <g> the editor treats as a named layer).
// Close with "  </g>\n".
void emit_open_layer(std::ostream& os, const char* label, const char* id) {
    os << "  <g inkscape:groupmode=\"layer\" inkscape:label=\"" << label
       << "\" id=\"" << id << "\">\n";
}

// Opens an unfilled, stroked <g> with round caps/joins. The stroke width prints
// at precision 3; the stream is left at precision 2 for the group's contents.
void emit_open_stroke_group(std::ostream& os, std::string_view stroke,
                            double stroke_width) {
    os << std::setprecision(3);
    os << "  <g fill=\"none\" stroke=\"" << stroke << "\" stroke-width=\"" << stroke_width
       << "\" stroke-linecap=\"round\" stroke-linejoin=\"round\">\n";
    os << std::setprecision(2);
}

// Opens a filled <polygon> tag up to (but not including) its points list. The
// stroke width prints at precision 3; the stream is left at precision 2 ready
// for emit_points, which the caller follows with "\"/>\n".
void emit_filled_polygon_open(std::ostream& os, std::string_view fill,
                              std::string_view stroke, double stroke_width) {
    os << std::setprecision(3);
    os << "  <polygon fill=\"" << fill << "\" stroke=\"" << stroke
       << "\" stroke-width=\"" << stroke_width << "\" stroke-linejoin=\"round\"\n";
    os << std::setprecision(2);
    os << "    points=\"";
}

// Emits an unfilled, stroked <rect> at the stream's current precision.
void emit_rect(std::ostream& os, double x, double y, double w, double h,
               std::string_view stroke, double stroke_width) {
    os << "  <rect x=\"" << x << "\" y=\"" << y << "\" width=\"" << w
       << "\" height=\"" << h << "\" fill=\"none\" stroke=\"" << stroke
       << "\" stroke-width=\"" << stroke_width << "\"/>\n";
}

// Writes the board coordinate labels: capital letters A, B, C, ... centred
// above and below each column (A on the left), and numbers 1, 2, 3, ... centred
// left and right of each row (1 on the bottom). The text is perfectly straight
// and lives in the margins, independent of the hand-scratched grid lines. A
// serif face is used so capital I keeps its top/bottom bars and stays distinct
// from L (and from the digit 1). `offset` is the units-to-pixels translation
// already applied to the board.
void emit_edge_labels(std::ostream& os, int rows, int columns, double scale,
                      double offset, double label_gap_units, double label_font_units) {
    const double left_x = offset;
    const double right_x = offset + static_cast<double>(columns) * scale;
    const double top_y = offset;
    const double bottom_y = offset + static_cast<double>(rows) * scale;
    const double gap = label_gap_units * scale;    // board edge -> label centre
    const double font = label_font_units * scale;  // label height

    os << "  <g font-family=\"serif\" font-size=\"" << font
       << "\" fill=\"" << kBlackInk << "\" text-anchor=\"middle\" dominant-baseline=\"central\">\n";

    for (int c = 0; c < columns; ++c) {
        const double x = offset + (static_cast<double>(c) + 0.5) * scale;
        const std::string letter(1, static_cast<char>('A' + c));
        os << "    <text x=\"" << x << "\" y=\"" << (top_y - gap) << "\">" << letter << "</text>\n";
        os << "    <text x=\"" << x << "\" y=\"" << (bottom_y + gap) << "\">" << letter << "</text>\n";
    }
    for (int r = 0; r < rows; ++r) {
        const double y = offset + (static_cast<double>(r) + 0.5) * scale;
        const std::string number = std::to_string(rows - r);  // 1 at the bottom
        os << "    <text x=\"" << (left_x - gap) << "\" y=\"" << y << "\">" << number << "</text>\n";
        os << "    <text x=\"" << (right_x + gap) << "\" y=\"" << y << "\">" << number << "</text>\n";
    }

    os << "  </g>\n";
}

// Uniform integer in [0, bound) drawn from the engine by rejection sampling, so
// the result is identical on every platform (std::uniform_int_distribution is
// implementation-defined, like its real-valued cousin).
std::uint64_t bounded_uint(std::mt19937_64& engine, std::uint64_t bound) {
    if (bound == 0) {
        throw std::invalid_argument("bound must be > 0");
    }
    const std::uint64_t reject = (-bound) % bound;  // == 2^64 mod bound
    std::uint64_t draw = engine();
    while (draw < reject) {
        draw = engine();
    }
    return draw % bound;
}

// Picks `pick` distinct square indices out of [0, square_count) via a partial
// Fisher-Yates shuffle. Throws rather than silently clamping when too many are
// requested.
std::vector<int> choose_squares(int square_count, int pick, std::mt19937_64& engine) {
    if (pick < 0 || pick > square_count) {
        throw std::invalid_argument("cannot place more discs than there are squares");
    }
    std::vector<int> index(static_cast<std::size_t>(square_count));
    for (int i = 0; i < square_count; ++i) {
        index[static_cast<std::size_t>(i)] = i;
    }
    for (int i = 0; i < pick; ++i) {
        const std::uint64_t span = static_cast<std::uint64_t>(square_count - i);
        const std::size_t j = static_cast<std::size_t>(i) +
                              static_cast<std::size_t>(bounded_uint(engine, span));
        std::swap(index[static_cast<std::size_t>(i)], index[j]);
    }
    index.resize(static_cast<std::size_t>(pick));
    return index;
}

// Builds one disc boundary from an explicit engine, so several discs drawn from
// the same engine each get their own independent noise. Geometry is centred on
// the origin; callers translate it to a board square.
DiscGeometry build_disc_core(const DiscSpec& spec, std::mt19937_64& engine) {
    const std::vector<double> noise =
        make_radial_noise(spec.radius, spec.point_count, spec.roughness, engine);
    const std::vector<double> radii = relax(noise, spec.smoothing, /*closed=*/true);

    DiscGeometry geometry;
    geometry.radius_units = spec.radius;
    geometry.center = Point{0.0, 0.0};
    geometry.width_units = 2.0 * spec.radius;
    geometry.height_units = 2.0 * spec.radius;

    geometry.boundary.label = "disc";
    geometry.boundary.points.reserve(radii.size());
    for (int k = 0; k < spec.point_count; ++k) {
        const double theta = 2.0 * kPi * static_cast<double>(k) /
                             static_cast<double>(spec.point_count);
        const double r = radii[static_cast<std::size_t>(k)];
        geometry.boundary.points.push_back(
            Point{geometry.center.x + r * std::cos(theta),
                  geometry.center.y + r * std::sin(theta)});
    }
    return geometry;
}

// Builds the "X" template: two crossed irregular lines at +/-45 degrees, each of
// the given length and centred on the origin. Each line is drawn with the same
// algorithm as the grid edges -- a straight local axis with perpendicular noise,
// relaxed -- then rotated into place. Same roughness/smoothing and the same
// per-square sample density as the edges. Square-width units.
std::vector<Polyline> build_x_template(double length, double roughness, double smoothing,
                                       int points_per_edge, std::mt19937_64& engine) {
    int samples = static_cast<int>(std::lround(length * static_cast<double>(points_per_edge))) + 1;
    if (samples < 2) {
        samples = 2;
    }
    const double half = 0.5 * length;
    const double angle[2] = {kDiagonalAngleRad, -kDiagonalAngleRad};  // the two crossed diagonals

    std::vector<Polyline> lines;
    lines.reserve(2);
    for (int a = 0; a < 2; ++a) {
        // Perpendicular deviation about a straight line (perfect value 0), exactly
        // as an edge is built, with both ends pinned.
        const std::vector<double> noise = make_noise(0.0, samples,  roughness, engine);
        const std::vector<double> perp = relax(noise, 0.8 * smoothing, /*closed=*/false);
        const double ct = std::cos(angle[a]);
        const double st = std::sin(angle[a]);

        Polyline line;
        line.label = "x-" + std::to_string(a);
        line.points.reserve(static_cast<std::size_t>(samples));
        for (int k = 0; k < samples; ++k) {
            const double u = -half + length * static_cast<double>(k) /
                                              static_cast<double>(samples - 1);
            const double v = perp[static_cast<std::size_t>(k)];
            // Rotate the local (u, v) by the diagonal angle; the centre is the origin.
            line.points.push_back(Point{u * ct - v * st, u * st + v * ct});
        }
        lines.push_back(std::move(line));
    }
    return lines;
}

// The color index occupying square (column, row), or -1 if empty or off-board.
int occupant_at(const std::vector<int>& occupant, int columns, int rows,
                int column, int row) {
    if (column < 0 || column >= columns || row < 0 || row >= rows) {
        return -1;
    }
    return occupant[static_cast<std::size_t>(row * columns + column)];
}

// True if (column, row) holds an enemy of `me` that can still pin it: occupied,
// a different color, AND not itself already marked immobilised. A disc that has
// already been marked "X" no longer immobilises its neighbours.
bool active_enemy(const std::vector<int>& occupant, const std::vector<bool>& marked,
                  int columns, int rows, int column, int row, int me) {
    const int o = occupant_at(occupant, columns, rows, column, row);
    if (o == -1 || o == me) {
        return false;
    }
    return !marked[static_cast<std::size_t>(row * columns + column)];
}

// Latrunculi immobilisation of the piece of color `me` on square (column, row):
//   - bracketed: squeezed between two enemy pieces on opposite sides, either
//     left/right or top/bottom; or
//   - corner: the piece is at a board corner and its two in-board orthogonal
//     neighbours (the "L" at the angle) are both enemies.
// Each pair is judged independently, and a pinning piece counts only if it is an
// *active* enemy (see active_enemy): an already-marked disc does not pin. So a
// piece can be free of the L/R pair yet still immobilised by the T/B pair.
bool is_immobilized(const std::vector<int>& occupant, const std::vector<bool>& marked,
                    int columns, int rows, int column, int row, int me) {
    const bool left_enemy = active_enemy(occupant, marked, columns, rows, column - 1, row, me);
    const bool right_enemy = active_enemy(occupant, marked, columns, rows, column + 1, row, me);
    const bool top_enemy = active_enemy(occupant, marked, columns, rows, column, row - 1, me);
    const bool bottom_enemy = active_enemy(occupant, marked, columns, rows, column, row + 1, me);

    if ((left_enemy && right_enemy) || (top_enemy && bottom_enemy)) {
        return true;
    }

    const bool on_left_or_right_edge = (column == 0 || column == columns - 1);
    const bool on_top_or_bottom_edge = (row == 0 || row == rows - 1);
    if (on_left_or_right_edge && on_top_or_bottom_edge) {
        const bool horizontal_enemy = (column == 0) ? right_enemy : left_enemy;
        const bool vertical_enemy = (row == 0) ? bottom_enemy : top_enemy;
        if (horizontal_enemy && vertical_enemy) {
            return true;
        }
    }

    return false;
}

}  // namespace

GridGeometry build_grid(const GridSpec& spec, const RenderConfig& config) {
    validate(spec);
    validate(config);

    std::mt19937_64 engine(config.seed);

    GridGeometry geometry;
    geometry.width_units = static_cast<double>(spec.columns);
    geometry.height_units = static_cast<double>(spec.rows);

    const int vertical_samples = spec.rows * config.points_per_edge + 1;
    for (int i = 0; i <= spec.columns; ++i) {
        const double perfect_x = static_cast<double>(i);
        const std::vector<double> noise =
            make_noise(perfect_x, vertical_samples, spec.roughness, engine);
        const std::vector<double> xs = relax(noise, spec.smoothing, /*closed=*/false);
        const std::vector<double> ys = along_axis(vertical_samples, config.points_per_edge);
        geometry.lines.push_back(zip_polyline(xs, ys, "vertical-" + std::to_string(i)));
    }

    const int horizontal_samples = spec.columns * config.points_per_edge + 1;
    for (int j = 0; j <= spec.rows; ++j) {
        const double perfect_y = static_cast<double>(j);
        const std::vector<double> noise =
            make_noise(perfect_y, horizontal_samples, spec.roughness, engine);
        const std::vector<double> ys = relax(noise, spec.smoothing, /*closed=*/false);
        const std::vector<double> xs = along_axis(horizontal_samples, config.points_per_edge);
        geometry.lines.push_back(zip_polyline(xs, ys, "horizontal-" + std::to_string(j)));
    }

    return geometry;
}

std::string to_svg(const GridGeometry& geometry, const RenderConfig& config,
                   const SvgStyle& style) {
    validate(config);

    const double scale = config.square_size;
    const double margin = style.margin_units * scale;
    const double width = geometry.width_units * scale + 2.0 * margin;
    const double height = geometry.height_units * scale + 2.0 * margin;
    const double stroke = style.stroke_width_units * scale;

    std::ostringstream os;
    os.imbue(std::locale::classic());  // force '.' decimal separator on every platform
    emit_svg_open(os, width, height);
    emit_background_rect(os, width, height, style.background);
    emit_open_stroke_group(os, style.ink, stroke);

    for (const Polyline& line : geometry.lines) {
        os << "    <!-- " << line.label << " -->\n";
        os << "    <polyline points=\"";
        emit_points(os, line.points, scale, margin);
        os << "\"/>\n";
    }

    os << "  </g>\n";
    os << "</svg>\n";
    return os.str();
}

std::string generate_svg(const GridSpec& spec, const RenderConfig& config,
                         const SvgStyle& style) {
    const GridGeometry geometry = build_grid(spec, config);
    return to_svg(geometry, config, style);
}

DiscGeometry build_disc(const DiscSpec& spec, const RenderConfig& config) {
    validate(spec);
    validate(config);

    std::mt19937_64 engine(config.seed);
    DiscGeometry geometry = build_disc_core(spec, engine);

    // Shift the origin-centred core so the disc sits tangent to the box corner,
    // giving a self-contained standalone document with non-negative coordinates.
    const Point shift{spec.radius, spec.radius};
    geometry.center = shift;
    for (Point& p : geometry.boundary.points) {
        p.x += shift.x;
        p.y += shift.y;
    }
    return geometry;
}

std::string to_svg(const DiscGeometry& geometry, const RenderConfig& config,
                   const SvgStyle& style) {
    validate(config);

    const double scale = config.square_size;
    const double margin = style.margin_units * scale;
    const double width = geometry.width_units * scale + 2.0 * margin;
    const double height = geometry.height_units * scale + 2.0 * margin;
    const double stroke = style.stroke_width_units * scale;

    std::ostringstream os;
    os.imbue(std::locale::classic());  // force '.' decimal separator on every platform
    emit_svg_open(os, width, height);
    emit_background_rect(os, width, height, style.background);
    emit_filled_polygon_open(os, style.ink, style.ink, stroke);
    emit_points(os, geometry.boundary.points, scale, margin);
    os << "\"/>\n";
    os << "</svg>\n";
    return os.str();
}

std::string generate_disc_svg(const DiscSpec& spec, const RenderConfig& config,
                              const SvgStyle& style) {
    const DiscGeometry geometry = build_disc(spec, config);
    return to_svg(geometry, config, style);
}

std::string generate_board_svg(const BoardSpec& board, const RenderConfig& config,
                               const SvgStyle& style) {
    validate(board.disc);
    validate(config);

    if (!(board.disc.radius <= 0.5)) {
        throw std::invalid_argument("disc.radius must be <= 0.5 to fit one disc per square");
    }
    if (board.grid.columns > 26) {
        throw std::invalid_argument("column labels run A..Z; columns must be <= 26");
    }
    if (!(board.outer_margin_units >= 0.0 && board.outer_margin_units < style.margin_units)) {
        throw std::invalid_argument("outer_margin_units must be in [0, margin_units): "
                                    "the outer box sits between the canvas edge and the inner border");
    }

    int requested = 0;
    for (const PieceSet& set : board.pieces) {
        if (set.count < 0) {
            throw std::invalid_argument("piece count must be >= 0");
        }
        requested += set.count;
    }

    // build_grid validates the grid spec and produces the line geometry.
    const GridGeometry grid = build_grid(board.grid, config);
    const int square_count = board.grid.rows * board.grid.columns;
    if (requested > square_count) {
        throw std::invalid_argument("more discs requested than there are squares");
    }

    // Two independent engines: the board engine (board.seed) fixes the layout --
    // which squares are occupied by which color -- while the render engine
    // (config.seed) drives all the hand-scratched noise (disc shapes, the X). The
    // two seeds can be varied independently, both from main.cpp.
    printf("Render seed: %llu, Board seed: %llu\n", // same order as main.cpp
           static_cast<unsigned long long>(config.seed),
           static_cast<unsigned long long>(board.seed));
    std::mt19937_64 board_engine(board.seed);
    std::mt19937_64 render_engine(config.seed);
    const std::vector<int> squares = choose_squares(square_count, requested, board_engine);

    const double scale = config.square_size;
    const double margin = style.margin_units * scale;
    // One margin on every side, so the gutter -- and hence the gap between the
    // inner and outer boxes -- is equal all around.
    const double offset = margin;
    const double width = grid.width_units * scale + 2.0 * margin;
    const double height = grid.height_units * scale + 2.0 * margin;
    const double grid_stroke = style.stroke_width_units * scale;
    const double outline_stroke = board.outline_width_units * scale;

    std::ostringstream os;
    os.imbue(std::locale::classic());  // force '.' decimal separator on every platform
    emit_svg_open(os, width, height, /*with_layers=*/true);

    // ── Background layer (bottom): background, grid, frame, labels ─────────────
    emit_open_layer(os, "Background", "layer-background");
    emit_background_rect(os, width, height, style.background);
    emit_open_stroke_group(os, style.ink, grid_stroke);
    for (const Polyline& line : grid.lines) {
        os << "    <polyline points=\"";
        emit_points(os, line.points, scale, offset);
        os << "\"/>\n";
    }
    os << "  </g>\n";

    // A crisp, perfectly straight black edge framing the grid extent (over the
    // hand-scratched outer lines).
    emit_rect(os, offset, offset, grid.width_units * scale, grid.height_units * scale,
              kBlackInk, grid_stroke);

    // A second, outer black box framing the whole diagram (board plus the edge
    // labels), inset from the canvas border by the configurable outer margin.
    const double box_inset = board.outer_margin_units * scale;
    emit_rect(os, box_inset, box_inset, width - 2.0 * box_inset, height - 2.0 * box_inset,
              kBlackInk, grid_stroke);

    emit_edge_labels(os, board.grid.rows, board.grid.columns, scale, offset,
                     style.label_gap_units, style.label_font_units);
    os << "  </g>\n";  // close Background layer

    // ── Pieces layer (middle): one disc per chosen square ─────────────────────
    // Each disc is generated separately (its own noise) and placed at the centre
    // of its randomly chosen square. squares[] holds the color assignment order:
    // the first set's discs take the first slots, the next set the following, etc.
    emit_open_layer(os, "Pieces", "layer-pieces");
    std::vector<Point> disc_centers;
    std::vector<int> disc_square;
    std::vector<int> disc_color;
    disc_centers.reserve(static_cast<std::size_t>(requested));
    disc_square.reserve(static_cast<std::size_t>(requested));
    disc_color.reserve(static_cast<std::size_t>(requested));
    // Board occupancy by color index (-1 == empty), for the immobilisation test.
    std::vector<int> occupant(static_cast<std::size_t>(square_count), -1);

    std::size_t slot = 0;
    int color_index = 0;
    for (const PieceSet& set : board.pieces) {
        for (int n = 0; n < set.count; ++n) {
            const int square = squares[slot];
            ++slot;
            const int column = square % board.grid.columns;
            const int row = square / board.grid.columns;
            const Point center{static_cast<double>(column) + 0.5,
                               static_cast<double>(row) + 0.5};
            disc_centers.push_back(center);
            disc_square.push_back(square);
            disc_color.push_back(color_index);
            occupant[static_cast<std::size_t>(square)] = color_index;

            // Origin-centred disc; emit_points stamps it at the square centre.
            const DiscGeometry disc = build_disc_core(board.disc, render_engine);
            emit_filled_polygon_open(os, set.fill, board.outline, outline_stroke);
            emit_points(os, disc.boundary.points, scale, offset, center.x, center.y);
            os << "\"/>\n";
        }
        ++color_index;
    }
    os << "  </g>\n";  // close Pieces layer

    // ── Markers layer (top): the crossed "X" on every immobilised piece ───────
    // Mark every immobilised piece (Latrunculi). The scan is a single pass in
    // disc-placement order, and a disc marked earlier in the pass no longer pins
    // later discs (see active_enemy): order-dependent by design, reproducible
    // from the seed.
    std::vector<Point> marked_centers;
    std::vector<bool> marked_square(static_cast<std::size_t>(square_count), false);
    for (std::size_t i = 0; i < disc_centers.size(); ++i) {
        const int square = disc_square[i];
        const int column = square % board.grid.columns;
        const int row = square / board.grid.columns;
        if (is_immobilized(occupant, marked_square, board.grid.columns, board.grid.rows,
                           column, row, disc_color[i])) {
            marked_square[static_cast<std::size_t>(square)] = true;
            marked_centers.push_back(disc_centers[i]);
        }
    }

    emit_open_layer(os, "Markers", "layer-markers");
    if (!marked_centers.empty()) {
        // The template is generated once, centred on the origin, then stamped at
        // each immobilised disc's centre. Arm length is a fraction of the nominal
        // disc diameter (2 * radius).
        const double x_length = board.mark_length_fraction * 2.0 * board.disc.radius;
        const std::vector<Polyline> x_template =
            build_x_template(x_length, board.grid.roughness, board.grid.smoothing,
                             config.points_per_edge, render_engine);
        const double x_stroke = board.mark_stroke_width_units * scale;

        emit_open_stroke_group(os, board.mark_color, x_stroke);
        for (const Point& c : marked_centers) {
            for (const Polyline& line : x_template) {  // template centre -> disc centre
                os << "    <polyline points=\"";
                emit_points(os, line.points, scale, offset, c.x, c.y);
                os << "\"/>\n";
            }
        }
        os << "  </g>\n";
    }
    os << "  </g>\n";  // close Markers layer

    os << "</svg>\n";
    return os.str();
}

}  // namespace games::board

// Copyright Ben Paul Wise. All Rights Reserved.

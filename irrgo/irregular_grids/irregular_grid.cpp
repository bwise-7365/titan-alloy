#include "irregular_grid.hpp"

#include <cmath>
#include <cstddef>
#include <iomanip>
#include <locale>
#include <random>
#include <sstream>
#include <stdexcept>
#include <utility>

namespace games::board {

namespace {

constexpr double kTolerance = 1E-4;  // fraction of a square width
constexpr int kMaxSweeps = 250;

void validate(const GridSpec& spec) {
    if (spec.rows < 1) {
        throw std::invalid_argument("rows must be >= 1");
    }
    if (spec.columns < 1) {
        throw std::invalid_argument("columns must be >= 1");
    }
    if (!(spec.roughness >= 0.0 && spec.roughness <= 1.0)) {
        throw std::invalid_argument("roughness must be in [0,1]");
    }
    if (!(spec.smoothing >= 0.0 && spec.smoothing <= 1.0)) {
        throw std::invalid_argument("smoothing must be in [0,1]");
    }
}

void validate(const RenderConfig& config) {
    if (!(config.square_size > 0.0)) {
        throw std::invalid_argument("square_size must be > 0");
    }
    if (config.points_per_edge < 1) {
        throw std::invalid_argument("points_per_edge must be >= 1");
    }
}

// 53-bit mantissa uniform in [0,1). Built directly from the standardised
// mt19937_64 engine so the sequence is identical on every platform, unlike
// std::uniform_real_distribution, whose algorithm is implementation-defined.
double canonical_uniform(std::mt19937_64& engine) {
    const std::uint64_t bits = engine() >> 11;  // top 53 bits
    return static_cast<double>(bits) * (1.0 / 9007199254740992.0);  // / 2^53
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

// Relaxes one line to the fixed point and returns a fresh vector. Throws rather
// than returning a half-relaxed line if it fails to converge.
std::vector<double> smooth(const std::vector<double>& noise, double perfect_value,
                           double smoothing) {
    const std::size_t n = noise.size();
    std::vector<double> s(n, perfect_value);  // init to perfect; endpoints stay pinned

    for (int sweep = 0; sweep < kMaxSweeps; ++sweep) {
        double max_change = 0.0;
        for (std::size_t k = 1; k + 1 < n; ++k) {
            const double neighbour_average = 0.5 * (s[k - 1] + s[k + 1]);
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
        const std::vector<double> xs = smooth(noise, perfect_x, spec.smoothing);
        const std::vector<double> ys = along_axis(vertical_samples, config.points_per_edge);
        geometry.lines.push_back(zip_polyline(xs, ys, "vertical-" + std::to_string(i)));
    }

    const int horizontal_samples = spec.columns * config.points_per_edge + 1;
    for (int j = 0; j <= spec.rows; ++j) {
        const double perfect_y = static_cast<double>(j);
        const std::vector<double> noise =
            make_noise(perfect_y, horizontal_samples, spec.roughness, engine);
        const std::vector<double> ys = smooth(noise, perfect_y, spec.smoothing);
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
    os << std::fixed << std::setprecision(2);

    os << "<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"" << width
       << "\" height=\"" << height << "\" viewBox=\"0 0 " << width << ' ' << height << "\">\n";
    if (!style.background.empty()) {
        os << "  <rect x=\"0\" y=\"0\" width=\"" << width << "\" height=\"" << height
           << "\" fill=\"" << style.background << "\"/>\n";
    }

    os << std::setprecision(3);
    os << "  <g fill=\"none\" stroke=\"" << style.ink << "\" stroke-width=\"" << stroke
       << "\" stroke-linecap=\"round\" stroke-linejoin=\"round\">\n";
    os << std::setprecision(2);

    for (const Polyline& line : geometry.lines) {
        os << "    <!-- " << line.label << " -->\n";
        os << "    <polyline points=\"";
        bool first = true;
        for (const Point& p : line.points) {
            if (!first) {
                os << ' ';
            }
            first = false;
            os << (p.x * scale + margin) << ',' << (p.y * scale + margin);
        }
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

}  // namespace games::board

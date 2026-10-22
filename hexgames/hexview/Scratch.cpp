// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
#include "hexview/Scratch.h"

#include <cmath>
#include <random>
#include <stdexcept>

namespace HexView {

  namespace {

    constexpr double kTolerance = 1e-4;  // fraction of a unit (irrgo)
    constexpr int kMaxSweeps = 250;      // irrgo
    constexpr double kTwoPow53 = 9007199254740992.0;

    void
    requireUnit(double v, const char* what)
    {
      if (!(0.0 <= v && v <= 1.0)) {
        throw std::invalid_argument(std::string("scratch: ") + what + " must lie in [0, 1]");
      }
      return;
    }

    // irrgo's relax(), open topology: the two ends stay pinned at their noise value.
    std::vector<double>
    relax(const std::vector<double>& noise, double smoothing, double tolerance)
    {
      std::vector<double> s = noise;
      const std::size_t n = noise.size();
      for (int sweep = 0; sweep < kMaxSweeps; ++sweep) {
        double maxChange = 0.0;
        for (std::size_t k = 1; k + 1 < n; ++k) {
          const double neighbourAverage = 0.5 * (s[k - 1] + s[k + 1]);
          const double updated = (1.0 - smoothing) * noise[k] + smoothing * neighbourAverage;
          maxChange = std::max(maxChange, std::fabs(updated - s[k]));
          s[k] = updated;
        }
        if (maxChange < tolerance) {
          return s;
        }
      }
      throw std::runtime_error("scratch: smoothing did not converge within 250 sweeps");
    }

  }  // namespace

  double
  canonicalUniform(std::uint64_t draw)
  {
    return static_cast<double>(draw >> 11) * (1.0 / kTwoPow53);
  }

  std::vector<HexCoord::Pixel>
  scratch(const std::vector<HexCoord::Pixel>& polyline, const ScratchSpec& spec)
  {
    requireUnit(spec.roughness, "roughness");
    requireUnit(spec.smoothing, "smoothing");
    if (!(0.0 < spec.unit)) {
      throw std::invalid_argument("scratch: unit must be positive");
    }
    if (spec.samplesPerUnit < 1) {
      throw std::invalid_argument("scratch: samplesPerUnit must be at least 1");
    }
    if (polyline.size() < 2) {
      throw std::invalid_argument("scratch: a polyline needs at least two points");
    }
    if (0.0 == spec.roughness) {
      return polyline;
    }

    // arc length along the polyline
    std::vector<double> cumulative(polyline.size(), 0.0);
    for (std::size_t i = 1; i < polyline.size(); ++i) {
      const double dx = polyline[i].x - polyline[i - 1].x;
      const double dy = polyline[i].y - polyline[i - 1].y;
      cumulative[i] = cumulative[i - 1] + std::hypot(dx, dy);
    }
    const double length = cumulative.back();
    if (!(0.0 < length)) {
      return polyline;
    }
    const int samples = std::max(2, static_cast<int>(std::ceil(length / spec.unit * spec.samplesPerUnit)) + 1);

    // the point and the unit normal at each sample
    std::vector<HexCoord::Pixel> along(static_cast<std::size_t>(samples));
    std::vector<HexCoord::Pixel> normal(static_cast<std::size_t>(samples));
    std::size_t seg = 1;
    for (int k = 0; k < samples; ++k) {
      const double t = length * static_cast<double>(k) / static_cast<double>(samples - 1);
      while (seg + 1 < polyline.size() && cumulative[seg] < t) {
        ++seg;
      }
      const HexCoord::Pixel& a = polyline[seg - 1];
      const HexCoord::Pixel& b = polyline[seg];
      const double segLength = cumulative[seg] - cumulative[seg - 1];
      const double u = (0.0 < segLength) ? (t - cumulative[seg - 1]) / segLength : 0.0;
      along[k] = HexCoord::Pixel{a.x + (b.x - a.x) * u, a.y + (b.y - a.y) * u};
      const double nx = -(b.y - a.y);
      const double ny = (b.x - a.x);
      const double nl = std::hypot(nx, ny);
      normal[k] = (0.0 < nl) ? HexCoord::Pixel{nx / nl, ny / nl} : HexCoord::Pixel{0.0, 0.0};
    }

    // irrgo's make_noise: interior samples deviate, the ends stay on the line
    std::mt19937_64 engine(spec.seed);
    std::vector<double> noise(static_cast<std::size_t>(samples), 0.0);
    for (int k = 1; k + 1 < samples; ++k) {
      noise[k] = spec.roughness * (canonicalUniform(engine()) - 0.5) * spec.unit;
    }
    const std::vector<double> s = relax(noise, spec.smoothing, kTolerance * spec.unit);

    std::vector<HexCoord::Pixel> out(static_cast<std::size_t>(samples));
    for (int k = 0; k < samples; ++k) {
      out[k] = HexCoord::Pixel{along[k].x + normal[k].x * s[k], along[k].y + normal[k].y * s[k]};
    }
    return out;
  }

}  // namespace HexView
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------

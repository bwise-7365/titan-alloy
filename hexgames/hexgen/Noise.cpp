// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
#include "hexgen/Noise.h"

#include "hexgen/Random.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <numeric>
#include <stdexcept>
#include <utility>

namespace HexGen {

  namespace {

    constexpr double kUnset = -1e6;
    constexpr int kOctaves = 6;

    // Rows are j and columns i, as HexKrieg's (row, col) array.
    class Midpoint {
    public:
      Midpoint(Raster& r, double roughness, std::mt19937_64& gen)
          : r_(r), gen_(gen), scale_(200.0 / static_cast<double>(r.nx + r.ny) * roughness)
      {
      }

      void
      run()
      {
        std::fill(r_.v.begin(), r_.v.end(), kUnset);
        const int lastJ = r_.ny - 1;
        const int lastI = r_.nx - 1;
        const std::array<std::pair<int, int>, 4> corners{{{0, 0}, {0, lastI}, {lastJ, 0}, {lastJ, lastI}}};
        for (const auto& [j, i] : corners) {
          r_.at(i, j) = 1.0 + uniform01(gen_) * 99.0;
        }
        subdivide(0, 0, lastJ, lastI);
        return;
      }

    private:
      double perturb(int d) { return (2.0 * uniform01(gen_) - 1.0) * static_cast<double>(d) * scale_; }

      void
      setIfUnset(int j, int i, double mean, int d)
      {
        if (kUnset == r_.at(i, j)) {
          r_.at(i, j) = mean + perturb(d);
        }
        return;
      }

      void
      subdivide(int j1, int i1, int j2, int i2)
      {
        const int dj = j2 - j1;
        const int di = i2 - i1;
        if (1 >= dj && 1 >= di) {
          return;
        }
        const int jm = (j1 + j2) / 2;
        const int im = (i1 + i2) / 2;
        if (1 < di) {
          setIfUnset(j1, im, (r_.at(i1, j1) + r_.at(i2, j1)) / 2.0, di);
          setIfUnset(j2, im, (r_.at(i1, j2) + r_.at(i2, j2)) / 2.0, di);
        }
        if (1 < dj) {
          setIfUnset(jm, i1, (r_.at(i1, j1) + r_.at(i1, j2)) / 2.0, dj);
          setIfUnset(jm, i2, (r_.at(i2, j1) + r_.at(i2, j2)) / 2.0, dj);
        }
        if (1 < dj && 1 < di) {
          setIfUnset(jm, im, (r_.at(i1, j1) + r_.at(i2, j1) + r_.at(i1, j2) + r_.at(i2, j2)) / 4.0, dj + di);
        }
        subdivide(j1, i1, jm, im);
        subdivide(j1, im, jm, i2);
        subdivide(jm, i1, j2, im);
        subdivide(jm, im, j2, i2);
        return;
      }

      Raster& r_;
      std::mt19937_64& gen_;
      double scale_;
    };

    class Perlin {
    public:
      explicit Perlin(std::mt19937_64& gen)
      {
        std::iota(p_.begin(), p_.end(), 0);
        for (int i = 255; 0 < i; --i) {
          std::swap(p_[static_cast<std::size_t>(i)], p_[uniformBelow(gen, static_cast<std::uint64_t>(i) + 1)]);
        }
      }

      double
      noise(double x, double y) const
      {
        const double fx = std::floor(x);
        const double fy = std::floor(y);
        const int xi = static_cast<int>(fx) & 255;
        const int yi = static_cast<int>(fy) & 255;
        const double xf = x - fx;
        const double yf = y - fy;
        const double u = fade(xf);
        const double v = fade(yf);
        const int aa = p(p(xi) + yi);
        const int ab = p(p(xi) + yi + 1);
        const int ba = p(p(xi + 1) + yi);
        const int bb = p(p(xi + 1) + yi + 1);
        const double x1 = lerp(grad(aa, xf, yf), grad(ba, xf - 1.0, yf), u);
        const double x2 = lerp(grad(ab, xf, yf - 1.0), grad(bb, xf - 1.0, yf - 1.0), u);
        return lerp(x1, x2, v);
      }

    private:
      int p(int k) const { return p_[static_cast<std::size_t>(k & 255)]; }
      static double fade(double t) { return t * t * t * (t * (t * 6.0 - 15.0) + 10.0); }
      static double lerp(double a, double b, double t) { return a + t * (b - a); }

      static double
      grad(int hash, double x, double y)
      {
        const int h = hash & 3;
        if (0 == h) {
          return x + y;
        }
        if (1 == h) {
          return -x + y;
        }
        if (2 == h) {
          return x - y;
        }
        return -x - y;
      }

      std::array<int, 256> p_{};
    };

  }  // namespace

  double
  Raster::sample(HexCoord::Pixel px) const
  {
    const double fx = std::clamp((px.x - x0) / dx, 0.0, static_cast<double>(nx - 1));
    const double fy = std::clamp((px.y - y0) / dy, 0.0, static_cast<double>(ny - 1));
    const int i = std::min(static_cast<int>(fx), nx - 2);
    const int j = std::min(static_cast<int>(fy), ny - 2);
    const double s = fx - i;
    const double t = fy - j;
    const double top = at(i, j) + s * (at(i + 1, j) - at(i, j));
    const double bottom = at(i, j + 1) + s * (at(i + 1, j + 1) - at(i, j + 1));
    return top + t * (bottom - top);
  }

  Raster
  rasterOver(const GenMap& m)
  {
    const double size = m.grid().spec().size;
    const bool flat = HexCoord::Orientation::Flat == m.grid().spec().orientation;
    Raster r;
    r.dx = flat ? 1.5 * size : std::sqrt(3.0) * size;
    r.dy = flat ? std::sqrt(3.0) * size : 1.5 * size;
    HexCoord::Pixel lo = m.pixel(0);
    HexCoord::Pixel hi = lo;
    for (int hex = 1; hex < m.size(); ++hex) {
      const HexCoord::Pixel p = m.pixel(hex);
      lo = HexCoord::Pixel{std::min(lo.x, p.x), std::min(lo.y, p.y)};
      hi = HexCoord::Pixel{std::max(hi.x, p.x), std::max(hi.y, p.y)};
    }
    r.x0 = lo.x;
    r.y0 = lo.y;
    r.nx = static_cast<int>(std::ceil((hi.x - lo.x) / r.dx - 1e-9)) + 1;
    r.ny = static_cast<int>(std::ceil((hi.y - lo.y) / r.dy - 1e-9)) + 1;
    r.v.assign(static_cast<std::size_t>(r.nx * r.ny), 0.0);
    return r;
  }

  void
  fillFractal(Raster& r, double roughness, std::mt19937_64& gen)
  {
    Midpoint(r, roughness, gen).run();
    return;
  }

  void
  fillPerlin(Raster& r, double persistence, std::mt19937_64& gen)
  {
    const Perlin perlin(gen);
    const double scale = 4.0 / static_cast<double>(std::max(r.nx, r.ny));
    for (int j = 0; j < r.ny; ++j) {
      for (int i = 0; i < r.nx; ++i) {
        double value = 0.0;
        double total = 0.0;
        double amp = 1.0;
        double freq = 1.0;
        for (int o = 0; o < kOctaves && 1e-6 <= amp; ++o) {
          value += perlin.noise(i * scale * freq, j * scale * freq) * amp;
          total += amp;
          amp *= persistence;
          freq *= 2.0;
        }
        r.at(i, j) = value / total;
      }
    }
    return;
  }

  std::vector<double>
  fieldOf(const GenMap& m, Method method, double roughness, std::mt19937_64& gen)
  {
    Raster r = rasterOver(m);
    switch (method) {
      case Method::Fractal:
        fillFractal(r, roughness, gen);
        break;
      case Method::Perlin:
        fillPerlin(r, roughness, gen);
        break;
    }
    std::vector<double> out;
    out.reserve(static_cast<std::size_t>(m.size()));
    for (int hex = 0; hex < m.size(); ++hex) {
      out.push_back(r.sample(m.pixel(hex)));
    }
    return out;
  }

}  // namespace HexGen
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------

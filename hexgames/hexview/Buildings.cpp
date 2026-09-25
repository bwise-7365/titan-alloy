// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
#include "hexview/Buildings.h"

#include <algorithm>
#include <cmath>
#include <random>

namespace HexView {

  namespace {

    constexpr std::pair<double, double> kShapes[] = {{0.44, 0.24}, {0.32, 0.22}, {0.24, 0.38},
                                                     {0.30, 0.30}, {0.38, 0.20}, {0.22, 0.22}};

    double
    uniform01(std::mt19937_64& gen)
    {
      return static_cast<double>(gen() >> 11) * 0x1.0p-53;
    }

    double
    uniform(std::mt19937_64& gen, double lo, double hi)
    {
      return lo + (hi - lo) * uniform01(gen);
    }

    std::size_t
    choice(std::mt19937_64& gen, std::size_t n)
    {
      return static_cast<std::size_t>(gen() % n);
    }

    // How much of the smaller of two boxes the other covers, from 0 to 1.
    double
    coveredFraction(const Box& a, const Box& b)
    {
      const double w = std::min(a.x1, b.x1) - std::max(a.x0, b.x0);
      const double h = std::min(a.y1, b.y1) - std::max(a.y0, b.y0);
      if (w <= 0 || h <= 0) {
        return 0.0;
      }
      const double smaller = std::min((a.x1 - a.x0) * (a.y1 - a.y0), (b.x1 - b.x0) * (b.y1 - b.y0));
      return (w * h) / smaller;
    }

  }  // namespace

  std::uint64_t
  fnv1a64(std::string_view text)
  {
    std::uint64_t h = 0xcbf29ce484222325ull;
    for (const char c : text) {
      h ^= static_cast<unsigned char>(c);
      h *= 0x100000001b3ull;
    }
    return h;
  }

  std::uint64_t
  splitMix64(std::uint64_t x)
  {
    std::uint64_t z = x + 0x9e3779b97f4a7c15ull;
    z = (z ^ (z >> 30)) * 0xbf58476d1ce4e5b9ull;
    z = (z ^ (z >> 27)) * 0x94d049bb133111ebull;
    return z ^ (z >> 31);
  }

  std::vector<Box>
  scatterBuildings(std::string_view sheetId, std::string_view hexId, Pixel at, double size)
  {
    std::mt19937_64 gen(splitMix64(fnv1a64(sheetId) ^ splitMix64(fnv1a64(hexId))));
    const std::size_t wanted = 4 + choice(gen, 2);
    std::vector<Box> placed;
    for (int attempt = 0; attempt < 200 && placed.size() < wanted; ++attempt) {
      const auto [sw, sh] = kShapes[choice(gen, std::size(kShapes))];
      const double w = sw * size;
      const double h = sh * size;
      const double cx = at.x + uniform(gen, -0.55, 0.55) * size;
      const double cy = at.y + uniform(gen, -0.48, 0.48) * size;
      const Box box{cx - w / 2, cy - h / 2, cx + w / 2, cy + h / 2};
      const bool fitsP = std::all_of(placed.begin(), placed.end(), [&](const Box& b) {
        return coveredFraction(box, b) < 0.15 &&
               std::hypot(cx - (b.x0 + b.x1) / 2, cy - (b.y0 + b.y1) / 2) >= 0.30 * size;
      });
      if (fitsP) {
        placed.push_back(box);
      }
    }
    return placed;
  }

}  // namespace HexView
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------

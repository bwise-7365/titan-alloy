// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
// The height fields and the terrain and altitude steps.
// ----------------------------------------------
#include "HexGenTest.h"

#include "hexgen/Generate.h"
#include "hexgen/Noise.h"
#include "hexgen/Random.h"
#include "hexgen/Terrain.h"

#include <gtest/gtest.h>

#include <algorithm>
#include <cmath>

namespace {

  using HexGen::Method;
  using HexGen::Role;

  HexGen::Raster
  fractal(int cols, int rows, double roughness, std::uint64_t seed)
  {
    const HexGen::GenMap m(HexGenTest::terrainOnly(cols, rows, Method::Fractal));
    HexGen::Raster r = HexGen::rasterOver(m);
    std::mt19937_64 gen(seed);
    HexGen::fillFractal(r, roughness, gen);
    return r;
  }

  TEST(HexGenFields, TheRasterCoversEveryCentre)
  {
    for (const HexCoord::Orientation o : {HexCoord::Orientation::Flat, HexCoord::Orientation::Pointy}) {
      HexGen::Settings s = HexGenTest::terrainOnly(13, 7, Method::Perlin);
      s.orientation = o;
      const HexGen::GenMap m(s);
      const HexGen::Raster r = HexGen::rasterOver(m);
      for (int hex = 0; hex < m.size(); ++hex) {
        const HexCoord::Pixel p = m.pixel(hex);
        EXPECT_LE(r.x0 - 1e-9, p.x);
        EXPECT_LE(r.y0 - 1e-9, p.y);
        EXPECT_GE(r.x0 + (r.nx - 1) * r.dx + 1e-9, p.x);
        EXPECT_GE(r.y0 + (r.ny - 1) * r.dy + 1e-9, p.y);
      }
    }
    return;
  }

  TEST(HexGenFields, FractalWithoutRoughnessStaysWithinItsCorners)
  {
    for (int cols = 5; cols <= 12; ++cols) {
      for (int rows = 5; rows <= 9; ++rows) {
        const HexGen::Raster r = fractal(cols, rows, 0.0, 7u);
        const auto [lo, hi] = std::minmax_element(r.v.begin(), r.v.end());
        EXPECT_LE(1.0, *lo) << cols << " x " << rows;  // also: no point was left unset (-1e6)
        EXPECT_GE(100.0, *hi) << cols << " x " << rows;
      }
    }
    return;
  }

  TEST(HexGenFields, TheSameSeedGivesTheSameField)
  {
    EXPECT_EQ(fractal(20, 12, 0.5, 42u).v, fractal(20, 12, 0.5, 42u).v);
    EXPECT_NE(fractal(20, 12, 0.5, 42u).v, fractal(20, 12, 0.5, 43u).v);
    const HexGen::GenMap m(HexGenTest::terrainOnly(20, 12, Method::Perlin));
    std::mt19937_64 a(5u);
    std::mt19937_64 b(5u);
    EXPECT_EQ(HexGen::fieldOf(m, Method::Perlin, 0.5, a), HexGen::fieldOf(m, Method::Perlin, 0.5, b));
    return;
  }

  TEST(HexGenFields, PerlinIsBoundedAndNotFlat)
  {
    const HexGen::GenMap m(HexGenTest::terrainOnly(30, 18, Method::Perlin));
    std::mt19937_64 gen(9u);
    const std::vector<double> v = HexGen::fieldOf(m, Method::Perlin, 0.5, gen);
    const auto [lo, hi] = std::minmax_element(v.begin(), v.end());
    EXPECT_LE(-2.0, *lo);
    EXPECT_GE(2.0, *hi);
    EXPECT_LT(0.1, *hi - *lo);
    return;
  }

  TEST(HexGenTerrain, CityCountIsHexKriegs)
  {
    EXPECT_EQ(7, HexGen::cityCount(12 * 20, 3));
    EXPECT_EQ(103, HexGen::cityCount(50 * 80, 3));
    EXPECT_EQ(199, HexGen::cityCount(200 * 300, 3));
    EXPECT_EQ(0, HexGen::cityCount(240, 0));
    return;
  }

  TEST(HexGenTerrain, EachRoleTakesItsQuota)
  {
    for (const Method method : {Method::Fractal, Method::Perlin}) {
      HexGen::Settings s = HexGenTest::terrainOnly(30, 18, method);  // 540 hexes
      const HexGen::GenMap m = HexGen::generateMap(s);
      EXPECT_EQ(54, HexGenTest::countOf(m, s, Role::Low));
      EXPECT_EQ(HexGen::cityCount(540, 3), HexGenTest::countOf(m, s, Role::City));
      HexGenTest::terrainOf(s, Role::City).percent = 0;  // no cities take hexes from the others
      const HexGen::GenMap noCities = HexGen::generateMap(s);
      EXPECT_EQ(54, HexGenTest::countOf(noCities, s, Role::Low));
      EXPECT_EQ(108, HexGenTest::countOf(noCities, s, Role::High));
      EXPECT_EQ(135, HexGenTest::countOf(noCities, s, Role::Cover));
      EXPECT_EQ(540 - 54 - 108 - 135, HexGenTest::countOf(noCities, s, Role::Rest));
    }
    return;
  }

  TEST(HexGenTerrain, LowIsLowestAndHighIsHighest)
  {
    HexGen::Settings s = HexGenTest::terrainOnly(25, 15, Method::Fractal);
    HexGenTest::terrainOf(s, Role::City).percent = 0;
    const HexGen::GenMap m = HexGen::generateMap(s);
    double lowMax = -1e300;
    double highMin = 1e300;
    double otherMin = 1e300;
    double otherMax = -1e300;
    for (int hex = 0; hex < m.size(); ++hex) {
      const double h = m.height[static_cast<std::size_t>(hex)];
      const std::size_t t = m.terrain[static_cast<std::size_t>(hex)];
      if (s.terrainIndex(Role::Low) == t) {
        lowMax = std::max(lowMax, h);
      }
      else if (s.terrainIndex(Role::High) == t) {
        highMin = std::min(highMin, h);
      }
      else {
        otherMin = std::min(otherMin, h);
        otherMax = std::max(otherMax, h);
      }
    }
    EXPECT_LE(lowMax, otherMin);
    EXPECT_LE(otherMax, highMin);
    return;
  }

  TEST(HexGenTerrain, AltitudeRunsFromTheDepthToTheHighestLand)
  {
    for (const Method method : {Method::Fractal, Method::Perlin}) {
      HexGen::Settings s = HexGenTest::terrainOnly(20, 20, method);
      s.altitude = HexGen::AltitudeSettings{};
      const HexGen::GenMap m = HexGen::generateMap(s);
      ASSERT_EQ(static_cast<std::size_t>(m.size()), m.alt.size());
      const auto [lo, hi] = std::minmax_element(m.alt.begin(), m.alt.end());
      EXPECT_NEAR(-200.0, *lo, 1e-9);
      EXPECT_NEAR(2000.0, *hi, 1e-9);
      for (int hex = 0; hex < m.size(); ++hex) {
        const bool lowP = s.terrainIndex(Role::Low) == m.terrain[static_cast<std::size_t>(hex)];
        EXPECT_EQ(lowP, m.alt[static_cast<std::size_t>(hex)] < 0.0) << hex;
      }
      s.altitude.reset();
      EXPECT_EQ(m.terrain, HexGen::generateMap(s).terrain);  // altitude changes no terrain
    }
    return;
  }

}  // namespace
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------

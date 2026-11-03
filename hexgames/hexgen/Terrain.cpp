// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
#include "hexgen/Terrain.h"

#include "hexgen/Random.h"

#include <algorithm>
#include <cmath>
#include <numeric>
#include <vector>

namespace HexGen {

  namespace {

    int
    quota(int hexes, int percent)
    {
      return percent * hexes / 100;
    }

    std::vector<int>
    hexesByHeight(const GenMap& m)
    {
      std::vector<int> order(static_cast<std::size_t>(m.size()));
      std::iota(order.begin(), order.end(), 0);
      std::stable_sort(order.begin(), order.end(),
                       [&](int a, int b) { return m.height[static_cast<std::size_t>(a)] < m.height[static_cast<std::size_t>(b)]; });
      return order;
    }

  }  // namespace

  int
  cityCount(int hexes, int percent)
  {
    const double x = static_cast<double>(percent) * hexes / 100.0;
    const double y = 200.0;
    if (0.0 >= x) {
      return 0;
    }
    return static_cast<int>(std::lround(x * y / std::sqrt(x * x + y * y)));
  }

  void
  assignTerrain(GenMap& m, const Settings& s, std::mt19937_64& cityGen)
  {
    const int n = m.size();
    const std::size_t low = s.terrainIndex(Role::Low);
    const std::size_t high = s.terrainIndex(Role::High);
    const std::size_t cover = s.terrainIndex(Role::Cover);
    const std::size_t city = s.terrainIndex(Role::City);
    const std::size_t rest = s.terrainIndex(Role::Rest);
    m.terrain.assign(static_cast<std::size_t>(n), rest);

    const std::vector<int> byHeight = hexesByHeight(m);
    const int nLow = quota(n, s.terrains[low].percent);
    const int nHigh = quota(n, s.terrains[high].percent);
    for (int k = 0; k < nLow; ++k) {
      m.terrain[static_cast<std::size_t>(byHeight[static_cast<std::size_t>(k)])] = low;
    }
    for (int k = 0; k < nHigh; ++k) {
      m.terrain[static_cast<std::size_t>(byHeight[static_cast<std::size_t>(n - 1 - k)])] = high;
    }

    std::vector<int> open;
    for (int hex = 0; hex < n; ++hex) {
      if (rest == m.terrain[static_cast<std::size_t>(hex)]) {
        open.push_back(hex);
      }
    }
    std::stable_sort(open.begin(), open.end(),
                     [&](int a, int b) { return m.cover[static_cast<std::size_t>(b)] < m.cover[static_cast<std::size_t>(a)]; });
    const std::size_t nCover = std::min(open.size(), static_cast<std::size_t>(quota(n, s.terrains[cover].percent)));
    for (std::size_t k = 0; k < nCover; ++k) {
      m.terrain[static_cast<std::size_t>(open[k])] = cover;
    }

    std::vector<int> land;
    for (int hex = 0; hex < n; ++hex) {
      if (low != m.terrain[static_cast<std::size_t>(hex)]) {
        land.push_back(hex);
      }
    }
    shuffle(land, cityGen);
    const std::size_t nCity = std::min(land.size(), static_cast<std::size_t>(cityCount(n, s.terrains[city].percent)));
    for (std::size_t k = 0; k < nCity; ++k) {
      m.terrain[static_cast<std::size_t>(land[k])] = city;
    }
    return;
  }

  void
  calibrateAltitude(GenMap& m, const Settings& s)
  {
    const int n = m.size();
    const std::vector<int> byHeight = hexesByHeight(m);
    const auto heightAt = [&](int k) { return m.height[static_cast<std::size_t>(byHeight[static_cast<std::size_t>(k)])]; };
    const double vMin = heightAt(0);
    const double vMax = heightAt(n - 1);
    const int nLow = quota(n, s.terrain(Role::Low).percent);
    const double waterline = 0 >= nLow ? vMin : n <= nLow ? vMax : (heightAt(nLow - 1) + heightAt(nLow)) / 2.0;
    m.alt.assign(static_cast<std::size_t>(n), 0.0);
    for (int hex = 0; hex < n; ++hex) {
      const double v = m.height[static_cast<std::size_t>(hex)];
      double& alt = m.alt[static_cast<std::size_t>(hex)];
      if (waterline < v) {
        alt = (v - waterline) / (vMax - waterline) * s.altitude->maxLand;
      }
      else if (v < waterline) {
        alt = (v - waterline) / (waterline - vMin) * s.altitude->maxDepth;
      }
    }
    return;
  }

}  // namespace HexGen
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------

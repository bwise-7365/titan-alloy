// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
// What the hexgen tests share: the corpus settings, small variants of them, and counts.
// ----------------------------------------------
#pragma once
#include "hexgen/GenMap.h"
#include "hexgen/Settings.h"

#include <cstddef>
#include <filesystem>
#include <queue>
#include <set>
#include <string>
#include <vector>

namespace HexGenTest {

  inline std::filesystem::path
  corpus(const std::string& name)
  {
    return std::filesystem::path(HEXRULES_DIR) / "corpus" / name;
  }

  // HexKrieg's settings with no roads: the terrain steps alone.
  inline HexGen::Settings
  terrainOnly(int cols, int rows, HexGen::Method method)
  {
    HexGen::Settings s = HexGen::Settings::hexkrieg();
    s.cols = cols;
    s.rows = rows;
    s.method = method;
    s.roads.reset();
    return s;
  }

  inline HexGen::TerrainSetting&
  terrainOf(HexGen::Settings& s, HexGen::Role role)
  {
    return s.terrains[s.terrainIndex(role)];
  }

  inline int
  countOf(const HexGen::GenMap& m, const HexGen::Settings& s, HexGen::Role role)
  {
    int n = 0;
    for (const std::size_t t : m.terrain) {
      n += s.terrainIndex(role) == t ? 1 : 0;
    }
    return n;
  }

  // Every hex joined to `from` by road steps, `from` included.
  inline std::set<int>
  byRoadFrom(const HexGen::GenMap& m, int from)
  {
    std::vector<std::vector<int>> next(static_cast<std::size_t>(m.size()));
    for (const auto& [a, b] : m.roads()) {
      next[static_cast<std::size_t>(a)].push_back(b);
      next[static_cast<std::size_t>(b)].push_back(a);
    }
    std::set<int> seen{from};
    std::queue<int> todo;
    todo.push(from);
    while (!todo.empty()) {
      const int here = todo.front();
      todo.pop();
      for (const int there : next[static_cast<std::size_t>(here)]) {
        if (seen.insert(there).second) {
          todo.push(there);
        }
      }
    }
    return seen;
  }

}  // namespace HexGenTest
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------

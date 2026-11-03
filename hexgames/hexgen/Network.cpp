// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
#include "hexgen/Network.h"

#include "hexgen/CostGraph.h"
#include "hexgen/Random.h"

#include <algorithm>
#include <cstdlib>
#include <map>
#include <set>
#include <stdexcept>

namespace HexGen {

  namespace {

    using CostMatrix = std::vector<std::vector<std::int64_t>>;

    void
    layPath(GenMap& m, const std::vector<HexSearch::NodeIndex>& path)
    {
      for (std::size_t k = 1; k < path.size(); ++k) {
        m.addRoad(static_cast<int>(path[k - 1]), static_cast<int>(path[k]));
      }
      return;
    }

    // The groups of nodes joined to one another by some path, each in index order, in order of their
    // lowest node.
    std::vector<std::vector<int>>
    groupsOf(const CostMatrix& cost)
    {
      const int n = static_cast<int>(cost.size());
      std::vector<int> group(static_cast<std::size_t>(n), -1);
      std::vector<std::vector<int>> groups;
      for (int start = 0; start < n; ++start) {
        if (0 <= group[static_cast<std::size_t>(start)]) {
          continue;
        }
        const int g = static_cast<int>(groups.size());
        groups.emplace_back();
        for (int v = start; v < n; ++v) {
          if (kNoPath != cost[static_cast<std::size_t>(start)][static_cast<std::size_t>(v)]) {
            group[static_cast<std::size_t>(v)] = g;
            groups.back().push_back(v);
          }
        }
      }
      return groups;
    }

    // The hex at position p along one map edge.
    int
    edgeHex(const GenMap& m, Side side, int p)
    {
      switch (side) {
        case Side::Top:
          return m.indexOf(p, 0);
        case Side::Bottom:
          return m.indexOf(p, m.rows() - 1);
        case Side::Left:
          return m.indexOf(0, p);
        case Side::Right:
          return m.indexOf(m.cols() - 1, p);
      }
      throw std::invalid_argument("edgeHex: not a Side");
    }

    int
    edgeLength(const GenMap& m, Side side)
    {
      return Side::Top == side || Side::Bottom == side ? m.cols() : m.rows();
    }

  }  // namespace

  std::vector<std::pair<int, int>>
  spanningForest(const CostMatrix& cost)
  {
    const std::size_t n = cost.size();
    std::vector<bool> joined(n, false);
    std::vector<std::int64_t> key(n, kNoPath);
    std::vector<int> parent(n, -1);
    std::vector<std::pair<int, int>> edges;
    for (std::size_t start = 0; start < n; ++start) {
      if (joined[start]) {
        continue;
      }
      key[start] = 0;
      while (true) {
        std::size_t u = n;
        for (std::size_t v = 0; v < n; ++v) {
          if (!joined[v] && kNoPath != key[v] && (n == u || key[v] < key[u])) {
            u = v;
          }
        }
        if (n == u) {
          break;
        }
        joined[u] = true;
        if (0 <= parent[u]) {
          edges.emplace_back(std::min(parent[u], static_cast<int>(u)), std::max(parent[u], static_cast<int>(u)));
        }
        for (std::size_t v = 0; v < n; ++v) {
          if (!joined[v] && cost[u][v] < key[v]) {
            key[v] = cost[u][v];
            parent[v] = static_cast<int>(u);
          }
        }
      }
    }
    return edges;
  }

  std::vector<int>
  furthestInsertionTour(const CostMatrix& cost, const std::vector<int>& members)
  {
    if (2 > members.size()) {
      return members;
    }
    const auto c = [&](int a, int b) { return cost[static_cast<std::size_t>(a)][static_cast<std::size_t>(b)]; };
    std::size_t first = 0;
    std::size_t second = 1;
    for (std::size_t i = 0; i < members.size(); ++i) {
      for (std::size_t j = i + 1; j < members.size(); ++j) {
        if (c(members[first], members[second]) < c(members[i], members[j])) {
          first = i;
          second = j;
        }
      }
    }
    std::vector<int> tour{members[first], members[second]};
    std::vector<int> out;
    std::vector<std::int64_t> nearest;
    for (std::size_t i = 0; i < members.size(); ++i) {
      if (first != i && second != i) {
        out.push_back(members[i]);
        nearest.push_back(std::min(c(members[i], tour[0]), c(members[i], tour[1])));
      }
    }
    while (!out.empty()) {
      std::size_t far = 0;
      for (std::size_t k = 1; k < out.size(); ++k) {
        if (nearest[far] < nearest[k]) {
          far = k;
        }
      }
      const int x = out[far];
      out.erase(out.begin() + static_cast<std::ptrdiff_t>(far));
      nearest.erase(nearest.begin() + static_cast<std::ptrdiff_t>(far));
      std::size_t at = 0;
      std::int64_t best = 0;
      for (std::size_t i = 0; i < tour.size(); ++i) {
        const int a = tour[i];
        const int b = tour[(i + 1) % tour.size()];
        const std::int64_t added = c(a, x) + c(x, b) - c(a, b);
        if (0 == i || added < best) {
          at = i;
          best = added;
        }
      }
      tour.insert(tour.begin() + static_cast<std::ptrdiff_t>(at + 1), x);
      for (std::size_t k = 0; k < out.size(); ++k) {
        nearest[k] = std::min(nearest[k], c(out[k], x));
      }
    }
    return tour;
  }

  void
  buildRoads(GenMap& m, const Settings& s)
  {
    const std::size_t city = s.terrainIndex(Role::City);
    std::vector<int> cities;
    for (int hex = 0; hex < m.size(); ++hex) {
      if (city == m.terrain[static_cast<std::size_t>(hex)]) {
        cities.push_back(hex);
      }
    }
    if (2 > cities.size()) {
      return;
    }
    const CostGraph graph(m, s);
    HexSearch::SearchScratch scratch;
    CostMatrix cost(cities.size(), std::vector<std::int64_t>(cities.size(), kNoPath));
    for (std::size_t i = 0; i < cities.size(); ++i) {
      const HexSearch::Field<HexSearch::CostHalves> field = graph.searchFrom(cities[i], scratch);
      for (std::size_t j = 0; j < cities.size(); ++j) {
        const auto node = static_cast<HexSearch::NodeIndex>(cities[j]);
        if (field.reachedP(node)) {
          cost[i][j] = field.distance(node).halves;
        }
      }
    }

    std::set<std::pair<int, int>> pairs;
    for (const std::pair<int, int>& edge : spanningForest(cost)) {
      pairs.insert(edge);
    }
    for (const std::vector<int>& group : groupsOf(cost)) {
      const std::vector<int> tour = furthestInsertionTour(cost, group);
      for (std::size_t k = 0; 2 <= tour.size() && k < tour.size(); ++k) {
        const int a = tour[k];
        const int b = tour[(k + 1) % tour.size()];
        pairs.insert({std::min(a, b), std::max(a, b)});
      }
    }

    std::map<int, std::vector<int>> partners;
    for (const auto& [a, b] : pairs) {
      partners[a].push_back(b);
    }
    for (const auto& [a, ends] : partners) {
      const HexSearch::Field<HexSearch::CostHalves> field = graph.searchFrom(cities[static_cast<std::size_t>(a)], scratch);
      for (const int b : ends) {
        layPath(m, field.pathTo(static_cast<HexSearch::NodeIndex>(cities[static_cast<std::size_t>(b)])));
      }
    }
    return;
  }

  std::vector<int>
  addEdgeRoads(GenMap& m, const Settings& s, const SideSetting& side, std::mt19937_64& gen)
  {
    const CostGraph graph(m, s);
    const int length = edgeLength(m, side.at);
    std::vector<int> candidates;
    for (int p = 1; p + 1 < length; ++p) {
      if (graph.roadsEnterP(edgeHex(m, side.at, p))) {
        candidates.push_back(p);
      }
    }
    shuffle(candidates, gen);
    std::vector<int> chosen;
    for (const int p : candidates) {
      if (static_cast<int>(chosen.size()) == side.roads) {
        break;
      }
      const bool spacedP = std::all_of(chosen.begin(), chosen.end(),
                                       [&](int o) { return length <= 2 * side.roads * std::abs(o - p); });
      if (spacedP) {
        chosen.push_back(p);
      }
    }
    const std::size_t city = s.terrainIndex(Role::City);
    const std::size_t nCity = std::min(chosen.size(), static_cast<std::size_t>(side.cities));
    for (std::size_t k = 0; k < nCity; ++k) {
      m.terrain[static_cast<std::size_t>(edgeHex(m, side.at, chosen[k]))] = city;
    }
    std::sort(chosen.begin(), chosen.end());

    std::vector<int> ends;
    HexSearch::SearchScratch scratch;
    for (const int p : chosen) {
      const int hex = edgeHex(m, side.at, p);
      ends.push_back(hex);
      if (m.onRoadP(hex)) {
        continue;
      }
      const HexSearch::Field<HexSearch::CostHalves> field = graph.searchFrom(hex, scratch);
      for (const HexSearch::NodeIndex node : field.reached()) {  // cheapest first, then lowest index
        if (m.onRoadP(static_cast<int>(node))) {
          layPath(m, field.pathTo(node));
          break;
        }
      }
    }
    return ends;
  }

}  // namespace HexGen
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------

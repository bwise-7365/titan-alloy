// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
// Preprocessing implementation: Floyd-Warshall, diagonal correction, arc
// pruning diagnostics, reduced-problem construction.
// ----------------------------------------------
#include "reduction.hpp"

#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace VIMCP::Network {

  ShortestRoutes
  computeShortestRoutes(const Instance& inst)
  {
    validateInstance(inst);
    const Index m = inst.numNodes;

    ShortestRoutes routes;
    routes.distance = inst.cost;
    routes.next.assign(static_cast<size_t>(m), vector<Index>(static_cast<size_t>(m)));
    for (Index i = 0; i < m; ++i) {
      routes.distance(i, i) = 0.0;      // staying put is free; self-SUPPLY is
                                        // priced by selfDistance below
      for (Index j = 0; j < m; ++j) {
        routes.next[i][j] = j;          // direct arc until relaxed
      }
    }

    // Floyd-Warshall. Positive costs keep the zero diagonal tight, so i == j
    // never relaxes and needs no special casing.
    for (Index k = 0; k < m; ++k) {
      for (Index i = 0; i < m; ++i) {
        const double toK = routes.distance(i, k);
        for (Index j = 0; j < m; ++j) {
          const double viaK = toK + routes.distance(k, j);
          if (viaK < routes.distance(i, j)) {
            routes.distance(i, j) = viaK;
            routes.next[i][j] = routes.next[i][k];
          }
        }
      }
    }

    // Diagonal correction: the cheapest AT-LEAST-ONE-ARC route n -> n is the
    // self-arc or a round trip decomposed by its first arc (reduction.md
    // section 1).
    routes.selfDistance.resize(m);
    routes.selfVia.assign(static_cast<size_t>(m), -1);
    for (Index n = 0; n < m; ++n) {
      double best = inst.cost(n, n);
      Index via = -1;
      for (Index k = 0; k < m; ++k) {
        if (k != n) {
          const double roundTrip = inst.cost(n, k) + routes.distance(k, n);
          if (roundTrip < best) {
            best = roundTrip;
            via = k;
          }
        }
      }
      routes.selfDistance(n) = best;
      routes.selfVia[static_cast<size_t>(n)] = via;
    }
    return routes;
  }

  vector<Index>
  routeNodes(const ShortestRoutes& routes, Index from, Index to)
  {
    const Index m = routes.distance.rows();
    if (!(0 <= from && from < m && 0 <= to && to < m)) {
      throw std::invalid_argument("routeNodes: node index out of range.");
    }

    vector<Index> nodes;
    nodes.push_back(from);
    if (from == to) {
      const Index via = routes.selfVia[static_cast<size_t>(from)];
      if (-1 == via) {
        nodes.push_back(from);          // the self-arc route [n, n]
        return nodes;
      }
      const vector<Index> back = routeNodes(routes, via, to);
      nodes.insert(nodes.end(), back.begin(), back.end());
      return nodes;
    }

    Index current = from;
    while (current != to) {
      current = routes.next[static_cast<size_t>(current)][static_cast<size_t>(to)];
      nodes.push_back(current);
      if (m + 1 < static_cast<Index>(nodes.size())) {
        throw std::runtime_error("routeNodes: successor walk did not terminate.");
      }
    }
    return nodes;
  }

  Index
  countDominatedArcs(const Instance& inst, const ShortestRoutes& routes,
                     double relTol)
  {
    if (0.0 > relTol) {
      throw std::invalid_argument("countDominatedArcs: relTol must be >= 0.");
    }
    Index dominated = 0;
    for (Index i = 0; i < inst.numNodes; ++i) {
      for (Index j = 0; j < inst.numNodes; ++j) {
        if (i != j
            && inst.cost(i, j) > routes.distance(i, j) * (1.0 + relTol)) {
          ++dominated;
        }
      }
    }
    return dominated;
  }

  ReducedProblem
  makeReducedProblem(const Instance& inst, const ShortestRoutes& routes,
                     const ScreenParams& screen)
  {
    validateInstance(inst);
    if (0 > screen.maxSourcesPerSink) {
      throw std::invalid_argument(
          "makeReducedProblem: maxSourcesPerSink must be >= 0 (0 = keep all).");
    }
    if (!(0.0 <= screen.gapFraction)
        || !std::isfinite(screen.gapFraction)) {
      throw std::invalid_argument(
          "makeReducedProblem: gapFraction must be finite and >= 0.");
    }

    ReducedProblem reduced;
    reduced.sources = sourceNodes(inst);
    reduced.sinks = sinkNodes(inst);
    const Index numSources = static_cast<Index>(reduced.sources.size());
    const Index numSinks = static_cast<Index>(reduced.sinks.size());

    reduced.shipCost.resize(numSources, numSinks);
    for (Index s = 0; s < numSources; ++s) {
      for (Index t = 0; t < numSinks; ++t) {
        const Index fromNode = reduced.sources[static_cast<size_t>(s)];
        const Index toNode = reduced.sinks[static_cast<size_t>(t)];
        reduced.shipCost(s, t) = (fromNode == toNode)
                                     ? routes.selfDistance(fromNode)
                                     : routes.distance(fromNode, toNode);
      }
    }

    // Screen per sink, ascending cost (ties by position, deterministic).
    // With both rules off everything is kept; otherwise kept = union of the
    // count and gap rules (both keep prefixes of the ascending order, so the
    // union is the longer prefix).
    const bool keepAllP =
        0 == screen.maxSourcesPerSink && 0.0 >= screen.gapFraction;
    reduced.kept.assign(static_cast<size_t>(numSinks), vector<Index>());
    for (Index t = 0; t < numSinks; ++t) {
      vector<Index> order(static_cast<size_t>(numSources));
      for (Index s = 0; s < numSources; ++s) {
        order[static_cast<size_t>(s)] = s;
      }
      std::sort(order.begin(), order.end(),
                [&reduced, t](Index a, Index b) {
                  if (reduced.shipCost(a, t) != reduced.shipCost(b, t)) {
                    return reduced.shipCost(a, t) < reduced.shipCost(b, t);
                  }
                  return a < b;
                });
      const double gapLimit =
          reduced.shipCost(order[0], t) * (1.0 + screen.gapFraction);
      vector<Index> kept;
      for (size_t pos = 0; pos < order.size(); ++pos) {
        const bool byCountP =
            0 < screen.maxSourcesPerSink
            && static_cast<Index>(pos) < screen.maxSourcesPerSink;
        const bool byGapP = 0.0 < screen.gapFraction
                            && reduced.shipCost(order[pos], t) <= gapLimit;
        if (keepAllP || byCountP || byGapP) {
          kept.push_back(order[pos]);
        }
      }
      reduced.kept[static_cast<size_t>(t)] = kept;
    }
    return reduced;
  }

  ReducedProblem
  makeReducedProblem(const Instance& inst, const ShortestRoutes& routes,
                     Index maxSourcesPerSink)
  {
    ScreenParams screen;
    screen.maxSourcesPerSink = maxSourcesPerSink;
    return makeReducedProblem(inst, routes, screen);
  }

} // namespace VIMCP::Network
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------

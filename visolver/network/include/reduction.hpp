// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
// Preprocessing: all-pairs shortest movement routes, dominated-arc pruning,
// and construction of the reduced source-sink problem (doc/reduction.md).
// ----------------------------------------------
#ifndef VINCP_NETWORK_REDUCTION_HPP
#define VINCP_NETWORK_REDUCTION_HPP

#include "instance.hpp"

namespace VINCP::Network {

  // ---------------------------------------------------------------------------
  // Shortest movement routes (reduction.md section 1)
  // ---------------------------------------------------------------------------

  // All-pairs shortest routes on the cost graph. distance(i, j) is the ordinary
  // shortest-path cost d0_ij (zero diagonal). Because supply must LEAVE and
  // resupply must ARRIVE via arcs (the F1 (delivery) constraint), the cost of
  // self-supply at n is not 0 but selfDistance(n) = min(c_nn,
  // min_k (c_nk + d0_kn)): the cheapest at-least-one-arc route n -> n.
  // next[i][j] is the node AFTER i on a canonical shortest i -> j path (path
  // recovery walks it forward); selfVia[n] is -1 when the self-arc (n, n)
  // realizes selfDistance(n), else the first hop of the cheaper round trip.
  struct ShortestRoutes {
    MatrixXd distance;          // d0: numNodes x numNodes, zero diagonal
    vector<vector<Index>> next; // successor matrix for path recovery
    VectorXd selfDistance;      // d-hat diagonal (>= 1 arc), size numNodes
    vector<Index> selfVia;      // -1 = self-arc, else first hop of round trip
  };

  // Floyd-Warshall, O(numNodes^3); positive costs, so no negative cycles.
  ShortestRoutes computeShortestRoutes(const Instance& inst);

  // Node sequence of the canonical shortest route, endpoints included.
  // from != to: [from, ..., to] along the successor matrix. from == to: the
  // self-supply route, [n, n] for the self-arc or [n, via, ..., n] for a round
  // trip. Throws std::invalid_argument on out-of-range nodes and
  // std::runtime_error if the successor walk fails to terminate (corrupt data).
  vector<Index> routeNodes(const ShortestRoutes& routes, Index from, Index to);

  // Number of DOMINATED off-diagonal arcs: c_ij > d0_ij * (1 + relTol), i.e. a
  // multi-hop route strictly beats the direct arc (Proposition R2: such arcs
  // are on no shortest route and in no undominated plan). Diagnostic for the
  // report; the canonical expansion avoids them by construction.
  Index countDominatedArcs(const Instance& inst, const ShortestRoutes& routes,
                           double relTol = 1.0e-12);

  // ---------------------------------------------------------------------------
  // Reduced source-sink problem (reduction.md section 2)
  // ---------------------------------------------------------------------------

  // The reduced problem P_eps ships t_{s,t} tons from source s to sink t at
  // per-ton cost shipCost(s, t) = d-hat. sources/sinks hold the instance node
  // ids; shipCost is indexed by POSITION in those arrays. kept[t] lists the
  // source positions admitted for sink t by the k-cheapest screen, in
  // ascending cost order (ties by position) -- the R3 certificate loop (task
  // C4) may append to these lists.
  struct ReducedProblem {
    vector<Index> sources;      // node ids with C_i > 0
    vector<Index> sinks;        // node ids with D_i > 0
    MatrixXd shipCost;          // d-hat, |sources| x |sinks|, ALL pairs
    vector<vector<Index>> kept; // kept[t] = admitted source positions, cheap first
  };

  // Build the reduced problem. maxSourcesPerSink = 0 keeps every pair (the
  // exact default at moderate sizes); k > 0 keeps each sink's k cheapest
  // sources (the R3 screen; exactness is restored by the certificate loop).
  ReducedProblem makeReducedProblem(const Instance& inst,
                                    const ShortestRoutes& routes,
                                    Index maxSourcesPerSink = 0);

} // namespace VINCP::Network

#endif // VINCP_NETWORK_REDUCTION_HPP
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------

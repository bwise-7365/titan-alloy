// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
// Flow-planning problem instance: node data, movement costs, the ton-mile
// budget, and the random instance generator (see network/doc/formulation.md).
// ----------------------------------------------
#ifndef VINCP_NETWORK_INSTANCE_HPP
#define VINCP_NETWORK_INSTANCE_HPP

#include "vincp.hpp"

#include <cstdint>
#include <vector>

using std::vector;

namespace VINCP::Network {

  // ---------------------------------------------------------------------------
  // Instance
  // ---------------------------------------------------------------------------

  // One flow-planning problem (formulation.md sections 1-4). Node i carries a
  // supply capacity C_i >= 0, a demand D_i >= 0, and a priority P_i > 0 (only
  // meaningful where D_i > 0). cost(i, j) = c_ij > 0 is the per-ton cost in
  // miles of moving stuff i -> j, including the diagonal self-supply cost c_ii
  // (small but positive: using own capacity for own demand is cheap, not free).
  struct Instance {
    Index numNodes = 0;
    VectorXd supplyCap;      // C_i >= 0                (size numNodes)
    VectorXd demand;         // D_i >= 0                (size numNodes)
    VectorXd priority;       // P_i > 0 where D_i > 0   (size numNodes)
    MatrixXd cost;           // c_ij > 0                (numNodes x numNodes)
    double tonMileLimit = 0.0;   // L; 0.0 means "not yet calibrated" (task B2
                                 // sets it to ~80% of the greedy plan's usage)
  };

  // Throws std::invalid_argument on any structural defect: mismatched sizes,
  // negative C or D, non-positive P at a demand node, non-positive cost entry,
  // or a negative tonMileLimit. (tonMileLimit == 0.0 is legal: uncalibrated.)
  void validateInstance(const Instance& inst);

  // Nodes with C_i > 0 (sources) and with D_i > 0 (sinks), in index order.
  // A node with both appears in both lists.
  vector<Index> sourceNodes(const Instance& inst);
  vector<Index> sinkNodes(const Instance& inst);

  // Sum of all C_i / of all D_i.
  double totalSupplyCap(const Instance& inst);
  double totalDemand(const Instance& inst);

  // ---------------------------------------------------------------------------
  // Random instance generator
  // ---------------------------------------------------------------------------

  // Generation profile. The defaults encode the 70-node example of the problem
  // spec: 20 supply-only, 20 supply-and-demand, 30 demand-only nodes; tonnages
  // 1000-5000; priorities 1-10; self-supply cost 1-5 miles.
  //
  // Off-diagonal costs come from a geometric model: nodes are placed uniformly
  // at random in a squareSide x squareSide plane and
  //   c_ij = (costFloor + milesPerUnit * euclidean(i, j)) * jitter_ij,
  // with jitter_ij drawn independently per ORDERED pair from
  // U[1, 1 + asymmetryMax]. This yields distance-like, near-metric costs in
  // roughly [costFloor, 2000] with the spec's 1-10% directional asymmetry
  // (c_ij / c_ji lies in [1/(1+asymmetryMax), 1+asymmetryMax]). The additive
  // costFloor acts as a per-move handling charge, so two-hop routes pay it
  // twice: triangle-inequality violations come only from the jitter.
  struct InstanceProfile {
    Index numSupplyOnly = 20;    // C_i > 0, D_i = 0
    Index numBoth       = 20;    // C_i > 0, D_i > 0
    Index numDemandOnly = 30;    // C_i = 0, D_i > 0

    double supplyLo = 1000.0, supplyHi = 5000.0;   // C_i range (tons)
    double demandLo = 1000.0, demandHi = 5000.0;   // D_i range (tons)
    double priorityLo = 1.0,  priorityHi = 10.0;   // P_i range
    double selfCostLo = 1.0,  selfCostHi = 5.0;    // c_ii range (miles)

    double squareSide   = 1000.0;   // node coordinates uniform in [0, side]^2
    double costFloor    = 100.0;    // additive per-move miles (min off-diagonal)
    double milesPerUnit = 1.35;     // miles per unit of Euclidean separation
    double asymmetryMax = 0.05;     // directional jitter: U[1, 1 + asymmetryMax]

    // Which laydown places the nodes and prices the off-diagonal arcs:
    //   0  fully random (fields above): uniform points in the squareSide
    //      square; cost = (costFloor + milesPerUnit * separation) * jitter,
    //      jitter ~ U[1, 1 + asymmetryMax] per ordered pair.
    //   1  banded rectangles (network/alternate-laydown.txt): group g = 0/1/2
    //      of [supply-only | both | demand-only] gets
    //      x ~ U[g * bandXStep, g * bandXStep + bandXWidth],
    //      y ~ U[bandYLo, bandYHi]; cost is the BARE Euclidean separation
    //      (no floor, no scale) times jitter ~ U[1 - jitterHalfWidth,
    //      1 + jitterHalfWidth], independent per ordered pair.
    //   2+ reserved, not yet defined: validateProfile throws.
    // Both laydowns share the node-class counts and the c_ii self-cost band.
    int laydownType = 0;

    // Type-1 geometry, defaults per alternate-laydown.txt: bands 240 wide
    // stepped 160 apart (A [0,240], B [160,400], C [320,560]), y in
    // [100, 200], +/-5% directional jitter.
    double bandXWidth = 240.0;
    double bandXStep = 160.0;
    double bandYLo = 100.0, bandYHi = 200.0;
    double jitterHalfWidth = 0.05;
  };

  // Throws std::invalid_argument on a non-realizable profile (no nodes, no
  // source or no sink, inverted or non-positive ranges).
  void validateProfile(const InstanceProfile& profile);

  // Deterministic pseudo-random instance for the given profile and seed. Node
  // order is by class: [supply-only | both | demand-only]. The result has
  // tonMileLimit == 0.0 (calibration is the greedy planner's job, task B2) and
  // has passed validateInstance. NOTE: the stream of std::uniform_real_
  // distribution values is implementation-defined, so instances for one seed
  // match within a platform but not necessarily across MSVC vs libstdc++.
  Instance makeRandomInstance(const InstanceProfile& profile, std::uint64_t seed);

} // namespace VINCP::Network

#endif // VINCP_NETWORK_INSTANCE_HPP
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------

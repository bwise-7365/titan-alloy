// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
// Fleet-planning problem instance: asset and vehicle types, per-(node, asset)
// supply/demand/priority, the shared distance matrix, and the random fleet
// instance generator (see network/doc/fleet-formulation.md).
// ----------------------------------------------
#ifndef VINCP_NETWORK_FLEETINSTANCE_HPP
#define VINCP_NETWORK_FLEETINSTANCE_HPP

#include "instance.hpp"

namespace VINCP::Network {

  // ---------------------------------------------------------------------------
  // Asset and vehicle types
  // ---------------------------------------------------------------------------

  // One asset type a: the weight and cargo area of ONE unit. Both non-negative
  // and finite with unitWeight + unitArea > 0 (fleet-formulation.md G-F1: a
  // weightless AND arealess asset would occupy no vehicle at all).
  struct AssetType {
    string name;
    double unitWeight = 0.0;   // w_a (tons/unit)
    double unitArea = 0.0;     // s_a (sqft/unit)
  };

  // One vehicle type k. weightCap/areaCap non-negative and finite with
  // weightCap + areaCap > 0 (a vehicle carrying neither is useless); pure
  // carriers (one capacity zero) are legal (G-F1). count is REAL, not integer:
  // fractional fleets express partial availability over the horizon.
  struct VehicleType {
    string name;
    double weightCap = 0.0;    // T_k (tons/vehicle)
    double areaCap = 0.0;      // A_k (sqft/vehicle)
    double count = 0.0;        // N_k >= 0 (vehicles, fractional OK)
    double speedMph = 0.0;     // v_k > 0 (miles/hour)
  };

  // ---------------------------------------------------------------------------
  // FleetInstance
  // ---------------------------------------------------------------------------

  // One fleet-planning problem (fleet-formulation.md sections 1-4). The base
  // model's scalar node data become one column per asset type; the scalar
  // ton-mile budget becomes per-type vehicle-mile budgets B_k = N_k v_k H,
  // DATA rather than a calibrated limit. distance(i, j) = d_ij > 0 is the
  // miles of the directed link i -> j, shared by all vehicle types, including
  // the diagonal self-supply distance d_ii (small but positive: using own
  // capacity for own demand is cheap, not free -- G-F2).
  struct FleetInstance {
    Index numNodes = 0;
    vector<AssetType> assets;       // size A >= 1
    vector<VehicleType> vehicles;   // size K >= 1
    MatrixXd supplyCap;    // C_ia >= 0              (numNodes x A, units)
    MatrixXd demand;       // D_ia >= 0              (numNodes x A, units)
    MatrixXd priority;     // P_ia > 0 where D > 0   (numNodes x A)
    MatrixXd distance;     // d_ij > 0               (numNodes x numNodes, miles)
    double horizonHours = 0.0;      // H > 0
    // Display geometry and labels, exactly as Instance: optional, validated
    // only when present. Populated by makeRandomFleetInstance.
    VectorXd xCoord;       // x_i (size numNodes, or 0 if not placed)
    VectorXd yCoord;       // y_i (size numNodes, or 0 if not placed)
    vector<string> labels; // size numNodes, or empty if not labelled
  };

  // Throws std::invalid_argument on any structural defect: mismatched shapes,
  // empty asset/vehicle lists, an asset with unitWeight + unitArea == 0, a
  // vehicle with weightCap + areaCap == 0 or non-positive speed or negative
  // count, negative C or D, non-positive P at a demand cell, a non-positive
  // distance entry, or a non-positive horizon.
  void validateFleetInstance(const FleetInstance& inst);

  // Sizes as Index, for loop headers.
  Index numAssets(const FleetInstance& inst);
  Index numVehicleTypes(const FleetInstance& inst);

  // The vehicle-miles budget of type k over the horizon: B_k = N_k v_k H
  // (fleet-formulation.md section 1).
  double vehicleBudget(const FleetInstance& inst, Index k);

  // Nodes with C_ia > 0 (sources of asset a) and with D_ia > 0 (sinks of
  // asset a), in index order.
  vector<Index> fleetSourceNodes(const FleetInstance& inst, Index asset);
  vector<Index> fleetSinkNodes(const FleetInstance& inst, Index asset);

  // Column sums of C / of D for one asset.
  double totalFleetSupplyCap(const FleetInstance& inst, Index asset);
  double totalFleetDemand(const FleetInstance& inst, Index asset);

  // Units of asset a that one vehicle of type k can carry: the binding of
  // the weight and area ratios, 0 when a needed capacity is absent
  // (fleet-formulation.md G-F5). Type validation guarantees w + s > 0, so at
  // least one ratio is present and the result is finite. Shared by the
  // greedy planner's transport step and the fleet swap reallocation.
  double unitCapacity(const AssetType& asset, const VehicleType& vehicle);

  // ---------------------------------------------------------------------------
  // Random fleet instance generator
  // ---------------------------------------------------------------------------

  // Generation profile. Node PLACEMENT (classes, coordinates, labels) reuses
  // InstanceProfile verbatim, but the fleet DISTANCE matrix is its own model
  // (G7, user decision 2026-07-07): the BARE Euclidean separation of the
  // placed coordinates times an independent per-direction multiplier
  // U[1, 1 + distanceJitterMax] -- no handling floor, no per-mile scale --
  // with self-distances d_ii drawn from [selfDistanceLo, selfDistanceHi]
  // (small but positive: cheap, not free). The base generator's cost matrix
  // is NOT used.
  //
  // The default assets and vehicles are engineered so BOTH link constraint
  // families bind somewhere: a truck is weight-bound on "dense" (20/1.0 = 20
  // units/vehicle vs 800/2.0 = 400 by area) and area-bound on "bulky"
  // (800/10 = 80 by area vs 20/0.05 = 400 by weight).
  struct FleetProfile {
    InstanceProfile geometry;   // node counts, laydown, arc-length model

    vector<AssetType> assets = {
        {"dense", 1.0, 2.0},        // heavy per area: weight tends to bind
        {"bulky", 0.05, 10.0},      // light per area: area tends to bind
    };
    vector<VehicleType> vehicles = {
        {"truck", 20.0, 800.0, 40.0, 45.0},
        {"airlift", 60.0, 4000.0, 3.0, 350.0},
    };
    double horizonHours = 72.0;

    double supplyLo = 200.0, supplyHi = 1000.0;   // C_ia range (units)
    double demandLo = 200.0, demandHi = 1000.0;   // D_ia range (units)
    double priorityLo = 1.0, priorityHi = 10.0;   // P_ia range

    // Probability that a class-eligible (node, asset) cell is nonzero, so
    // asset sparsity varies across nodes. Each asset is guaranteed at least
    // one supply cell and one demand cell (forced deterministically to the
    // range midpoint if the draws leave a column empty).
    double assetPresence = 0.8;

    // Distance model (G7): per-direction multiplier U[1, 1 + jitterMax] on
    // the bare Euclidean separation (so d_ij / d_ji stays within a few
    // percent of 1), and the diagonal self-distance band (miles).
    double distanceJitterMax = 0.05;
    double selfDistanceLo = 1.0, selfDistanceHi = 5.0;
  };

  // Throws std::invalid_argument on a non-realizable profile (empty asset or
  // vehicle lists, invalid asset/vehicle fields, inverted or non-positive
  // ranges, presence outside [0, 1], non-positive horizon). The geometry
  // sub-profile is validated by makeRandomInstance.
  void validateFleetProfile(const FleetProfile& profile);

  // Deterministic pseudo-random fleet instance. The placement (coordinates,
  // labels) comes from makeRandomInstance(profile.geometry, seed); the
  // per-(node, asset) draws AND the distance jitter use an INDEPENDENT
  // generator stream (seed xor a fixed tag), so changes to the base
  // generator never reshuffle the fleet draws and vice versa. Distances are
  // the G7 model above, drawn AFTER the C/D/P draws so the supply/demand
  // pattern of a given seed is independent of the distance model. Node
  // classes are the base profile's contiguous blocks: [supply-only | both |
  // demand-only | transit]. The result has passed validateFleetInstance.
  // The instance.hpp caveat on cross-platform std::uniform_real_distribution
  // streams applies.
  FleetInstance makeRandomFleetInstance(const FleetProfile& profile,
                                        std::uint64_t seed);

  // ---------------------------------------------------------------------------
  // Type catalogs
  // ---------------------------------------------------------------------------

  // The first `count` (1..10) entries of a FIXED catalog of asset / vehicle
  // types spanning the dense-vs-bulky and heavy-vs-fast spectrum. The first
  // two entries of each are the FleetProfile defaults (dense/bulky,
  // truck/airlift), so small counts keep both link-constraint families
  // interesting, and a given (seed, count) pair always produces the same
  // fleet instance. Throws std::invalid_argument outside [1, 10].
  vector<AssetType> assetCatalog(Index count);
  vector<VehicleType> vehicleCatalog(Index count);

} // namespace VINCP::Network

#endif // VINCP_NETWORK_FLEETINSTANCE_HPP
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------

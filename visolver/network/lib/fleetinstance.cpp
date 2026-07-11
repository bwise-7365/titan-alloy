// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
// Fleet-planning instance validation and random generation.
// ----------------------------------------------
#include "fleetinstance.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <random>
#include <stdexcept>
#include <string>

using std::string;

namespace VIMCP::Network {

  namespace {

    void
    require(bool okP, const string& message)
    {
      if (!okP) {
        throw std::invalid_argument("Network::FleetInstance: " + message);
      }
      return;
    }

    void
    requireAssetType(const AssetType& asset)
    {
      require(std::isfinite(asset.unitWeight) && 0.0 <= asset.unitWeight,
              "asset unitWeight must be finite and non-negative.");
      require(std::isfinite(asset.unitArea) && 0.0 <= asset.unitArea,
              "asset unitArea must be finite and non-negative.");
      require(0.0 < asset.unitWeight + asset.unitArea,
              "asset must have positive weight or area (G-F1).");
      return;
    }

    void
    requireVehicleType(const VehicleType& vehicle)
    {
      require(std::isfinite(vehicle.weightCap) && 0.0 <= vehicle.weightCap,
              "vehicle weightCap must be finite and non-negative.");
      require(std::isfinite(vehicle.areaCap) && 0.0 <= vehicle.areaCap,
              "vehicle areaCap must be finite and non-negative.");
      require(0.0 < vehicle.weightCap + vehicle.areaCap,
              "vehicle must have positive weight or area capacity (G-F1).");
      require(std::isfinite(vehicle.count) && 0.0 <= vehicle.count,
              "vehicle count must be finite and non-negative.");
      require(std::isfinite(vehicle.speedMph) && 0.0 < vehicle.speedMph,
              "vehicle speedMph must be finite and positive.");
      return;
    }

    // Distinct stream tag ("FleetNet" in ASCII) so the per-(node, asset)
    // draws are independent of the base geometry stream.
    const std::uint64_t kFleetStreamTag = 0x466C6565744E6574ull;

  } // namespace

  void
  validateFleetInstance(const FleetInstance& inst)
  {
    const Index m = inst.numNodes;
    require(0 < m, "numNodes must be positive.");
    require(!inst.assets.empty(), "at least one asset type is required.");
    require(!inst.vehicles.empty(), "at least one vehicle type is required.");
    for (const AssetType& asset : inst.assets) {
      requireAssetType(asset);
    }
    for (const VehicleType& vehicle : inst.vehicles) {
      requireVehicleType(vehicle);
    }

    const Index numA = numAssets(inst);
    require(inst.supplyCap.rows() == m && inst.supplyCap.cols() == numA,
            "supplyCap must be numNodes x numAssets.");
    require(inst.demand.rows() == m && inst.demand.cols() == numA,
            "demand must be numNodes x numAssets.");
    require(inst.priority.rows() == m && inst.priority.cols() == numA,
            "priority must be numNodes x numAssets.");
    require(inst.distance.rows() == m && inst.distance.cols() == m,
            "distance must be numNodes x numNodes.");
    require(std::isfinite(inst.horizonHours) && 0.0 < inst.horizonHours,
            "horizonHours must be finite and positive.");

    // Coordinates and labels are optional display metadata, as on Instance.
    const bool hasCoordsP = 0 != inst.xCoord.size() || 0 != inst.yCoord.size();
    if (hasCoordsP) {
      require(inst.xCoord.size() == m && inst.yCoord.size() == m,
              "xCoord/yCoord, when present, must both have size numNodes.");
      require(inst.xCoord.allFinite() && inst.yCoord.allFinite(),
              "coordinate entries must be finite.");
    }
    if (!inst.labels.empty()) {
      require(static_cast<Index>(inst.labels.size()) == m,
              "labels, when present, must have size numNodes.");
    }

    for (Index i = 0; i < m; ++i) {
      for (Index a = 0; a < numA; ++a) {
        require(std::isfinite(inst.supplyCap(i, a))
                    && 0.0 <= inst.supplyCap(i, a),
                "supplyCap entries must be finite and non-negative.");
        require(std::isfinite(inst.demand(i, a)) && 0.0 <= inst.demand(i, a),
                "demand entries must be finite and non-negative.");
        require(std::isfinite(inst.priority(i, a)),
                "priority entries must be finite.");
        if (0.0 < inst.demand(i, a)) {
          require(0.0 < inst.priority(i, a),
                  "priority must be positive at demand cells.");
        }
      }
      for (Index j = 0; j < m; ++j) {
        require(std::isfinite(inst.distance(i, j))
                    && 0.0 < inst.distance(i, j),
                "distance entries must be finite and positive.");
      }
    }
    return;
  }

  Index
  numAssets(const FleetInstance& inst)
  {
    return static_cast<Index>(inst.assets.size());
  }

  Index
  numVehicleTypes(const FleetInstance& inst)
  {
    return static_cast<Index>(inst.vehicles.size());
  }

  double
  vehicleBudget(const FleetInstance& inst, Index k)
  {
    if (k < 0 || numVehicleTypes(inst) <= k) {
      throw std::invalid_argument(
          "Network::vehicleBudget: vehicle type index out of range.");
    }
    const VehicleType& vehicle = inst.vehicles[static_cast<size_t>(k)];
    return vehicle.count * vehicle.speedMph * inst.horizonHours;
  }

  vector<Index>
  fleetSourceNodes(const FleetInstance& inst, Index asset)
  {
    vector<Index> nodes;
    for (Index i = 0; i < inst.numNodes; ++i) {
      if (0.0 < inst.supplyCap(i, asset)) {
        nodes.push_back(i);
      }
    }
    return nodes;
  }

  vector<Index>
  fleetSinkNodes(const FleetInstance& inst, Index asset)
  {
    vector<Index> nodes;
    for (Index i = 0; i < inst.numNodes; ++i) {
      if (0.0 < inst.demand(i, asset)) {
        nodes.push_back(i);
      }
    }
    return nodes;
  }

  double
  totalFleetSupplyCap(const FleetInstance& inst, Index asset)
  {
    return inst.supplyCap.col(asset).sum();
  }

  double
  totalFleetDemand(const FleetInstance& inst, Index asset)
  {
    return inst.demand.col(asset).sum();
  }

  double
  unitCapacity(const AssetType& asset, const VehicleType& vehicle)
  {
    double cap = std::numeric_limits<double>::infinity();
    if (0.0 < asset.unitWeight) {
      if (0.0 >= vehicle.weightCap) {
        return 0.0;
      }
      cap = std::min(cap, vehicle.weightCap / asset.unitWeight);
    }
    if (0.0 < asset.unitArea) {
      if (0.0 >= vehicle.areaCap) {
        return 0.0;
      }
      cap = std::min(cap, vehicle.areaCap / asset.unitArea);
    }
    return cap;
  }

  void
  validateFleetProfile(const FleetProfile& profile)
  {
    require(!profile.assets.empty(),
            "profile must include at least one asset type.");
    require(!profile.vehicles.empty(),
            "profile must include at least one vehicle type.");
    for (const AssetType& asset : profile.assets) {
      requireAssetType(asset);
    }
    for (const VehicleType& vehicle : profile.vehicles) {
      requireVehicleType(vehicle);
    }
    require(std::isfinite(profile.horizonHours) && 0.0 < profile.horizonHours,
            "profile horizonHours must be finite and positive.");
    require(0.0 < profile.supplyLo && profile.supplyLo <= profile.supplyHi,
            "profile supply range must be positive and ordered.");
    require(0.0 < profile.demandLo && profile.demandLo <= profile.demandHi,
            "profile demand range must be positive and ordered.");
    require(0.0 < profile.priorityLo
                && profile.priorityLo <= profile.priorityHi,
            "profile priority range must be positive and ordered.");
    require(0.0 <= profile.assetPresence && profile.assetPresence <= 1.0,
            "profile assetPresence must lie in [0, 1].");
    require(std::isfinite(profile.distanceJitterMax)
                && 0.0 <= profile.distanceJitterMax,
            "profile distanceJitterMax must be finite and non-negative.");
    require(0.0 < profile.selfDistanceLo
                && profile.selfDistanceLo <= profile.selfDistanceHi,
            "profile self-distance range must be positive and ordered.");
    return;
  }

  FleetInstance
  makeRandomFleetInstance(const FleetProfile& profile, std::uint64_t seed)
  {
    validateFleetProfile(profile);

    // Placement (coordinates, labels) from the base generator; its tonnage
    // draws AND its cost matrix are discarded -- fleet distances are the G7
    // bare-Euclidean model built below from the coordinates.
    const Instance base = makeRandomInstance(profile.geometry, seed);
    const Index m = base.numNodes;
    const Index numA = static_cast<Index>(profile.assets.size());

    FleetInstance inst;
    inst.numNodes = m;
    inst.assets = profile.assets;
    inst.vehicles = profile.vehicles;
    inst.horizonHours = profile.horizonHours;
    inst.xCoord = base.xCoord;
    inst.yCoord = base.yCoord;
    inst.labels = base.labels;
    inst.supplyCap = MatrixXd::Zero(m, numA);
    inst.demand = MatrixXd::Zero(m, numA);
    inst.priority = MatrixXd::Zero(m, numA);

    // Independent stream for the fleet draws (see header): the base geometry
    // stream above and this one never interleave.
    std::mt19937_64 rng(seed ^ kFleetStreamTag);
    std::uniform_real_distribution<double> supplyDist(profile.supplyLo,
                                                      profile.supplyHi);
    std::uniform_real_distribution<double> demandDist(profile.demandLo,
                                                      profile.demandHi);
    std::uniform_real_distribution<double> priorityDist(profile.priorityLo,
                                                        profile.priorityHi);
    std::uniform_real_distribution<double> presenceDist(0.0, 1.0);

    // Node classes are the base profile's contiguous blocks (instance.cpp):
    // [supply-only | both | demand-only | transit].
    const InstanceProfile& geometry = profile.geometry;
    const Index numClassed = geometry.numSupplyOnly + geometry.numBoth
                             + geometry.numDemandOnly;

    // Fixed draw order (priorities, then supply, then demand; row-major over
    // cells) keeps one seed's stream reproducible.
    for (Index i = 0; i < m; ++i) {
      for (Index a = 0; a < numA; ++a) {
        inst.priority(i, a) = priorityDist(rng);
      }
    }
    for (Index i = 0; i < m; ++i) {
      const bool suppliesP = i < geometry.numSupplyOnly + geometry.numBoth;
      if (!suppliesP) {
        continue;
      }
      for (Index a = 0; a < numA; ++a) {
        if (presenceDist(rng) <= profile.assetPresence) {
          inst.supplyCap(i, a) = supplyDist(rng);
        }
      }
    }
    for (Index i = 0; i < m; ++i) {
      const bool demandsP = geometry.numSupplyOnly <= i && i < numClassed;
      if (!demandsP) {
        continue;
      }
      for (Index a = 0; a < numA; ++a) {
        if (presenceDist(rng) <= profile.assetPresence) {
          inst.demand(i, a) = demandDist(rng);
        }
      }
    }

    // Distances (G7, user decision 2026-07-07): bare Euclidean separation of
    // the placed coordinates times an independent per-direction multiplier
    // U[1, 1 + distanceJitterMax] -- so d_ij / d_ji stays within a few
    // percent of 1 -- with self-distances d_ii ~ U[selfDistanceLo, Hi]
    // (cheap, not free). kMinSeparationMiles guards the (measure-zero) case
    // of coincident placements, which would otherwise produce a non-positive
    // distance and fail validation. Drawn AFTER the C/D/P draws so the
    // supply/demand pattern of a seed is independent of the distance model.
    const double kMinSeparationMiles = 1.0;
    std::uniform_real_distribution<double> jitterDist(
        1.0, 1.0 + profile.distanceJitterMax);
    std::uniform_real_distribution<double> selfDistanceDist(
        profile.selfDistanceLo, profile.selfDistanceHi);
    inst.distance = MatrixXd::Zero(m, m);
    for (Index i = 0; i < m; ++i) {
      for (Index j = 0; j < m; ++j) {
        if (i == j) {
          inst.distance(i, i) = selfDistanceDist(rng);
        }
        else {
          const double separation = std::hypot(base.xCoord(i) - base.xCoord(j),
                                               base.yCoord(i) - base.yCoord(j));
          inst.distance(i, j) =
              std::max(separation, kMinSeparationMiles) * jitterDist(rng);
        }
      }
    }

    // Guarantee every asset has at least one supply cell and one demand cell:
    // force the first eligible node to the range midpoint, deterministically
    // (no further draws, so the stream above is untouched).
    const Index firstSupply = 0;                       // supply-only block head
    const Index firstDemand = geometry.numSupplyOnly;  // both/demand block head
    for (Index a = 0; a < numA; ++a) {
      if (0.0 == inst.supplyCap.col(a).sum()) {
        inst.supplyCap(firstSupply, a) =
            0.5 * (profile.supplyLo + profile.supplyHi);
      }
      if (0.0 == inst.demand.col(a).sum()) {
        inst.demand(firstDemand, a) =
            0.5 * (profile.demandLo + profile.demandHi);
      }
    }

    validateFleetInstance(inst);
    return inst;
  }

  vector<AssetType>
  assetCatalog(Index count)
  {
    // Ordered so every prefix mixes weight-bound and area-bound assets: the
    // FleetProfile defaults first, then intermediates, then extremes.
    static const vector<AssetType> kCatalog = {
        {"dense", 1.0, 2.0},        // weight-bound on a truck
        {"bulky", 0.05, 10.0},      // area-bound on a truck
        {"rations", 0.8, 4.0},
        {"parts", 0.4, 8.0},
        {"ammo", 2.0, 1.0},         // extreme dense
        {"tents", 0.2, 15.0},
        {"medical", 0.5, 5.0},
        {"drums", 1.5, 12.0},       // heavy AND bulky
        {"foam", 0.02, 30.0},       // extreme bulky
        {"crates", 0.3, 20.0},
    };
    require(1 <= count && count <= static_cast<Index>(kCatalog.size()),
            "assetCatalog count must lie in [1, 10].");
    return {kCatalog.begin(), kCatalog.begin() + count};
  }

  vector<VehicleType>
  vehicleCatalog(Index count)
  {
    // Same prefix idea: slow/heavy and fast/scarce first, then the spread.
    static const vector<VehicleType> kCatalog = {
        {"truck", 20.0, 800.0, 40.0, 45.0},
        {"airlift", 60.0, 4000.0, 3.0, 350.0},
        {"van", 5.0, 300.0, 60.0, 55.0},
        {"semi", 30.0, 1500.0, 25.0, 55.0},
        {"helo", 10.0, 600.0, 6.0, 150.0},
        {"flatbed", 25.0, 1000.0, 20.0, 50.0},
        {"pickup", 2.0, 100.0, 100.0, 60.0},
        {"cargoplane", 90.0, 6000.0, 2.0, 480.0},
        {"barge", 200.0, 12000.0, 1.5, 8.0},
        {"drone", 0.2, 20.0, 200.0, 80.0},
    };
    require(1 <= count && count <= static_cast<Index>(kCatalog.size()),
            "vehicleCatalog count must lie in [1, 10].");
    return {kCatalog.begin(), kCatalog.begin() + count};
  }

} // namespace VIMCP::Network
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------

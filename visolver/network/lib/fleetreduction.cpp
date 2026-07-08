// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
// Fleet preprocessing implementation: distance-as-cost shortest routes,
// per-asset reduced problems over round-trip mileage, capability matrix.
// ----------------------------------------------
#include "fleetreduction.hpp"

#include <algorithm>
#include <stdexcept>

namespace VINCP::Network {

  namespace {

    // A geometry-only single-commodity view of the fleet instance: the
    // distance matrix as the cost matrix, node data zeroed. Valid by
    // construction (distances are validated positive), and enough for
    // computeShortestRoutes, which reads only the cost matrix.
    Instance
    distanceAsInstance(const FleetInstance& fleet)
    {
      Instance inst;
      inst.numNodes = fleet.numNodes;
      inst.supplyCap = VectorXd::Zero(fleet.numNodes);
      inst.demand = VectorXd::Zero(fleet.numNodes);
      inst.priority = VectorXd::Zero(fleet.numNodes);
      inst.cost = fleet.distance;
      return inst;
    }

    // The single-commodity view of one asset column over the same geometry;
    // only sourceNodes/sinkNodes of it are consulted by makeReducedProblem.
    Instance
    assetColumnInstance(const FleetInstance& fleet, Index asset)
    {
      Instance inst = distanceAsInstance(fleet);
      inst.supplyCap = fleet.supplyCap.col(asset);
      inst.demand = fleet.demand.col(asset);
      inst.priority = fleet.priority.col(asset);
      return inst;
    }

  } // namespace

  FleetReducedProblem
  makeFleetReducedProblem(const FleetInstance& inst,
                          const ScreenParams& screen)
  {
    validateFleetInstance(inst);
    const Index numA = numAssets(inst);
    const Index numK = numVehicleTypes(inst);

    FleetReducedProblem reduced;
    reduced.routes = computeShortestRoutes(distanceAsInstance(inst));

    // Capability matrix (G-F5); every demand-bearing asset needs a capable
    // type or its cells could never be served by ANY plan.
    reduced.kappa = MatrixXd(numA, numK);
    for (Index a = 0; a < numA; ++a) {
      double bestKappa = 0.0;
      for (Index k = 0; k < numK; ++k) {
        reduced.kappa(a, k) = unitCapacity(inst.assets[static_cast<size_t>(a)],
                                           inst.vehicles[static_cast<size_t>(k)]);
        bestKappa = std::max(bestKappa, reduced.kappa(a, k));
      }
      if (0.0 < totalFleetDemand(inst, a) && 0.0 >= bestKappa) {
        throw std::invalid_argument(
            "makeFleetReducedProblem: an asset with demand has no capable "
            "vehicle type.");
      }
    }

    // Per-asset reduction: REUSE makeReducedProblem verbatim, feeding it a
    // routes copy whose off-diagonal distances are the round trips
    // d-hat(i, j) + d-hat(j, i) (the diagonal stays zero, and selfDistance
    // is already the >= 1-arc loop cost, i.e. its own round trip). Only
    // .distance and .selfDistance are read by makeReducedProblem, so the
    // successor matrix of the copy being one-way is harmless.
    ShortestRoutes roundTripRoutes = reduced.routes;
    roundTripRoutes.distance =
        reduced.routes.distance + reduced.routes.distance.transpose();

    reduced.perAsset.reserve(static_cast<size_t>(numA));
    for (Index a = 0; a < numA; ++a) {
      const Instance column = assetColumnInstance(inst, a);
      if (0.0 < totalFleetDemand(inst, a)
          && 0.0 >= totalFleetSupplyCap(inst, a)) {
        throw std::invalid_argument(
            "makeFleetReducedProblem: an asset with demand has no supply.");
      }
      reduced.perAsset.push_back(
          makeReducedProblem(column, roundTripRoutes, screen));
    }
    return reduced;
  }

} // namespace VINCP::Network
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------

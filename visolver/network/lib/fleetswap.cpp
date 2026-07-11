// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
// Fleet swap improvement: per-asset 2-exchange local optima via the
// single-commodity swap engine, then vehicle reallocation.
// ----------------------------------------------
#include "fleetswap.hpp"

#include <algorithm>
#include <numeric>
#include <stdexcept>
#include <vector>

namespace VIMCP::Network {

  namespace {

    // The single-commodity view of one asset column, priced by ROUND TRIPS:
    // cost(i, j) = d_ij + d_ji off the diagonal (the deadhead return leg is
    // charged), d_ii on it (a diagonal traversal closes its own loop, G-F2).
    Instance
    roundTripSlice(const FleetInstance& fleet, Index asset)
    {
      const Index m = fleet.numNodes;
      Instance inst;
      inst.numNodes = m;
      inst.supplyCap = fleet.supplyCap.col(asset);
      inst.demand = fleet.demand.col(asset);
      inst.priority = fleet.priority.col(asset);
      inst.cost = fleet.distance + fleet.distance.transpose();
      inst.cost.diagonal() = fleet.distance.diagonal();
      return inst;
    }

    void
    requireShapes(const FleetInstance& inst, const FleetPlan& plan)
    {
      const Index m = inst.numNodes;
      const Index numA = numAssets(inst);
      const Index numK = numVehicleTypes(inst);
      bool okP = plan.supplied.rows() == m && plan.supplied.cols() == numA
                 && plan.resupply.rows() == m && plan.resupply.cols() == numA
                 && static_cast<Index>(plan.flow.size()) == numA
                 && static_cast<Index>(plan.vehicles.size()) == numK;
      if (okP) {
        for (const MatrixXd& x : plan.flow) {
          okP = okP && x.rows() == m && x.cols() == m;
        }
        for (const MatrixXd& u : plan.vehicles) {
          okP = okP && u.rows() == m && u.cols() == m;
        }
      }
      if (!okP) {
        throw std::invalid_argument(
            "Network::swapFleetToLocalOptimum: plan shapes do not match the "
            "instance.");
      }
      return;
    }

    // Rebuild plan.vehicles from scratch to carry plan.flow with the greedy
    // planner's transport rule (fleetgreedy.cpp step (4)): per shipment,
    // spill across types best unitCapacity first within the per-type
    // budgets, exact-zero assignment of drained resources, out-and-back
    // legs (FL2). Returns the per-type miles. Throws std::runtime_error if
    // the (order-dependent, greedy) spill cannot place some cargo -- see
    // the swapFleetToLocalOptimum contract in the header.
    VectorXd
    reallocateVehicles(const FleetInstance& inst, FleetPlan& plan)
    {
      const Index m = inst.numNodes;
      const Index numA = numAssets(inst);
      const Index numK = numVehicleTypes(inst);

      MatrixXd kappa(numA, numK);
      for (Index a = 0; a < numA; ++a) {
        for (Index k = 0; k < numK; ++k) {
          kappa(a, k) = unitCapacity(inst.assets[static_cast<size_t>(a)],
                                     inst.vehicles[static_cast<size_t>(k)]);
        }
      }
      vector<vector<Index>> typeOrder(static_cast<size_t>(numA));
      for (Index a = 0; a < numA; ++a) {
        vector<Index>& order = typeOrder[static_cast<size_t>(a)];
        order.resize(static_cast<size_t>(numK));
        std::iota(order.begin(), order.end(), Index{0});
        std::stable_sort(order.begin(), order.end(),
                         [&kappa, a](Index lhs, Index rhs) {
                           return kappa(a, lhs) > kappa(a, rhs);
                         });
      }

      VectorXd remBudget(numK);
      for (Index k = 0; k < numK; ++k) {
        remBudget(k) = vehicleBudget(inst, k);
      }
      VectorXd milesUsed = VectorXd::Zero(numK);
      plan.vehicles.assign(static_cast<size_t>(numK), MatrixXd::Zero(m, m));

      for (Index a = 0; a < numA; ++a) {
        const MatrixXd& x = plan.flow[static_cast<size_t>(a)];
        const vector<Index>& order = typeOrder[static_cast<size_t>(a)];
        for (Index i = 0; i < m; ++i) {
          for (Index j = 0; j < m; ++j) {
            if (0.0 >= x(i, j)) {
              continue;
            }
            const double trip =
                (i == j) ? inst.distance(i, i)
                         : inst.distance(i, j) + inst.distance(j, i);
            double remaining = x(i, j);
            for (const Index k : order) {
              if (0.0 >= remBudget(k) || 0.0 >= kappa(a, k)) {
                continue;
              }
              const double avail = (remBudget(k) / trip) * kappa(a, k);
              const double units = std::min(remaining, avail);
              if (0.0 >= units) {
                continue;
              }
              const double vehicleCount = units / kappa(a, k);
              MatrixXd& u = plan.vehicles[static_cast<size_t>(k)];
              u(i, j) += vehicleCount;
              if (i != j) {
                u(j, i) += vehicleCount;    // deadhead return leg (FL2)
              }
              milesUsed(k) += vehicleCount * trip;
              if (units == avail) {
                remBudget(k) = 0.0;         // drained EXACTLY
              }
              else {
                remBudget(k) -= vehicleCount * trip;
              }
              remaining -= units;           // exact 0 when units == remaining
              if (0.0 >= remaining) {
                break;
              }
            }
            if (0.0 < remaining) {
              throw std::runtime_error(
                  "fleet vehicle reallocation exceeded the budgets.");
            }
          }
        }
      }
      return milesUsed;
    }

  } // namespace

  FleetSwapSummary
  swapFleetToLocalOptimum(const FleetInstance& inst, FleetPlan& plan,
                          int maxSwapsPerAsset)
  {
    validateFleetInstance(inst);
    requireShapes(inst, plan);
    const Index numA = numAssets(inst);
    const Index numK = numVehicleTypes(inst);

    FleetSwapSummary summary;
    summary.swapsPerAsset.assign(static_cast<size_t>(numA), 0);
    summary.milesUsedBefore = VectorXd(numK);
    for (Index k = 0; k < numK; ++k) {
      summary.milesUsedBefore(k) = vehicleMiles(inst, plan, k);
    }

    // Per asset: the EXISTING swap engine on the round-trip slice, verbatim.
    // supplied/resupply are invariant under the 2-exchange, so only the flow
    // matrix is written back.
    for (Index a = 0; a < numA; ++a) {
      const Instance slice = roundTripSlice(inst, a);
      Plan slicePlan;
      slicePlan.supplied = plan.supplied.col(a);
      slicePlan.resupply = plan.resupply.col(a);
      slicePlan.flow = plan.flow[static_cast<size_t>(a)];
      const SwapSummary swapped =
          swapToLocalOptimum(slice, slicePlan, maxSwapsPerAsset);
      summary.swapsPerAsset[static_cast<size_t>(a)] = swapped.swaps;
      summary.totalSwaps += swapped.swaps;
      summary.unitMileSaving += swapped.totalSaving;
      plan.flow[static_cast<size_t>(a)] = slicePlan.flow;
    }

    // Reallocate vehicles to the swapped flows. The swaps strictly reduced
    // every asset's unit round-trip-miles, so an allocation within the
    // budgets exists (reuse the original plan's per-type shares, scaled
    // down); the greedy spill missing it is an ordering pathology worth
    // surfacing, not papering over -- hence the helper's throw.
    summary.milesUsedAfter = reallocateVehicles(inst, plan);
    return summary;
  }

  FleetPurifySummary
  purifyFleetPlan(const FleetInstance& inst, FleetPlan& plan,
                  int maxSwapsPerAsset)
  {
    validateFleetInstance(inst);
    requireShapes(inst, plan);
    const Index numA = numAssets(inst);
    const Index numK = numVehicleTypes(inst);

    FleetPurifySummary summary;
    summary.arcsBeforePerAsset.assign(static_cast<size_t>(numA), 0);
    summary.arcsAfterPerAsset.assign(static_cast<size_t>(numA), 0);
    summary.milesUsedBefore = VectorXd(numK);
    for (Index k = 0; k < numK; ++k) {
      summary.milesUsedBefore(k) = vehicleMiles(inst, plan, k);
    }

    // Per asset: the single-commodity purification on the round-trip slice,
    // capped at the asset's ENTRY usage (G-F9): zero initial slack, so no
    // consolidation spends budget the incoming plan had not already spent.
    for (Index a = 0; a < numA; ++a) {
      const Instance slice = roundTripSlice(inst, a);
      Plan slicePlan;
      slicePlan.supplied = plan.supplied.col(a);
      slicePlan.resupply = plan.resupply.col(a);
      slicePlan.flow = plan.flow[static_cast<size_t>(a)];
      const double entryMiles = tonMiles(slice, slicePlan);
      const PurifySummary purified =
          purifyPlan(slice, slicePlan, entryMiles, maxSwapsPerAsset);
      summary.improvingSwaps += purified.improvingSwaps;
      summary.consolidatingSwaps += purified.consolidatingSwaps;
      summary.arcsBeforePerAsset[static_cast<size_t>(a)] = purified.arcsBefore;
      summary.arcsAfterPerAsset[static_cast<size_t>(a)] = purified.arcsAfter;
      plan.flow[static_cast<size_t>(a)] = slicePlan.flow;
    }

    summary.milesUsedAfter = reallocateVehicles(inst, plan);
    return summary;
  }

} // namespace VIMCP::Network
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------

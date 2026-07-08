// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
// Greedy fleet planner implementation: per-asset water-filling rationing,
// then greedy vehicle-borne flow construction under per-type budgets.
// ----------------------------------------------
#include "fleetgreedy.hpp"

#include <algorithm>
#include <limits>
#include <numeric>
#include <stdexcept>

namespace VINCP::Network {

  namespace {

    const double kInfinity = std::numeric_limits<double>::infinity();

    void
    validateFleetGreedyParams(const FleetGreedyParams& params)
    {
      if (!(0.0 < params.demandScaleDown && params.demandScaleDown < 1.0)) {
        throw std::invalid_argument(
            "greedyFleetPlan: demandScaleDown must lie in (0, 1).");
      }
      return;
    }

    // One phase-2 run over given per-type budget levels. Shared by the real
    // pass and the unlimited-budget pass behind fleetScaleHint (G-F4).
    struct LoopOutcome {
      vector<MatrixXd> flow;      // x^a, one matrix per asset
      vector<MatrixXd> vehicles;  // u^k, one matrix per type
      VectorXd milesUsed;         // accumulated d * u per type
      int iterations = 0;
    };

    LoopOutcome
    runGreedyFleetLoop(const FleetInstance& inst, const MatrixXd& scaledTargets,
                       VectorXd remBudget)
    {
      const Index m = inst.numNodes;
      const Index numA = numAssets(inst);
      const Index numK = numVehicleTypes(inst);

      // Per-(asset, type) unit capacities and, per asset, the type order:
      // best capacity first (units per vehicle and, since the round trip is
      // common to all types, units per budget-mile), ties broken by lower
      // type index for determinism (G-F6).
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

      LoopOutcome outcome;
      outcome.flow.assign(static_cast<size_t>(numA), MatrixXd::Zero(m, m));
      outcome.vehicles.assign(static_cast<size_t>(numK), MatrixXd::Zero(m, m));
      outcome.milesUsed = VectorXd::Zero(numK);

      MatrixXd remTarget = scaledTargets;
      MatrixXd remCap = inst.supplyCap;
      MatrixXd unservable = MatrixXd::Zero(m, numA);   // 1.0 = given up

      // Termination (fleet-formulation.md section 8): every pass zeroes a
      // target cell, zeroes a capacity cell, drains the last capable
      // positive budget, or marks a cell unservable. The explicit exact-zero
      // ASSIGNMENTS below (never a subtraction past the bound) are what make
      // the count sound in floating point.
      const int iterCap = static_cast<int>(3 * m * numA + numK + 2);
      while (true) {
        // (1) Selection: largest remaining priority-weighted fractional
        // shortfall P (remTarget/D)^2 -- the objective decrease available
        // from fully serving the cell (G-F6). remTarget > 0 implies D > 0.
        Index cellNode = -1;
        Index cellAsset = -1;
        double bestScore = 0.0;
        for (Index n = 0; n < m; ++n) {
          for (Index a = 0; a < numA; ++a) {
            if (0.0 >= remTarget(n, a) || 0.0 != unservable(n, a)) {
              continue;
            }
            const double fraction = remTarget(n, a) / inst.demand(n, a);
            const double score = inst.priority(n, a) * fraction * fraction;
            if (score > bestScore) {
              bestScore = score;
              cellNode = n;
              cellAsset = a;
            }
          }
        }
        if (-1 == cellNode) {
          break;                            // all cells served or given up
        }
        const Index n = cellNode;
        const Index a = cellAsset;

        // (2) Cheapest source by ROUND-TRIP miles: the deadhead return is
        // charged (G-F3), so the source ranking uses d_in + d_ni (d_ii on
        // the diagonal, which closes its own loop, G-F2).
        Index source = -1;
        double sourceTrip = kInfinity;
        for (Index i = 0; i < m; ++i) {
          if (0.0 >= remCap(i, a)) {
            continue;
          }
          const double trip = (i == n)
                                  ? inst.distance(i, i)
                                  : inst.distance(i, n) + inst.distance(n, i);
          if (trip < sourceTrip) {
            source = i;
            sourceTrip = trip;
          }
        }
        if (-1 == source) {
          // Unreachable for valid inputs: the per-asset scale-down keeps each
          // asset's remaining targets strictly below its total capacity.
          throw std::runtime_error(
              "greedyFleetPlan: no source with capacity left.");
        }

        // (3) Transport availability across capable types with budget left.
        const vector<Index>& order = typeOrder[static_cast<size_t>(a)];
        double transportMax = 0.0;
        for (const Index k : order) {
          if (0.0 < remBudget(k) && 0.0 < kappa(a, k)) {
            transportMax += (remBudget(k) / sourceTrip) * kappa(a, k);
          }
        }
        ++outcome.iterations;
        if (iterCap < outcome.iterations) {
          throw std::runtime_error(
              "greedyFleetPlan: iteration cap exceeded.");
        }
        if (0.0 >= transportMax) {
          // Budget/capability exhaustion is source-independent, so the cell
          // is unservable outright, not merely from this source.
          unservable(n, a) = 1.0;
          continue;
        }

        // (4) Ship, spilling across types best capacity first. Whichever
        // resource binds is ASSIGNED its bound exactly.
        const double quantity =
            std::min({remTarget(n, a), remCap(source, a), transportMax});
        double remaining = quantity;
        for (const Index k : order) {
          if (0.0 >= remBudget(k) || 0.0 >= kappa(a, k)) {
            continue;
          }
          const double avail = (remBudget(k) / sourceTrip) * kappa(a, k);
          const double units = std::min(remaining, avail);
          if (0.0 >= units) {
            continue;
          }
          const double vehicleCount = units / kappa(a, k);
          MatrixXd& u = outcome.vehicles[static_cast<size_t>(k)];
          u(source, n) += vehicleCount;
          if (source != n) {
            u(n, source) += vehicleCount;   // deadhead return leg (FL2)
          }
          outcome.milesUsed(k) += vehicleCount * sourceTrip;
          if (units == avail) {
            remBudget(k) = 0.0;             // drained EXACTLY
          }
          else {
            remBudget(k) -= vehicleCount * sourceTrip;
          }
          remaining -= units;               // exact 0 when units == remaining
          if (0.0 >= remaining) {
            break;
          }
        }
        const double shipped = quantity - remaining;
        outcome.flow[static_cast<size_t>(a)](source, n) += shipped;
        if (shipped == remTarget(n, a)) {
          remTarget(n, a) = 0.0;
        }
        else {
          remTarget(n, a) -= shipped;
        }
        if (shipped == remCap(source, a)) {
          remCap(source, a) = 0.0;
        }
        else {
          remCap(source, a) -= shipped;
        }
      }
      return outcome;
    }

  } // namespace

  MatrixXd
  rationFleetTargets(const FleetInstance& inst)
  {
    validateFleetInstance(inst);
    const Index numA = numAssets(inst);
    MatrixXd targets(inst.numNodes, numA);
    for (Index a = 0; a < numA; ++a) {
      const double meetable = std::min(totalFleetDemand(inst, a),
                                       totalFleetSupplyCap(inst, a));
      targets.col(a) = waterFillTargets(inst.demand.col(a),
                                        inst.priority.col(a), meetable);
    }
    return targets;
  }

  FleetGreedyResult
  greedyFleetPlan(const FleetInstance& inst, const FleetGreedyParams& params)
  {
    validateFleetInstance(inst);
    validateFleetGreedyParams(params);

    const Index numK = numVehicleTypes(inst);
    FleetGreedyResult result;
    result.targets = rationFleetTargets(inst);
    const MatrixXd scaledTargets =
        (1.0 - params.demandScaleDown) * result.targets;

    result.budget = VectorXd(numK);
    for (Index k = 0; k < numK; ++k) {
      result.budget(k) = vehicleBudget(inst, k);
    }

    // Advisory pass with unlimited budgets on the types that EXIST (B_k > 0):
    // its per-type mileage over B_k is the fleet scale this heuristic would
    // need (G-F4). Zero-budget types stay at zero -- they cannot be "scaled
    // up" from nothing.
    VectorXd unlimited(numK);
    for (Index k = 0; k < numK; ++k) {
      unlimited(k) = (0.0 < result.budget(k)) ? kInfinity : 0.0;
    }
    const LoopOutcome hintPass =
        runGreedyFleetLoop(inst, scaledTargets, unlimited);
    result.fleetScaleHint = 0.0;
    for (Index k = 0; k < numK; ++k) {
      if (0.0 < result.budget(k)) {
        result.fleetScaleHint = std::max(
            result.fleetScaleHint, hintPass.milesUsed(k) / result.budget(k));
      }
    }

    // The real pass, under the actual budgets.
    LoopOutcome real = runGreedyFleetLoop(inst, scaledTargets, result.budget);

    const Index numA = numAssets(inst);
    FleetPlan plan;
    plan.supplied = MatrixXd(inst.numNodes, numA);
    plan.resupply = MatrixXd(inst.numNodes, numA);
    for (Index a = 0; a < numA; ++a) {
      const MatrixXd& x = real.flow[static_cast<size_t>(a)];
      plan.supplied.col(a) = x.rowwise().sum();
      plan.resupply.col(a) = x.colwise().sum().transpose();
    }
    plan.flow = std::move(real.flow);
    plan.vehicles = std::move(real.vehicles);

    result.milesUsed = real.milesUsed;
    result.utilization = VectorXd::Zero(numK);
    for (Index k = 0; k < numK; ++k) {
      if (0.0 < result.budget(k)) {
        result.utilization(k) = result.milesUsed(k) / result.budget(k);
      }
    }
    result.unserved = (scaledTargets - plan.resupply).cwiseMax(0.0);
    result.shortfallValue = fleetShortfallObjective(inst, plan);
    result.iterations = real.iterations;
    result.plan = std::move(plan);
    return result;
  }

} // namespace VINCP::Network
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------

// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
// Fleet plan evaluation and feasibility checking.
// ----------------------------------------------
#include "fleetplan.hpp"

#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace VIMCP::Network {

  namespace {

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
            "Network::checkFleetPlan: plan shapes do not match the instance.");
      }
      return;
    }

  } // namespace

  FleetPlan
  makeZeroFleetPlan(const FleetInstance& inst)
  {
    validateFleetInstance(inst);
    const Index m = inst.numNodes;
    FleetPlan plan;
    plan.supplied = MatrixXd::Zero(m, numAssets(inst));
    plan.resupply = MatrixXd::Zero(m, numAssets(inst));
    plan.flow.assign(static_cast<size_t>(numAssets(inst)),
                     MatrixXd::Zero(m, m));
    plan.vehicles.assign(static_cast<size_t>(numVehicleTypes(inst)),
                         MatrixXd::Zero(m, m));
    return plan;
  }

  double
  vehicleMiles(const FleetInstance& inst, const FleetPlan& plan, Index k)
  {
    requireShapes(inst, plan);
    if (k < 0 || numVehicleTypes(inst) <= k) {
      throw std::invalid_argument(
          "Network::vehicleMiles: vehicle type index out of range.");
    }
    const MatrixXd& u = plan.vehicles[static_cast<size_t>(k)];
    return (inst.distance.array() * u.array()).sum();
  }

  double
  fleetShortfallVsTarget(const FleetInstance& inst, const MatrixXd& target,
                         const MatrixXd& resupply)
  {
    const Index m = inst.numNodes;
    const Index numA = numAssets(inst);
    const bool okP = target.rows() == m && target.cols() == numA
                     && resupply.rows() == m && resupply.cols() == numA;
    if (!okP) {
      throw std::invalid_argument(
          "Network::fleetShortfallVsTarget: target/resupply must be "
          "numNodes x numAssets.");
    }
    double objective = 0.0;
    for (Index i = 0; i < m; ++i) {
      for (Index a = 0; a < numA; ++a) {
        if (0.0 < inst.demand(i, a)) {
          const double fraction =
              (target(i, a) - resupply(i, a)) / inst.demand(i, a);
          objective += inst.priority(i, a) * fraction * fraction;
        }
      }
    }
    return objective;
  }

  double
  fleetShortfallOfResupply(const FleetInstance& inst, const MatrixXd& resupply)
  {
    return fleetShortfallVsTarget(inst, inst.demand, resupply);
  }

  double
  fleetShortfallObjective(const FleetInstance& inst, const FleetPlan& plan)
  {
    requireShapes(inst, plan);
    return fleetShortfallOfResupply(inst, plan.resupply);
  }

  FleetPlanViolations
  checkFleetPlan(const FleetInstance& inst, const FleetPlan& plan)
  {
    requireShapes(inst, plan);
    const Index m = inst.numNodes;
    const Index numA = numAssets(inst);
    const Index numK = numVehicleTypes(inst);
    FleetPlanViolations violations;

    // Per-asset families: balance, delivery, capacity, negativity of S/R/x,
    // idle resupply -- the base checkPlan loop, once per asset column.
    for (Index a = 0; a < numA; ++a) {
      const MatrixXd& x = plan.flow[static_cast<size_t>(a)];
      for (Index j = 0; j < m; ++j) {
        const double inflow = x.col(j).sum();
        const double outflow = x.row(j).sum();

        const double balanceResidual =
            inflow + plan.supplied(j, a) - plan.resupply(j, a) - outflow;
        violations.assetBalance = std::max(violations.assetBalance,
                                           std::abs(balanceResidual));
        violations.delivery = std::max(violations.delivery,
                                       plan.resupply(j, a) - inflow);
        violations.capacity = std::max(violations.capacity,
                                       plan.supplied(j, a)
                                           - inst.supplyCap(j, a));
        violations.negativity = std::max(violations.negativity,
                                         -plan.supplied(j, a));
        violations.negativity = std::max(violations.negativity,
                                         -plan.resupply(j, a));
        if (0.0 == inst.demand(j, a)) {
          violations.idleResupply = std::max(violations.idleResupply,
                                             plan.resupply(j, a));
        }
      }
      violations.negativity = std::max(violations.negativity, -x.minCoeff());
    }

    // Link weight/area: aggregate cargo vs allocated vehicle capacity, as
    // whole-matrix expressions (fleet-formulation.md section 3).
    MatrixXd cargoWeight = MatrixXd::Zero(m, m);
    MatrixXd cargoArea = MatrixXd::Zero(m, m);
    for (Index a = 0; a < numA; ++a) {
      const AssetType& asset = inst.assets[static_cast<size_t>(a)];
      cargoWeight += asset.unitWeight * plan.flow[static_cast<size_t>(a)];
      cargoArea += asset.unitArea * plan.flow[static_cast<size_t>(a)];
    }
    MatrixXd capWeight = MatrixXd::Zero(m, m);
    MatrixXd capArea = MatrixXd::Zero(m, m);
    for (Index k = 0; k < numK; ++k) {
      const VehicleType& vehicle = inst.vehicles[static_cast<size_t>(k)];
      capWeight += vehicle.weightCap * plan.vehicles[static_cast<size_t>(k)];
      capArea += vehicle.areaCap * plan.vehicles[static_cast<size_t>(k)];
    }
    violations.linkWeight = (cargoWeight - capWeight).maxCoeff();
    violations.linkArea = (cargoArea - capArea).maxCoeff();

    // Vehicle families: negativity, circulation, budget, per type. The
    // circulation residual accumulates PER-ARC differences u_ij - u_ji, not
    // (sum in) - (sum out): each term is exactly zero for an out-and-back
    // plan (Lemma FL2), so symmetric u reports exactly 0 regardless of the
    // reduction order a whole-row/column sum would use.
    for (Index k = 0; k < numK; ++k) {
      const MatrixXd& u = plan.vehicles[static_cast<size_t>(k)];
      violations.negativity = std::max(violations.negativity, -u.minCoeff());
      for (Index j = 0; j < m; ++j) {
        double residual = 0.0;
        for (Index i = 0; i < m; ++i) {
          residual += u(i, j) - u(j, i);
        }
        violations.vehicleBalance = std::max(violations.vehicleBalance,
                                             std::abs(residual));
      }
      violations.budget = std::max(violations.budget,
                                   vehicleMiles(inst, plan, k)
                                       - vehicleBudget(inst, k));
    }

    // Clamp families that are differences to non-negative magnitudes.
    violations.delivery = std::max(violations.delivery, 0.0);
    violations.capacity = std::max(violations.capacity, 0.0);
    violations.negativity = std::max(violations.negativity, 0.0);
    violations.idleResupply = std::max(violations.idleResupply, 0.0);
    violations.linkWeight = std::max(violations.linkWeight, 0.0);
    violations.linkArea = std::max(violations.linkArea, 0.0);
    violations.budget = std::max(violations.budget, 0.0);
    return violations;
  }

  double
  maxViolation(const FleetPlanViolations& violations)
  {
    double worst = violations.assetBalance;
    worst = std::max(worst, violations.delivery);
    worst = std::max(worst, violations.capacity);
    worst = std::max(worst, violations.negativity);
    worst = std::max(worst, violations.idleResupply);
    worst = std::max(worst, violations.linkWeight);
    worst = std::max(worst, violations.linkArea);
    worst = std::max(worst, violations.vehicleBalance);
    worst = std::max(worst, violations.budget);
    return worst;
  }

} // namespace VIMCP::Network
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------

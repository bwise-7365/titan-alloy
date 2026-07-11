// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
// Fleet plan (S, R, x, u) for a FleetInstance: evaluation and feasibility
// checking (fleet-formulation.md sections 2-4).
// ----------------------------------------------
#ifndef VIMCP_NETWORK_FLEETPLAN_HPP
#define VIMCP_NETWORK_FLEETPLAN_HPP

#include "fleetinstance.hpp"

namespace VIMCP::Network {

  // ---------------------------------------------------------------------------
  // FleetPlan
  // ---------------------------------------------------------------------------

  // One candidate fleet plan: supplied(i, a) = S_ia (units of asset a node i
  // injects), resupply(i, a) = R_ia (units delivered to i), flow[a](i, j) =
  // x^a_ij (units of asset a shipped directly i -> j; the diagonal is
  // self-supply), vehicles[k](i, j) = u^k_ij (vehicles of type k traversing
  // i -> j over the horizon, loaded or empty, fractional).
  struct FleetPlan {
    MatrixXd supplied;            // S_ia   (numNodes x numAssets)
    MatrixXd resupply;            // R_ia   (numNodes x numAssets)
    vector<MatrixXd> flow;        // x^a    (numAssets matrices, numNodes^2)
    vector<MatrixXd> vehicles;    // u^k    (numVehicleTypes matrices)
  };

  // All-zero plan sized for the instance. Always feasible; its objective is
  // the full-scale shortfall sum of P over demand cells.
  FleetPlan makeZeroFleetPlan(const FleetInstance& inst);

  // Total vehicle-miles run by type k: sum_ij d_ij * u^k_ij (loaded AND
  // empty; fleet-formulation.md G-F3).
  double vehicleMiles(const FleetInstance& inst, const FleetPlan& plan,
                      Index k);

  // Weighted quadratic shortfall of `resupply` against a `target` resupply
  // (both numNodes x numAssets), normalized by demand:
  // sum_{(i,a): D_ia > 0} P_ia ((target_ia - resupply_ia)/D_ia)^2.
  // Cells with D_ia = 0 are excluded by convention.
  double fleetShortfallVsTarget(const FleetInstance& inst,
                                const MatrixXd& target,
                                const MatrixXd& resupply);

  // fleetShortfallVsTarget with target = inst.demand: the objective vs
  // ORIGINAL demand for a bare resupply matrix (e.g. phase-1 targets).
  double fleetShortfallOfResupply(const FleetInstance& inst,
                                  const MatrixXd& resupply);

  // The same shortfall evaluated at plan.resupply.
  double fleetShortfallObjective(const FleetInstance& inst,
                                 const FleetPlan& plan);

  // ---------------------------------------------------------------------------
  // Feasibility checking
  // ---------------------------------------------------------------------------

  // Worst violation of each constraint family, all as non-negative
  // magnitudes; a feasible plan has every field 0. Families carry
  // HETEROGENEOUS units (noted per line).
  //   assetBalance   max_{j,a} |sum_i x^a_ij + S_ja - R_ja - sum_l x^a_jl|
  //                  (units of asset a)
  //   delivery       max_{j,a} (R_ja - sum_i x^a_ij)+   [per-asset F1 fix:
  //                  resupply must ARRIVE via arcs]      (units)
  //   capacity       max_{i,a} (S_ia - C_ia)+                     (units)
  //   negativity     worst negative entry magnitude across S, R, x, u
  //   idleResupply   max R_ja at a cell with D_ja = 0 (convention) (units)
  //   linkWeight     max_ij (sum_a w_a x^a_ij - sum_k T_k u^k_ij)+  (tons)
  //   linkArea       max_ij (sum_a s_a x^a_ij - sum_k A_k u^k_ij)+  (sqft)
  //   vehicleBalance max_{k,j} |sum_i u^k_ij - sum_l u^k_jl|     (vehicles)
  //   budget         max_k (vehicleMiles_k - B_k)+          (vehicle-miles)
  struct FleetPlanViolations {
    double assetBalance = 0.0;
    double delivery = 0.0;
    double capacity = 0.0;
    double negativity = 0.0;
    double idleResupply = 0.0;
    double linkWeight = 0.0;
    double linkArea = 0.0;
    double vehicleBalance = 0.0;
    double budget = 0.0;
  };

  // Evaluate all violation families. Throws std::invalid_argument if the
  // plan's shapes do not match the instance.
  FleetPlanViolations checkFleetPlan(const FleetInstance& inst,
                                     const FleetPlan& plan);

  // Largest violation across all families: 0 exactly when the plan is
  // feasible.
  double maxViolation(const FleetPlanViolations& violations);

} // namespace VIMCP::Network

#endif // VIMCP_NETWORK_FLEETPLAN_HPP
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------

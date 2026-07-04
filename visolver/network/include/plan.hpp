// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
// Flow plan (S, R, f) for an Instance: evaluation and feasibility checking.
// ----------------------------------------------
#ifndef VINCP_NETWORK_PLAN_HPP
#define VINCP_NETWORK_PLAN_HPP

#include "instance.hpp"

namespace VINCP::Network {

  // ---------------------------------------------------------------------------
  // Plan
  // ---------------------------------------------------------------------------

  // One candidate plan (formulation.md section 2): supplied(i) = S_i (tons node
  // i injects), resupply(i) = R_i (tons delivered to i), flow(i, j) = f_ij
  // (tons shipped directly i -> j; the diagonal f_ii is the self-supply flow).
  struct Plan {
    VectorXd supplied;    // S_i    (size numNodes)
    VectorXd resupply;    // R_i    (size numNodes)
    MatrixXd flow;        // f_ij   (numNodes x numNodes)
  };

  // All-zero plan sized for the instance. Always feasible (formulation.md
  // section 4); its objective is the full-scale shortfall sum of P over sinks.
  Plan makeZeroPlan(const Instance& inst);

  // Total ton-miles moved: sum_ij c_ij * f_ij.
  double tonMiles(const Instance& inst, const Plan& plan);

  // Weighted quadratic shortfall of `resupply` against a `target` resupply,
  // normalized by demand: sum_{i: D_i > 0} P_i ((target_i - resupply_i)/D_i)^2.
  // Nodes with D_i = 0 are excluded by convention (formulation.md section 4).
  // With target = inst.demand this is the objective vs ORIGINAL demand; with
  // target = the phase-1 rationed targets it measures how far a plan falls short
  // of what capacity actually allows.
  double shortfallVsTarget(const Instance& inst, const VectorXd& target,
                           const VectorXd& resupply);

  // The weighted quadratic shortfall sum_{i: D_i > 0} P_i ((D_i - R_i)/D_i)^2
  // for a bare resupply vector (size numNodes) -- i.e. shortfallVsTarget with
  // target = inst.demand. Used both for full plans and for resupply TARGETS
  // that have no flows yet (e.g. the phase-1 rationing bound).
  double shortfallOfResupply(const Instance& inst, const VectorXd& resupply);

  // The same shortfall evaluated at plan.resupply.
  double shortfallObjective(const Instance& inst, const Plan& plan);

  // ---------------------------------------------------------------------------
  // Feasibility checking
  // ---------------------------------------------------------------------------

  // Worst violation of each constraint family, all as non-negative magnitudes
  // in tons (budget in ton-miles); a feasible plan has every field 0.
  //   balance      max_j |sum_i f_ij + S_j - R_j - sum_k f_jk|
  //   delivery     max_j (R_j - sum_i f_ij)+       [the F1 fix: resupply must
  //                                                 ARRIVE via arcs]
  //   capacity     max_i (S_i - C_i)+
  //   negativity   worst negative entry magnitude across S, R, f
  //   idleResupply max resupply at a node with D_i = 0 (convention R_i = 0)
  //   budget       (tonMiles - L)+, or 0 when tonMileLimit == 0 (uncalibrated)
  struct PlanViolations {
    double balance = 0.0;
    double delivery = 0.0;
    double capacity = 0.0;
    double negativity = 0.0;
    double idleResupply = 0.0;
    double budget = 0.0;
  };

  // Evaluate all violation families. Throws std::invalid_argument if the plan's
  // shapes do not match the instance.
  PlanViolations checkPlan(const Instance& inst, const Plan& plan);

  // Largest violation across all families: 0 exactly when the plan is feasible.
  double maxViolation(const PlanViolations& violations);

} // namespace VINCP::Network

#endif // VINCP_NETWORK_PLAN_HPP
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------

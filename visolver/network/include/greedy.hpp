// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
// The greedy notional planner: phase-1 rationing targets, phase-2 greedy
// flows, and ton-mile budget calibration (problem spec; formulation.md
// section 6 audit).
// ----------------------------------------------
#ifndef VIMCP_NETWORK_GREEDY_HPP
#define VIMCP_NETWORK_GREEDY_HPP

#include "plan.hpp"

namespace VIMCP::Network {

  // ---------------------------------------------------------------------------
  // Phase 1: rationing
  // ---------------------------------------------------------------------------

  // scqkp -- of course, a very simple example of the CONTINUOUS QUADRATIC
  // KNAPSACK PROBLEM (the quadratic case of Patriksson's continuous separable
  // convex resource-allocation problem; itself just a special case of the
  // monotone complementarity problems this library solves). Shared by
  // rationTargets (whole instance) and the fleet planner's per-asset rationing
  // (fleet-formulation.md Lemma FL1): minimize the weighted quadratic shortfall
  // of a resupply vector against `demand` subject to sum R_i <= meetable and
  // 0 <= R_i <= D_i. Closed form R_i = D_i - lambda D_i^2 / P_i on the active
  // entries, the multiplier lambda raised until the budget binds, with the
  // exclude-and-resolve PEGGING clamp (any R_i that comes out negative is fixed
  // to 0 and lambda re-solved over the rest; finitely many rounds). When total
  // demand <= meetable the targets are simply D. `priority` must be positive
  // wherever `demand` is positive; callers validate.
  VectorXd scqkp(const VectorXd& demand, const VectorXd& priority,
                 double meetable);

  // Optimal resupply TARGETS ignoring routing and the budget: scqkp with
  // meetable = min(total demand, total capacity). The result is also the
  // budget-unconstrained LOWER bound theta_ration of the validation sandwich
  // (formulation.md section 7).
  VectorXd rationTargets(const Instance& inst);

  // ---------------------------------------------------------------------------
  // Phase 2: greedy flows + calibration
  // ---------------------------------------------------------------------------

  struct GreedyParams {
    // Targets are scaled by (1 - demandScaleDown) before flow construction, so
    // total remaining demand is strictly below total capacity and step (2) of
    // the greedy loop can never fail to find a source. Spec value: 0.01%.
    double demandScaleDown = 1.0e-4;
    // Suggested ton-mile limit as a fraction of the greedy plan's own usage.
    // Spec value: 80%.
    double budgetFraction = 0.8;
  };

  struct GreedyResult {
    Plan plan;                  // feasible (checkPlan-clean) notional plan
    VectorXd targets;           // phase-1 rationed targets (UNscaled)
    double tonMilesUsed = 0.0;  // sum c_ij f_ij of the greedy plan
    double suggestedLimit = 0.0;   // budgetFraction * tonMilesUsed
    int iterations = 0;         // phase-2 loop count
  };

  // The greedy notional planner. Repeatedly serves the largest remaining
  // (scaled) target from its cheapest source with remaining capacity, shipping
  // on DIRECT arcs only (no multi-hop routing; that advantage is the
  // optimizer's). The returned plan ignores any tonMileLimit on the instance
  // by design: its role is to CALIBRATE the limit (set inst.tonMileLimit =
  // result.suggestedLimit) and to serve as the quality baseline at
  // L = tonMilesUsed. Throws std::invalid_argument on bad params and
  // std::runtime_error if the loop cannot proceed (defensive; unreachable for
  // valid inputs).
  GreedyResult greedyPlan(const Instance& inst, const GreedyParams& params = {});

} // namespace VIMCP::Network

#endif // VIMCP_NETWORK_GREEDY_HPP
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------

// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
// The greedy fleet planner: per-asset rationing targets and greedy
// vehicle-borne flows under per-type vehicle-mile budgets
// (fleet-formulation.md sections 5-8).
// ----------------------------------------------
#ifndef VINCP_NETWORK_FLEETGREEDY_HPP
#define VINCP_NETWORK_FLEETGREEDY_HPP

#include "fleetplan.hpp"
#include "greedy.hpp"

namespace VINCP::Network {

  // ---------------------------------------------------------------------------
  // Phase 1: per-asset rationing
  // ---------------------------------------------------------------------------

  // Optimal resupply TARGETS ignoring transport entirely: one scqkp (continuous
  // quadratic-knapsack) rationing per asset column, with meetable_a = min(total
  // demand of a, total capacity of a). Exact, not heuristic -- assets never compete for
  // SUPPLY, only for transport, so the joint rationing problem decomposes
  // column-wise (fleet-formulation.md Lemma FL1).
  MatrixXd rationFleetTargets(const FleetInstance& inst);

  // ---------------------------------------------------------------------------
  // Phase 2: greedy fleet flows
  // ---------------------------------------------------------------------------

  struct FleetGreedyParams {
    // Targets are scaled by (1 - demandScaleDown) PER ASSET before flow
    // construction, so each asset's remaining demand stays strictly below its
    // capacity and a source always exists for a servable cell. Spec value:
    // 0.01%, as the base greedy.
    double demandScaleDown = 1.0e-4;
  };

  struct FleetGreedyResult {
    FleetPlan plan;           // feasible (checkFleetPlan-clean) notional plan
    MatrixXd targets;         // phase-1 rationed targets, UNscaled (m x A)
    VectorXd milesUsed;       // vehicle-miles run per type (size K)
    VectorXd budget;          // B_k = N_k v_k H per type (size K)
    VectorXd utilization;     // milesUsed / budget per type; 0 when B_k = 0
    MatrixXd unserved;        // (scaled target - resupply)+ per cell (m x A)
    double shortfallValue = 0.0;   // fleetShortfallObjective of the plan
    // max_k (miles type k uses when budgets are unlimited) / B_k, over types
    // with B_k > 0: how much bigger the fleet must be for this heuristic to
    // serve every rationed target; <= 1 means the given fleet sufficed
    // (fleet-formulation.md G-F4, the inversion of the base greedy's
    // suggestedLimit -- budgets are DATA here, so calibration runs backward).
    double fleetScaleHint = 0.0;
    int iterations = 0;       // phase-2 loop passes (real pass)
  };

  // The greedy fleet planner. Repeatedly serves the (node, asset) cell with
  // the largest remaining priority-weighted fractional shortfall from its
  // cheapest-ROUND-TRIP source with remaining capacity (deadhead return legs
  // are charged, G-F3), loading vehicle types best-unit-capacity-first
  // (G-F5/G-F6); vehicles move out-and-back so circulation holds exactly
  // (Lemma FL2). Ships on DIRECT arcs only, like the base greedy. Cells no
  // positive-budget type can carry are reported in `unserved` rather than
  // failing. Throws std::invalid_argument on bad inputs and
  // std::runtime_error if the loop cannot proceed (defensive; unreachable
  // for valid inputs).
  FleetGreedyResult greedyFleetPlan(const FleetInstance& inst,
                                    const FleetGreedyParams& params = {});

} // namespace VINCP::Network

#endif // VINCP_NETWORK_FLEETGREEDY_HPP
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------

// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
// Swap improvement of a fleet plan: each asset's flows are driven to a
// 2-exchange local optimum by the single-commodity swap engine, then the
// vehicle matrices are reallocated to carry the swapped flows.
// ----------------------------------------------
#ifndef VIMCP_NETWORK_FLEETSWAP_HPP
#define VIMCP_NETWORK_FLEETSWAP_HPP

#include "fleetplan.hpp"
#include "swap.hpp"

namespace VIMCP::Network {

  struct FleetSwapSummary
  {
    vector<int> swapsPerAsset;      // 2-exchanges applied, per asset
    int totalSwaps = 0;
    double unitMileSaving = 0.0;    // sum over assets of unit round-trip-mile
                                    // savings (the swap engine's metric)
    VectorXd milesUsedBefore;       // per-type vehicle-miles of the incoming
    VectorXd milesUsedAfter;        //   plan / of the reallocated plan
  };

  // Improve a feasible fleet plan IN PLACE without changing what anyone
  // receives. Per asset a, the flows x^a are a single-commodity
  // transportation plan, so the existing swap engine applies verbatim:
  // swapToLocalOptimum runs on a slice whose cost matrix is the ROUND-TRIP
  // distance d_ij + d_ji (deadheading is charged, fleet-formulation.md
  // G-F3), leaving supplied/resupply -- hence the shortfall objective --
  // bit-for-bit unchanged. Vehicles cannot be swapped along (u^k aggregates
  // all assets on a link), so after the flows settle the vehicle matrices
  // are REBUILT from scratch by the greedy planner's transport rule: per
  // (asset, link) shipment, spill across types best unitCapacity first
  // within the per-type budgets, out-and-back so circulation holds exactly
  // (Lemma FL2). The swaps only reduce each asset's unit round-trip-miles,
  // so the rebuilt plan needs no more vehicle-miles than the original;
  // should the (order-dependent, greedy) spill nevertheless fail to place
  // some cargo within the budgets, this throws std::runtime_error rather
  // than return an infeasible plan -- the incoming plan is left partially
  // modified only in its vehicle matrices, so callers should treat a throw
  // as "recompute the plan". Throws std::invalid_argument on shape
  // mismatches.
  FleetSwapSummary swapFleetToLocalOptimum(const FleetInstance& inst,
                                           FleetPlan& plan,
                                           int maxSwapsPerAsset = 100000);

  // ---------------------------------------------------------------------------
  // Fleet purification (plan.md G5f; fleet-formulation.md G-F9)
  // ---------------------------------------------------------------------------

  struct FleetPurifySummary
  {
    int improvingSwaps = 0;          // summed over assets
    int consolidatingSwaps = 0;
    vector<int> arcsBeforePerAsset;  // positive arcs of flow[a]
    vector<int> arcsAfterPerAsset;
    VectorXd milesUsedBefore;        // per type
    VectorXd milesUsedAfter;
  };

  // Sparsify a fleet plan IN PLACE without changing deliveries: per asset,
  // run the single-commodity purifyPlan (swap.hpp) on the round-trip-cost
  // slice with the ton-mile cap set to that asset's round-trip unit-miles
  // AT ENTRY -- the cap starts with zero slack, so only pivots with
  // non-negative saving are accepted at first, and no asset's usage ever
  // exceeds what the incoming (feasible) plan already spent (user decision
  // 2026-07-07, finding G-F9). One vehicle reallocation at the end, exactly
  // as swapFleetToLocalOptimum; the same throw-on-overflow contract applies.
  FleetPurifySummary purifyFleetPlan(const FleetInstance& inst,
                                     FleetPlan& plan,
                                     int maxSwapsPerAsset = 100000);

} // namespace VIMCP::Network

#endif // VIMCP_NETWORK_FLEETSWAP_HPP
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------

// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
// The transportation 2-exchange ("swap") on a plan: move x tons from f_ij to
// f_in and the same x from f_mn to f_mj, which preserves every node's total
// in/out flow and changes only the routing cost. Per-unit saving is
// (c_ij + c_mn) - (c_in + c_mj); the step is x = min(f_ij, f_mn). Three search
// depths (node-local, global, iterate-to-local-optimum) share one evaluator.
// ----------------------------------------------
#ifndef VINCP_NETWORK_SWAP_HPP
#define VINCP_NETWORK_SWAP_HPP

#include "plan.hpp"

namespace VINCP::Network {

  // A candidate 2-exchange. `saving` is the TOTAL ton-mile reduction of applying
  // it (amount * per-unit saving); `improvingP` is saving > a small tolerance.
  // i,j,m,n = -1 and improvingP = false means "no improving move found".
  struct SwapMove
  {
    Index i = -1, j = -1, m = -1, n = -1;   // move x: f_ij->f_in, f_mn->f_mj
    double amount = 0.0;                     // x = min(f_ij, f_mn)
    double saving = 0.0;                     // ton-miles removed (>= 0 if improving)
    bool improvingP = false;
  };

  struct SwapSummary
  {
    int swaps = 0;
    double totalSaving = 0.0;   // cumulative ton-miles removed
  };

  // Version 1: the most ton-mile-improving 2-exchange whose two reduced arcs
  // include one incident to `node` (node is a sender or receiver of it). Returns
  // a non-improving move if none helps.
  SwapMove bestSwapAtNode(const Instance& inst, const Plan& plan, Index node);

  // Version 2: the most improving 2-exchange over every pair of positive-flow
  // arcs in the plan.
  SwapMove bestSwap(const Instance& inst, const Plan& plan);

  // Intermediate: repeatedly apply the best swap incident to `node` until none
  // improves (or maxSwaps is hit). Mutates plan; returns count + saving. Cheaper
  // and more local than swapToLocalOptimum -- it drives one node to a swap
  // optimum without touching the rest of the map.
  SwapSummary swapNodeToLocalOptimum(const Instance& inst, Plan& plan, Index node,
                                     int maxSwaps = 100000);

  // Apply a move in place (updates plan.flow; plan.supplied / plan.resupply are
  // invariant under a 2-exchange and left unchanged). A non-move (i < 0) is a
  // no-op.
  void applySwap(Plan& plan, const SwapMove& move);

  // Version 3: repeatedly apply the global best swap until none improves (or
  // maxSwaps is hit). Mutates plan; returns the count and cumulative saving.
  SwapSummary swapToLocalOptimum(const Instance& inst, Plan& plan,
                                 int maxSwaps = 100000);

  // ---------------------------------------------------------------------------
  // Vertex purification ("swap-as-pivot" crossover, plan.md F4)
  // ---------------------------------------------------------------------------

  // Off-diagonal arcs carrying flow above the engine's flow tolerance -- the
  // sparsity measure purification drives down.
  int positiveArcCount(const Plan& plan);

  struct PurifySummary
  {
    int improvingSwaps = 0;       // ton-mile-saving pivots applied
    int consolidatingSwaps = 0;   // arc-count-reducing pivots applied
    double totalSaving = 0.0;     // NET ton-miles removed (a consolidating
                                  // pivot may spend some, so this can dip)
    int arcsBefore = 0;
    int arcsAfter = 0;
  };

  // Sparsify a plan WITHOUT changing what anyone receives: the 2-exchange is
  // a transportation-simplex cycle pivot, supplied/resupply (hence the
  // shortfall objective theta) are invariant, and every optimal flow pattern
  // lives on a polytope whose vertices are forest-sparse -- an interior-point
  // solve parks at that polytope's maximally-spread analytic center, so the
  // tiny flows are geometry, not noise (F4). Runs an opening pass of plain
  // improving swaps to a local optimum, then repeatedly applies the
  // best-saving swap that strictly REDUCES the positive-arc count --
  // accepting even a NEGATIVE saving as long as total ton-miles stay within
  // tonMileCap (pass tonMileLimit for an optimal plan so budget feasibility
  // is preserved; +infinity for an uncapped greedy plan) -- re-running
  // improving swaps between consolidations, restricted to pivots that do not
  // raise the arc count (an unrestricted improving pivot could re-spread
  // what a spending consolidation just removed, and the pair would cycle).
  // Terminates: after the opening pass the arc count never rises and each
  // consolidating pivot strictly drops it; improving pivots each remove
  // > kSaveTol ton-miles, bounded below by zero. Length-4 cycle pivots only,
  // so the result is a pairwise-pivot local optimum -- in practice
  // forest-sparse, though not a certified polytope vertex.
  PurifySummary purifyPlan(const Instance& inst, Plan& plan,
                           double tonMileCap, int maxSwaps = 100000);

} // namespace VINCP::Network

#endif // VINCP_NETWORK_SWAP_HPP
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------

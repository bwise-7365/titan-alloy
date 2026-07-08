// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
// Transportation 2-exchange implementation: enumerate positive-flow arc pairs,
// score each by its ton-mile saving, and apply the chosen move.
// ----------------------------------------------
#include "swap.hpp"

#include <algorithm>
#include <utility>
#include <vector>

using std::pair;
using std::vector;

namespace VINCP::Network {

  namespace {

    const double kFlowTol = 1.0e-9;    // an arc "carries flow" above this
    const double kSaveTol = 1.0e-6;    // a swap "improves" above this (ton-miles)

    // Off-diagonal arcs that carry flow. Self-supply (f_ii) is not a swap
    // candidate: the 2-exchange re-routes transport between two source-sink
    // pairs.
    vector<pair<Index, Index>>
    positiveArcs(const Plan& plan)
    {
      vector<pair<Index, Index>> arcs;
      const Index m = plan.flow.rows();
      for (Index a = 0; a < m; ++a) {
        for (Index b = 0; b < m; ++b) {
          if (a != b && plan.flow(a, b) > kFlowTol) {
            arcs.emplace_back(a, b);
          }
        }
      }
      return arcs;
    }

    // Score the exchange that reduces arcs (i,j) and (m,n). Degenerate cases
    // (shared endpoint, or a self-loop target arc) are rejected as non-moves.
    SwapMove
    evaluate(const Instance& inst, const Plan& plan, Index i, Index j, Index mm,
             Index n)
    {
      SwapMove move;
      if (i == mm || j == n || i == n || mm == j) {
        return move;                      // no meaningful swap
      }
      const double perUnit = (inst.cost(i, j) + inst.cost(mm, n))
                             - (inst.cost(i, n) + inst.cost(mm, j));
      const double amount = std::min(plan.flow(i, j), plan.flow(mm, n));
      move.i = i;
      move.j = j;
      move.m = mm;
      move.n = n;
      move.amount = amount;
      move.saving = amount * perUnit;
      move.improvingP = move.saving > kSaveTol;
      return move;
    }

    // Keep whichever of two candidates saves more.
    const SwapMove&
    better(const SwapMove& a, const SwapMove& b)
    {
      return (b.saving > a.saving) ? b : a;
    }

  } // namespace

  SwapMove
  bestSwapAtNode(const Instance& inst, const Plan& plan, Index node)
  {
    const vector<pair<Index, Index>> arcs = positiveArcs(plan);
    SwapMove best;
    for (const pair<Index, Index>& p : arcs) {
      if (p.first != node && p.second != node) {
        continue;                         // p must touch the clicked node
      }
      for (const pair<Index, Index>& q : arcs) {
        if (p == q) {
          continue;
        }
        best = better(best,
                      evaluate(inst, plan, p.first, p.second, q.first, q.second));
      }
    }
    return best;
  }

  SwapMove
  bestSwap(const Instance& inst, const Plan& plan)
  {
    const vector<pair<Index, Index>> arcs = positiveArcs(plan);
    SwapMove best;
    for (size_t a = 0; a < arcs.size(); ++a) {
      for (size_t b = a + 1; b < arcs.size(); ++b) {
        best = better(best, evaluate(inst, plan, arcs[a].first, arcs[a].second,
                                     arcs[b].first, arcs[b].second));
      }
    }
    return best;
  }

  void
  applySwap(Plan& plan, const SwapMove& move)
  {
    if (0 > move.i) {
      return;                             // no move
    }
    // supplied / resupply are invariant under a 2-exchange (each node's total
    // in and out are unchanged); only the routing, hence plan.flow, moves.
    plan.flow(move.i, move.j) -= move.amount;
    plan.flow(move.i, move.n) += move.amount;
    plan.flow(move.m, move.n) -= move.amount;
    plan.flow(move.m, move.j) += move.amount;
    return;
  }

  SwapSummary
  swapNodeToLocalOptimum(const Instance& inst, Plan& plan, Index node,
                         int maxSwaps)
  {
    SwapSummary summary;
    while (summary.swaps < maxSwaps) {
      const SwapMove move = bestSwapAtNode(inst, plan, node);
      if (!move.improvingP) {
        break;
      }
      applySwap(plan, move);
      ++summary.swaps;
      summary.totalSaving += move.saving;
    }
    return summary;
  }

  SwapSummary
  swapToLocalOptimum(const Instance& inst, Plan& plan, int maxSwaps)
  {
    SwapSummary summary;
    while (summary.swaps < maxSwaps) {
      const SwapMove move = bestSwap(inst, plan);
      if (!move.improvingP) {
        break;
      }
      applySwap(plan, move);
      ++summary.swaps;
      summary.totalSaving += move.saving;
    }
    return summary;
  }

  int
  positiveArcCount(const Plan& plan)
  {
    return static_cast<int>(positiveArcs(plan).size());
  }

  namespace {

    // Change in the positive-arc count if `move` were applied: the reduced
    // arcs that would fall to (or below) the flow tolerance leave, the target
    // arcs that are not yet positive join. amount > kFlowTol (both reduced
    // arcs are positive arcs), so a new target arc really becomes positive.
    int
    arcCountDelta(const Plan& plan, const SwapMove& move)
    {
      int delta = 0;
      if (plan.flow(move.i, move.j) - move.amount <= kFlowTol) {
        --delta;
      }
      if (plan.flow(move.m, move.n) - move.amount <= kFlowTol) {
        --delta;
      }
      if (plan.flow(move.i, move.n) <= kFlowTol) {
        ++delta;
      }
      if (plan.flow(move.m, move.j) <= kFlowTol) {
        ++delta;
      }
      return delta;
    }

    // The best-saving swap that strictly reduces the positive-arc count and
    // keeps total ton-miles within the cap. Unlike bestSwap, the saving may
    // be NEGATIVE: theta is invariant under any 2-exchange, so within the
    // budget the plan is free to spend ton-miles to buy sparsity. Returns a
    // non-move (i = -1) when no candidate qualifies; foundP reports it.
    struct ConsolidateMove {
      SwapMove move;
      bool foundP = false;
    };

    ConsolidateMove
    bestConsolidatingSwap(const Instance& inst, const Plan& plan,
                          double tonMileSlack)
    {
      const vector<pair<Index, Index>> arcs = positiveArcs(plan);
      ConsolidateMove best;
      for (size_t a = 0; a < arcs.size(); ++a) {
        for (size_t b = a + 1; b < arcs.size(); ++b) {
          const SwapMove move =
              evaluate(inst, plan, arcs[a].first, arcs[a].second,
                       arcs[b].first, arcs[b].second);
          if (0 > move.i || 0 <= arcCountDelta(plan, move)) {
            continue;                     // not a move, or not consolidating
          }
          if (-move.saving > tonMileSlack) {
            continue;                     // would spend past the budget cap
          }
          if (!best.foundP || move.saving > best.move.saving) {
            best.move = move;
            best.foundP = true;
          }
        }
      }
      return best;
    }

    // The most improving swap that does NOT increase the positive-arc count.
    // Used once consolidation begins: an unrestricted improving pivot can add
    // an arc, and a spending consolidating pivot can then remove it again --
    // an endless A<->B exchange. Filtering to non-spreading pivots keeps the
    // arc count monotone, which is the termination argument.
    SwapMove
    bestNonSpreadingSwap(const Instance& inst, const Plan& plan)
    {
      const vector<pair<Index, Index>> arcs = positiveArcs(plan);
      SwapMove best;
      for (size_t a = 0; a < arcs.size(); ++a) {
        for (size_t b = a + 1; b < arcs.size(); ++b) {
          const SwapMove move =
              evaluate(inst, plan, arcs[a].first, arcs[a].second,
                       arcs[b].first, arcs[b].second);
          if (0 > move.i || 0 < arcCountDelta(plan, move)) {
            continue;
          }
          best = better(best, move);
        }
      }
      return best;
    }

  } // namespace

  PurifySummary
  purifyPlan(const Instance& inst, Plan& plan, double tonMileCap, int maxSwaps)
  {
    PurifySummary summary;
    summary.arcsBefore = positiveArcCount(plan);
    int total = 0;

    // Opening pass: plain improving swaps, unrestricted (they may spread).
    // Every later improving pivot is restricted to non-spreading ones, so
    // from here on the arc count never rises. Termination: consolidating
    // pivots strictly drop the arc count (at most arcsBefore of them ever);
    // between two of them the non-spreading improving pivots each remove
    // > kSaveTol ton-miles at a fixed-or-falling arc count, bounded below by
    // zero ton-miles.
    const SwapSummary opening = swapToLocalOptimum(inst, plan, maxSwaps);
    summary.improvingSwaps += opening.swaps;
    summary.totalSaving += opening.totalSaving;
    total += opening.swaps;

    while (total < maxSwaps) {
      const double slack = tonMileCap - tonMiles(inst, plan);
      const ConsolidateMove found = bestConsolidatingSwap(inst, plan, slack);
      if (!found.foundP) {
        break;
      }
      applySwap(plan, found.move);
      ++summary.consolidatingSwaps;
      summary.totalSaving += found.move.saving;
      ++total;

      while (total < maxSwaps) {
        const SwapMove move = bestNonSpreadingSwap(inst, plan);
        if (!move.improvingP) {
          break;
        }
        applySwap(plan, move);
        ++summary.improvingSwaps;
        summary.totalSaving += move.saving;
        ++total;
      }
    }

    summary.arcsAfter = positiveArcCount(plan);
    return summary;
  }

} // namespace VINCP::Network
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------

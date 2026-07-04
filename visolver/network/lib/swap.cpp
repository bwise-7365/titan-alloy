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

} // namespace VINCP::Network
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------

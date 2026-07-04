// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
// Greedy notional planner implementation: water-filling rationing, then
// cheapest-source greedy flow construction.
// ----------------------------------------------
#include "greedy.hpp"

#include <algorithm>
#include <limits>
#include <stdexcept>

namespace VINCP::Network {

  namespace {

    double
    rationWeight(const Instance& inst, Index i)
    {
      return inst.demand(i) * inst.demand(i) / inst.priority(i);
    }

    void
    validateGreedyParams(const GreedyParams& params)
    {
      if (!(0.0 < params.demandScaleDown && params.demandScaleDown < 1.0)) {
        throw std::invalid_argument(
            "greedyPlan: demandScaleDown must lie in (0, 1).");
      }
      if (!(0.0 < params.budgetFraction && params.budgetFraction <= 1.0)) {
        throw std::invalid_argument(
            "greedyPlan: budgetFraction must lie in (0, 1].");
      }
      return;
    }

  } // namespace

  VectorXd
  rationTargets(const Instance& inst)
  {
    validateInstance(inst);
    VectorXd targets = VectorXd::Zero(inst.numNodes);
    const double meetable =
        std::min(totalDemand(inst), totalSupplyCap(inst));
    if (0.0 >= meetable) {
      return targets;                       // nothing can move (or no demand)
    }
    if (totalDemand(inst) <= totalSupplyCap(inst)) {
      targets = inst.demand;                // all demand meetable (G1 boundary
      return targets;                       // included: equality lands here)
    }

    // Shortfall: water-filling with the exclude-and-resolve clamp (G2). Each
    // round solves lambda over the active set; nodes whose interior R_i comes
    // out negative are fixed to 0 and the round repeats. The active set
    // strictly shrinks, so at most |V_D| rounds.
    vector<Index> active = sinkNodes(inst);
    while (!active.empty()) {
      double activeDemand = 0.0;
      double activeWeight = 0.0;
      for (const Index i : active) {
        activeDemand += inst.demand(i);
        activeWeight += rationWeight(inst, i);
      }
      const double lambda = (activeDemand - meetable) / activeWeight;
      if (0.0 >= lambda) {
        // Remaining active demand fits under the cap: no rationing among them.
        for (const Index i : active) {
          targets(i) = inst.demand(i);
        }
        return targets;
      }

      bool clampedP = false;
      vector<Index> survivors;
      for (const Index i : active) {
        if (0.0 > inst.demand(i) - lambda * rationWeight(inst, i)) {
          clampedP = true;                  // excluded: stays at target 0
        }
        else {
          survivors.push_back(i);
        }
      }
      if (!clampedP) {
        for (const Index i : active) {
          targets(i) = inst.demand(i) - lambda * rationWeight(inst, i);
        }
        return targets;
      }
      active = survivors;
    }
    // Unreachable: within a round the interior R_i sum to meetable > 0, so
    // they cannot all be negative. Guard per the throw-never-substitute stance.
    throw std::runtime_error("rationTargets: clamping emptied the active set.");
  }

  GreedyResult
  greedyPlan(const Instance& inst, const GreedyParams& params)
  {
    validateInstance(inst);
    validateGreedyParams(params);

    GreedyResult result;
    result.targets = rationTargets(inst);

    const Index m = inst.numNodes;
    VectorXd remainingDemand = (1.0 - params.demandScaleDown) * result.targets;
    VectorXd remainingCap = inst.supplyCap;
    MatrixXd flow = MatrixXd::Zero(m, m);

    // Each pass zeroes remainingDemand(n) or remainingCap(source) exactly (the
    // subtraction of the min is exact), and no (source, n) pair repeats, so
    // the loop runs at most #sinks + #sources <= 2m times (G3).
    const int iterCap = static_cast<int>(2 * m + 2);
    int iter = 0;
    while (true) {
      Index n = 0;
      const double maxRemaining = remainingDemand.maxCoeff(&n);
      if (0.0 >= maxRemaining) {
        break;                              // (1): all targets served
      }

      Index source = -1;                    // (2): cheapest source with capacity
      double sourceCost = std::numeric_limits<double>::infinity();
      for (Index i = 0; i < m; ++i) {
        if (0.0 < remainingCap(i) && inst.cost(i, n) < sourceCost) {
          source = i;
          sourceCost = inst.cost(i, n);
        }
      }
      if (-1 == source) {
        // Unreachable for valid inputs: the scale-down keeps total remaining
        // demand strictly below total capacity.
        throw std::runtime_error("greedyPlan: no source with capacity left.");
      }

      const double quantity = std::min(remainingDemand(n), remainingCap(source));
      flow(source, n) += quantity;          // (3), (4)
      remainingDemand(n) -= quantity;
      remainingCap(source) -= quantity;

      ++iter;
      if (iterCap < iter) {
        throw std::runtime_error("greedyPlan: iteration cap exceeded.");
      }
    }

    Plan plan;
    plan.flow = flow;
    plan.supplied = flow.rowwise().sum();
    plan.resupply = flow.colwise().sum().transpose();
    result.plan = plan;
    result.tonMilesUsed = tonMiles(inst, plan);
    result.suggestedLimit = params.budgetFraction * result.tonMilesUsed;
    result.iterations = iter;
    return result;
  }

} // namespace VINCP::Network
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------

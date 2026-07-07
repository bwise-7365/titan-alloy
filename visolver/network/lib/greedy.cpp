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
  waterFillTargets(const VectorXd& demand, const VectorXd& priority,
                   double meetable)
  {
    if (demand.size() != priority.size()) {
      throw std::invalid_argument(
          "waterFillTargets: demand and priority sizes must match.");
    }
    // Rationing weight D_i^2 / P_i of entry i (only evaluated where D_i > 0,
    // so callers' positive-priority-at-demand precondition keeps it finite).
    const auto rationWeight = [&demand, &priority](Index i) {
      return demand(i) * demand(i) / priority(i);
    };

    VectorXd targets = VectorXd::Zero(demand.size());
    if (0.0 >= meetable) {
      return targets;                       // nothing can move (or no demand)
    }
    if (demand.sum() <= meetable) {
      targets = demand;                     // all demand meetable (G1 boundary
      return targets;                       // included: equality lands here)
    }

    // Shortfall: water-filling with the exclude-and-resolve clamp (G2). Each
    // round solves lambda over the active set; entries whose interior R_i
    // comes out negative are fixed to 0 and the round repeats. The active set
    // strictly shrinks, so at most |V_D| rounds.
    vector<Index> active;
    for (Index i = 0; i < demand.size(); ++i) {
      if (0.0 < demand(i)) {
        active.push_back(i);
      }
    }
    while (!active.empty()) {
      double activeDemand = 0.0;
      double activeWeight = 0.0;
      for (const Index i : active) {
        activeDemand += demand(i);
        activeWeight += rationWeight(i);
      }
      const double lambda = (activeDemand - meetable) / activeWeight;
      if (0.0 >= lambda) {
        // Remaining active demand fits under the cap: no rationing among them.
        for (const Index i : active) {
          targets(i) = demand(i);
        }
        return targets;
      }

      bool clampedP = false;
      vector<Index> survivors;
      for (const Index i : active) {
        if (0.0 > demand(i) - lambda * rationWeight(i)) {
          clampedP = true;                  // excluded: stays at target 0
        }
        else {
          survivors.push_back(i);
        }
      }
      if (!clampedP) {
        for (const Index i : active) {
          targets(i) = demand(i) - lambda * rationWeight(i);
        }
        return targets;
      }
      active = survivors;
    }
    // Unreachable: within a round the interior R_i sum to meetable > 0, so
    // they cannot all be negative. Guard per the throw-never-substitute stance.
    throw std::runtime_error(
        "waterFillTargets: clamping emptied the active set.");
  }

  VectorXd
  rationTargets(const Instance& inst)
  {
    validateInstance(inst);
    const double meetable =
        std::min(totalDemand(inst), totalSupplyCap(inst));
    return waterFillTargets(inst.demand, inst.priority, meetable);
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

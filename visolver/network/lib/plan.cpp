// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
// Plan evaluation and feasibility checking.
// ----------------------------------------------
#include "plan.hpp"

#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace VINCP::Network {

  namespace {

    void
    requireShapes(const Instance& inst, const Plan& plan)
    {
      const Index m = inst.numNodes;
      const bool okP = plan.supplied.size() == m && plan.resupply.size() == m
                       && plan.flow.rows() == m && plan.flow.cols() == m;
      if (!okP) {
        throw std::invalid_argument(
            "Network::checkPlan: plan shapes do not match the instance.");
      }
      return;
    }

  } // namespace

  Plan
  makeZeroPlan(const Instance& inst)
  {
    validateInstance(inst);
    Plan plan;
    plan.supplied = VectorXd::Zero(inst.numNodes);
    plan.resupply = VectorXd::Zero(inst.numNodes);
    plan.flow = MatrixXd::Zero(inst.numNodes, inst.numNodes);
    return plan;
  }

  double
  tonMiles(const Instance& inst, const Plan& plan)
  {
    requireShapes(inst, plan);
    return (inst.cost.array() * plan.flow.array()).sum();
  }

  double
  shortfallVsTarget(const Instance& inst, const VectorXd& target,
                    const VectorXd& resupply)
  {
    if (target.size() != inst.numNodes || resupply.size() != inst.numNodes) {
      throw std::invalid_argument(
          "Network::shortfallVsTarget: target/resupply size must equal "
          "numNodes.");
    }
    double objective = 0.0;
    for (Index i = 0; i < inst.numNodes; ++i) {
      if (0.0 < inst.demand(i)) {
        const double fraction = (target(i) - resupply(i)) / inst.demand(i);
        objective += inst.priority(i) * fraction * fraction;
      }
    }
    return objective;
  }

  double
  shortfallOfResupply(const Instance& inst, const VectorXd& resupply)
  {
    return shortfallVsTarget(inst, inst.demand, resupply);
  }

  double
  shortfallObjective(const Instance& inst, const Plan& plan)
  {
    requireShapes(inst, plan);
    return shortfallOfResupply(inst, plan.resupply);
  }

  PlanViolations
  checkPlan(const Instance& inst, const Plan& plan)
  {
    requireShapes(inst, plan);
    PlanViolations violations;

    for (Index j = 0; j < inst.numNodes; ++j) {
      const double inflow = plan.flow.col(j).sum();
      const double outflow = plan.flow.row(j).sum();

      const double balanceResidual =
          inflow + plan.supplied(j) - plan.resupply(j) - outflow;
      violations.balance = std::max(violations.balance,
                                    std::abs(balanceResidual));
      violations.delivery = std::max(violations.delivery,
                                     plan.resupply(j) - inflow);
      violations.capacity = std::max(violations.capacity,
                                     plan.supplied(j) - inst.supplyCap(j));
      violations.negativity = std::max(violations.negativity,
                                       -plan.supplied(j));
      violations.negativity = std::max(violations.negativity,
                                       -plan.resupply(j));
      if (0.0 == inst.demand(j)) {
        violations.idleResupply = std::max(violations.idleResupply,
                                           plan.resupply(j));
      }
    }
    violations.negativity = std::max(violations.negativity,
                                     -plan.flow.minCoeff());

    if (0.0 < inst.tonMileLimit) {
      violations.budget = std::max(0.0,
                                   tonMiles(inst, plan) - inst.tonMileLimit);
    }

    // Clamp families that are differences to non-negative magnitudes.
    violations.delivery = std::max(violations.delivery, 0.0);
    violations.capacity = std::max(violations.capacity, 0.0);
    violations.negativity = std::max(violations.negativity, 0.0);
    violations.idleResupply = std::max(violations.idleResupply, 0.0);
    return violations;
  }

  double
  maxViolation(const PlanViolations& violations)
  {
    double worst = violations.balance;
    worst = std::max(worst, violations.delivery);
    worst = std::max(worst, violations.capacity);
    worst = std::max(worst, violations.negativity);
    worst = std::max(worst, violations.idleResupply);
    worst = std::max(worst, violations.budget);
    return worst;
  }

} // namespace VINCP::Network
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------

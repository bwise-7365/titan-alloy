// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
// Gravity planner implementation: one outer product, normalized by the larger
// of total supply / total demand.
// ----------------------------------------------
#include "gravity.hpp"

#include <algorithm>

namespace VIMCP::Network {

  Plan
  gravityPlan(const Instance& inst)
  {
    validateInstance(inst);
    Plan plan = makeZeroPlan(inst);
    const double normalizer =
        std::max(totalSupplyCap(inst), totalDemand(inst));
    if (normalizer > 0.0) {
      // f_ij = C_i * D_j / normalizer is the outer product C D^T scaled. Rows of
      // non-suppliers and columns of non-demand nodes are zero automatically.
      plan.flow = (inst.supplyCap * inst.demand.transpose()) / normalizer;
      plan.supplied = plan.flow.rowwise().sum();
      plan.resupply = plan.flow.colwise().sum().transpose();
    }
    return plan;
  }

} // namespace VIMCP::Network
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------

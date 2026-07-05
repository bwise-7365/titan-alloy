// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
// The gravity (proportional all-to-all) planner: a crude, cost-blind baseline.
// ----------------------------------------------
#ifndef VINCP_NETWORK_GRAVITY_HPP
#define VINCP_NETWORK_GRAVITY_HPP

#include "plan.hpp"

namespace VINCP::Network {

  // Gravity plan: f_ij = C_i * D_j / max(totalC, totalD), for every (i, j).
  // DENSE (every source ships to every sink) and COST-BLIND -- a deliberately
  // inefficient baseline. Feasible by construction: each supplier ships
  // C_i * totalD/max <= C_i and each sink receives totalC/max * D_j <= D_j, so
  // whichever side is short is proportionally rationed, and the total moved is
  // min(totalC, totalD) -- the meetable (summed-rationed) amount.
  Plan gravityPlan(const Instance& inst);

} // namespace VINCP::Network

#endif // VINCP_NETWORK_GRAVITY_HPP
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------

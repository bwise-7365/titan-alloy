// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
// Shared test fixture: a gentle-scale 5-node random instance the dense
// full-formulation oracle can afford (used by oracle_test and flowplan_test).
// ----------------------------------------------
#ifndef VINCP_NETWORK_GENTLESUPPORT_HPP
#define VINCP_NETWORK_GENTLESUPPORT_HPP

#include "greedy.hpp"

#include <cstdint>

namespace VINCP::Network::TestSupport {

  // 5 nodes (2 supply-only, 1 both, 1 demand-only, 1 transit), single-digit
  // tonnages, two-digit costs; budget calibrated by the greedy planner.
  inline Instance
  makeGentleInstance(std::uint64_t seed)
  {
    InstanceProfile profile;
    profile.numSupplyOnly = 2;
    profile.numBoth = 1;
    profile.numDemandOnly = 1;
    profile.numNeither = 1;
    profile.supplyLo = 2.0;
    profile.supplyHi = 10.0;
    profile.demandLo = 2.0;
    profile.demandHi = 10.0;
    profile.squareSide = 100.0;
    profile.costFloor = 10.0;
    profile.milesPerUnit = 1.0;
    Instance inst = makeRandomInstance(profile, seed);
    inst.tonMileLimit = greedyPlan(inst).suggestedLimit;
    return inst;
  }

} // namespace VINCP::Network::TestSupport

#endif // VINCP_NETWORK_GENTLESUPPORT_HPP
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------

// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
// Shared test fixture: the two-node single-lane instance whose KKT points are
// hand-computable (used by flowlcp_test and oracle_test).
// ----------------------------------------------
#ifndef VINCP_NETWORK_LANESUPPORT_HPP
#define VINCP_NETWORK_LANESUPPORT_HPP

#include "instance.hpp"

namespace VINCP::Network::TestSupport {

  // One source (node 0) and one sink (node 1) joined by one shipping lane:
  // d-hat(0 -> 1) = laneMiles, so t* = laneDemand - eps*laneMiles*D^2/(2P)
  // when nothing binds and t = L/laneMiles when the budget does.
  inline constexpr double laneCap = 100.0;
  inline constexpr double laneDemand = 8.0;
  inline constexpr double lanePriority = 2.0;
  inline constexpr double laneMiles = 500.0;

  inline Instance
  makeLaneInstance(double tonMileLimit)
  {
    Instance inst;
    inst.numNodes = 2;
    inst.supplyCap = VectorXd(2);
    inst.supplyCap << laneCap, 0.0;
    inst.demand = VectorXd(2);
    inst.demand << 0.0, laneDemand;
    inst.priority = VectorXd(2);
    inst.priority << 1.0, lanePriority;
    inst.cost = MatrixXd(2, 2);
    inst.cost << 2.0, laneMiles,
                 520.0, 3.0;
    inst.tonMileLimit = tonMileLimit;
    validateInstance(inst);
    return inst;
  }

} // namespace VINCP::Network::TestSupport

#endif // VINCP_NETWORK_LANESUPPORT_HPP
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------

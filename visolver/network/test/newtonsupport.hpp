// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
// Shared helpers for the structured-Newton parity tests (flownewton_test,
// fleetnewton_test): deterministic complementarity diagonals and right-hand
// sides, and the scale-aware backward error.
// ----------------------------------------------
#ifndef VIMCP_NETWORK_TEST_NEWTONSUPPORT_HPP
#define VIMCP_NETWORK_TEST_NEWTONSUPPORT_HPP

#include "vimcp.hpp"

#include <cmath>

namespace VIMCP::Network::TestSupport {

  // Deterministic sOverY sweeping the given log10 range across the vector:
  // entry i gets 10^(lo + (hi - lo) * i / (dim - 1)), scrambled by stride
  // so neighboring slots differ sharply (as real complementarity diagonals
  // do near a degenerate face).
  inline VectorXd
  makeSpreadDiagonal(Index dim, double log10Lo, double log10Hi)
  {
    VectorXd s(dim);
    const Index stride = 7;                  // coprime with typical dims
    for (Index i = 0; i < dim; ++i) {
      const Index slot = (i * stride) % dim;
      const double frac =
          (1 == dim) ? 0.0
                     : static_cast<double>(slot) / static_cast<double>(dim - 1);
      s(i) = std::pow(10.0, log10Lo + (log10Hi - log10Lo) * frac);
    }
    return s;
  }

  // Deterministic rhs with mixed signs and scales.
  inline VectorXd
  makeRhs(Index dim)
  {
    VectorXd rhs(dim);
    for (Index i = 0; i < dim; ++i) {
      const double sign = (0 == i % 2) ? 1.0 : -1.0;
      rhs(i) = sign * (1.0 + static_cast<double>(i % 13));
    }
    return rhs;
  }

  // Scale-aware backward error ||K d - rhs|| / (||K||_inf ||d|| + ||rhs||):
  // the parity metric that stays valid at any conditioning.
  inline double
  backwardError(const MatrixXd& K, const VectorXd& d, const VectorXd& rhs)
  {
    const double scale = K.cwiseAbs().rowwise().sum().maxCoeff() * d.norm()
                         + rhs.norm();
    return (K * d - rhs).norm() / scale;
  }

} // namespace VIMCP::Network::TestSupport

#endif // VIMCP_NETWORK_TEST_NEWTONSUPPORT_HPP
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------

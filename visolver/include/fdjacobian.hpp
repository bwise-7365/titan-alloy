// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
// Finite-difference Jacobian for the real-valued VI port.
// ----------------------------------------------
#ifndef VINCP_FDJACOBIAN_HPP
#define VINCP_FDJACOBIAN_HPP

// The Octave original computed Jacobians with the optim-package complex-step
// method.  The C++ port is real-valued, so the outer Josephy-Newton driver
// linearizes F with a central finite difference instead.  A 4th-order central
// stencil (step of order eps^(1/5)) drives the truncation error down to about
// eps^(4/5), roughly two orders of magnitude below a 2nd-order central scheme --
// enough to reach tight outer tolerances on stiff (e.g. cubic) maps.

#include "vincp.hpp"   // Eigen type aliases (VectorXd, MatrixXd, ...) live here

#include <Eigen/Dense>
#include <functional>

namespace VINCP {

  // A vector field R^d -> R^p whose Jacobian is to be approximated.
  using VectorField = std::function<VectorXd(const VectorXd&)>;

  // Central-difference Jacobian of F at z (4th-order accurate).
  //
  // Column j uses the 4th-order central stencil
  //   [ F(z-2h e_j) - 8 F(z-h e_j) + 8 F(z+h e_j) - F(z+2h e_j) ] / (12 h),
  // with a per-component step  h_j = stepRel * max(|z_j|, 1)  snapped to an exactly
  // representable value to limit cancellation error.  Pass stepRel <= 0 to use the
  // default eps^(1/5), the accuracy-optimal scale for this stencil.
  //
  // The output dimension p is taken from F(z); F need not be square (it is not,
  // for a sub-block, though for the VI driver d = p = n + m).
  //
  // Throws std::invalid_argument if F is unset or z is empty, and
  // std::runtime_error if any evaluation of F is non-finite or returns a
  // different length than F(z).  It never silently substitutes a value.
  MatrixXd centralDifferenceJacobian(const VectorField& F,
                                     const VectorXd& z,
                                     double stepRel = -1.0);

} // namespace VINCP

#endif // VINCP_FDJACOBIAN_HPP
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------

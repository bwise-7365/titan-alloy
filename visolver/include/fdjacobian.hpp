#ifndef VINCP_FDJACOBIAN_HPP
#define VINCP_FDJACOBIAN_HPP

// ============================================================================
// Finite-difference Jacobian for the real-valued VI port.
//
// The Octave original computed Jacobians with the optim-package complex-step
// method.  The C++ port is real-valued, so the outer Josephy-Newton driver
// linearizes F with a central finite difference instead.  Central differencing
// with a step of order eps^(1/3) is the accuracy-matched real-arithmetic
// analogue of that complex-step Jacobian.
// ============================================================================

#include <Eigen/Dense>
#include <functional>

namespace VINCP {

    using Eigen::VectorXd;

// A vector field R^d -> R^p whose Jacobian is to be approximated.
using VectorField = std::function<VectorXd(const VectorXd&)>;

// Central-difference Jacobian of F at z.
//
// Column j is  (F(z + h_j e_j) - F(z - h_j e_j)) / (2 h_j),  with a per-
// component step  h_j = stepRel * max(|z_j|, 1)  that is then snapped to an
// exactly representable width to limit cancellation error.  Pass stepRel <= 0
// to use the default eps^(1/3), the accuracy-optimal scale for this scheme.
//
// The output dimension p is taken from F(z); F need not be square (it is not,
// for a sub-block, though for the VI driver d = p = n + m).
//
// Throws std::invalid_argument if F is unset or z is empty, and
// std::runtime_error if any evaluation of F is non-finite or returns a
// different length than F(z).  It never silently substitutes a value.
Eigen::MatrixXd centralDifferenceJacobian(const VectorField& F,
                                          const VectorXd& z,
                                          double stepRel = -1.0);

} // namespace VINCP

#endif // VINCP_FDJACOBIAN_HPP

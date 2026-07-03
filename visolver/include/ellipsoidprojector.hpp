// Copyright Ben Paul Wise. All Rights Reserved.
#ifndef VINCP_ELLIPSOIDPROJECTOR_HPP
#define VINCP_ELLIPSOIDPROJECTOR_HPP

// ============================================================================
// Euclidean projection onto an axis-aligned, origin-centered solid ellipsoid
//
//     E(r) = { x in R^n : sum_i (x_i / r_i)^2 <= 1 },   every r_i > 0,
//
// a C++/Eigen port of the GNU Octave routines eNorm.m / eProj.m.
//
// The projection of a point y onto E(r) is y itself when y is inside; otherwise it
// is the boundary point
//     x_i = r_i^2 y_i / (r_i^2 + lambda),
// where the KKT multiplier lambda > 0 is the unique root of
//     sum_i ( r_i y_i / (r_i^2 + lambda) )^2 = 1.
// The left side is strictly decreasing in lambda, so the root is bracketed and
// found by a hybrid of regula-falsi (for speed) and bisection (for guaranteed
// bracket halving), matching the Octave reference.
//
// makeEllipsoidProjector returns a Projector (vincp.hpp) so E(r) can serve as the
// feasible set K for the VI/NCP solvers -- e.g. minimizing a linear objective over
// an ellipsoid.
// ============================================================================

#include "vincp.hpp"

#include <Eigen/Dense>

namespace VINCP {

// Ellipsoid "norm" of x for E(r): sqrt(sum_i (x_i / r_i)^2). x is inside E(r) iff
// this is <= 1. Throws std::invalid_argument on a size mismatch, an empty input, or
// a non-positive radius.
double ellipsoidNorm(const VectorXd& x, const VectorXd& radii);

// Euclidean projection of y onto the solid ellipsoid E(radii). Returns y unchanged
// when it is already inside. 'tol' bounds the boundary residual |ellipsoidNorm(x) - 1|
// of the returned point; 'iterMax' caps the root-find. Throws std::invalid_argument
// on a size mismatch, an empty input, a non-positive radius, or non-positive
// tol / iterMax.
VectorXd projectEllipsoid(const VectorXd& y, const VectorXd& radii,
                          double tol = 1.0e-10, int iterMax = 5000);

// Build a Projector onto E(radii) for use as the feasible set K in the VI solvers.
// The radii are captured by value and validated up front (throws on empty or a
// non-positive radius), so the returned functor never fails on bad radii.
Projector makeEllipsoidProjector(const VectorXd& radii,
                                 double tol = 1.0e-10, int iterMax = 5000);

} // namespace VINCP

#endif // VINCP_ELLIPSOIDPROJECTOR_HPP
// Copyright Ben Paul Wise. All Rights Reserved.

// Copyright Ben Paul Wise. All Rights Reserved.
#ifndef VINCP_DAMPEDNEWTON_HPP
#define VINCP_DAMPEDNEWTON_HPP

// ============================================================================
// Smoothed damped-Newton solver for nonlinear least squares -- a C++/Eigen port of
// the GNU Octave dNewton.m.
//
// For F: R^n -> R^m (any m, n) it minimizes 1/2 ||F(x)||^2 by, at each iterate,
// blending the Gauss-Newton normal-equations (LLSE) step with the properly-scaled
// steepest-descent step, then backtracking the step length by Armijo:
//
//     ( theta J^T J + (1 - theta) alpha I ) d = -J^T F,
//     alpha = (1/2 ||F||^2) / ||J^T F||^2          (the Cauchy / steepest-descent scale),
//     x <- x + (Armijo step length) * d.
//
// theta in (0, 1) is a fixed smoothing weight: theta -> 1 approaches pure Gauss-Newton
// (the normal-equations step, valid for rectangular J), theta -> 0 approaches pure
// scaled gradient descent. Only J^T J and J^T F appear, so J need not be square.
//
// It reuses the shared building blocks levenbergMarquardtDamp (J^T J + lambda I) and
// armijoLineSearch, plus the finite-difference Jacobian.
// ============================================================================

#include "vincp.hpp"
#include "fdjacobian.hpp"
#include "armijo.hpp"
#include "levenbergmarquardt.hpp"

#include <Eigen/Dense>

namespace VINCP {

struct DampedNewtonParams {
    double meritTol  = 1.0e-16;   // stop when ||F(x)||^2 < meritTol (SQUARED norm)
    int    iterMax   = 200;       // outer iteration cap
    double theta     = 0.618034;  // smoothing weight in (0, 1); see the header above
    double fdStepRel = -1.0;      // finite-difference Jacobian step (<= 0 => default)
    // Armijo backtracking on the step length (dNewton.m: start 0.75, shrink 0.8, cap 70).
    ArmijoParams armijo = ArmijoParams{ 0.75, 0.8, 1.0e-4, 70 };
};

// Solve min 1/2 ||F(x)||^2 by the smoothed damped-Newton method above. Returns the
// shared VIResult: 'z' the solution, 'residual' the final ||F||^2 (SQUARED norm),
// 'converged' iff it fell below meritTol within iterMax steps.
//
// Throws std::invalid_argument on an unset F, empty x0, or theta not in (0, 1), and
// std::runtime_error if F(x0) is non-finite. It stops with converged = false (rather
// than throwing) when the gradient vanishes at a non-root or the line search stalls.
VIResult dampedNewtonSolve(const VectorField& F,
                           const VectorXd& x0,
                           const DampedNewtonParams& params = DampedNewtonParams{});

} // namespace VINCP

#endif // VINCP_DAMPEDNEWTON_HPP
// Copyright Ben Paul Wise. All Rights Reserved.

// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
// Levenberg-Marquardt for nonlinear least squares (building blocks + solver).
// ----------------------------------------------
#ifndef VIMCP_LEVENBERGMARQUARDT_HPP
#define VIMCP_LEVENBERGMARQUARDT_HPP

// Two layers:
//   - Building blocks: the damped normal-equations operator  J^T J + lambda I  and
//     a policy for adapting lambda between iterations. Reusable by any solver (the
//     Josephy-Newton driver does NOT use them; it globalizes with an Armijo line
//     search, armijo.hpp).
//   - levenbergMarquardtSolve: a self-contained LM solver for
//     min 1/2 ||F(x)||^2 over F: R^n -> R^m (any m, n; the normal equations use
//     only J^T J and J^T F, so the Jacobian need not be square), built on those
//     blocks and a finite-difference Jacobian. Returns the shared VIResult.

#include "vimcp.hpp"
#include "fdjacobian.hpp"

#include <Eigen/Dense>

namespace VIMCP {

  struct LevenbergMarquardtParams {
    double lambda0   = 1.0e-3;    // initial damping
    double increase  = 10.0;      // grow lambda after a rejected trial step
    double decrease  = 0.1;       // shrink lambda after an accepted trial step
    double lambdaMin = 1.0e-12;   // clamp (toward the pure Newton step)
    double lambdaMax = 1.0e+12;   // clamp (toward a short gradient-like step)
  };

  // The damped Gauss-Newton normal-equations operator J^T J + lambda I (n x n) for a
  // possibly-rectangular m x n Jacobian J -- a square J is just the special case
  // m = n. Throws std::invalid_argument if lambda < 0.
  MatrixXd levenbergMarquardtDamp(const MatrixXd& J, double lambda);

  // Adapt the damping: shrink toward Newton on an accepted step, grow toward a
  // short, more conservative step on a rejected one, clamped to [lambdaMin,
  // lambdaMax].
  double levenbergMarquardtUpdate(double lambda, bool stepAccepted,
                                  const LevenbergMarquardtParams& params = LevenbergMarquardtParams{});

  // Controls for levenbergMarquardtSolve.
  struct LevenbergMarquardtSolveParams {
    double meritTol  = 1.0e-16;  // stop when ||F(x)||^2 < meritTol (SQUARED norm)
    int    iterMax   = 200;      // outer iteration cap
    int    innerMax  = 40;       // max lambda increases per outer step
    double fdStepRel = -1.0;     // finite-difference Jacobian step (<= 0 => default)
    LevenbergMarquardtParams lambda = LevenbergMarquardtParams{};  // damping schedule
  };

  // Solve min 1/2 ||F(x)||^2 by damped Gauss-Newton (Levenberg-Marquardt): at each
  // iterate form the FD Jacobian J, solve (J^T J + lambda I) dx = -J^T F, and
  // accept the step if it lowers ||F||^2 (else grow lambda and retry). Returns the
  // shared VIResult: 'z' is the solution, 'residual' is ||F(x)||^2 at termination,
  // 'converged' is true iff that fell below meritTol within iterMax steps.
  //
  // Throws std::invalid_argument on an unset F or empty x0, and std::runtime_error
  // if F(x0) is non-finite. It never silently substitutes a result.
  VIResult levenbergMarquardtSolve(const VectorField& F,
                                   const VectorXd& x0,
                                   const LevenbergMarquardtSolveParams& params = LevenbergMarquardtSolveParams{});

} // namespace VIMCP

#endif // VIMCP_LEVENBERGMARQUARDT_HPP
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------

// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
// Smoothed damped-Newton solver implementation (port of dNewton.m).
// ----------------------------------------------
#include "dampednewton.hpp"

#include <stdexcept>

namespace VINCP {

  VIResult
  dampedNewtonSolve(const VectorField& F,
                    const VectorXd& x0,
                    const DampedNewtonParams& params)
  {
    if (!F) {
      throw std::invalid_argument("dampedNewtonSolve: F must be set.");
    }
    if (0 >= x0.size()) {
      throw std::invalid_argument("dampedNewtonSolve: x0 must be non-empty.");
    }
    if (!(0.0 < params.theta && params.theta < 1.0)) {
      throw std::invalid_argument("dampedNewtonSolve: theta must lie in (0, 1).");
    }

    VectorXd x = x0;
    VectorXd Fx = F(x);
    if (!Fx.allFinite()) {
      throw std::runtime_error("dampedNewtonSolve: F(x0) is non-finite.");
    }
    double merit = Fx.squaredNorm();
    int iter = 0;
    bool converged = false;

    while (iter < params.iterMax) {
      if (merit < params.meritTol) {
        converged = true;
        break;
      }

      // Normal-equation pieces from the FD Jacobian (J is m x n; only J^T J and
      // J^T F appear, so any m, n is fine).
      const MatrixXd J = centralDifferenceJacobian(F, x, params.fdStepRel);
      const VectorXd grad = J.transpose() * Fx;   // gradient of 1/2 ||F||^2
      const double gg = grad.squaredNorm();
      if (0.0 == gg) {
        break;   // gradient vanished at a non-root: cannot descend
      }

      // Cauchy (steepest-descent) step scale, then the smoothed damped operator
      //   theta J^T J + (1-theta) alpha I = theta * ( J^T J + ((1-theta)/theta) alpha I ),
      // reusing the LM normal-equations block for the parenthesized part. op is PD
      // (a positive multiple of J^T J + positive*I), so ldlt is valid.
      const double alpha    = (0.5 * merit) / gg;
      const double lmLambda = ((1.0 - params.theta) / params.theta) * alpha;
      const MatrixXd op = params.theta * levenbergMarquardtDamp(J, lmLambda);
      const VectorXd d  = op.ldlt().solve(-grad);

      // Armijo backtracking on the step length, over the merit ||F(x + step d)||^2.
      const auto meritAt = [&](double step) -> double {
        return F(x + step * d).squaredNorm();
      };
      const ArmijoResult ls = armijoLineSearch(meritAt, merit, params.armijo);
      if (!(ls.merit < merit)) {
        break;   // no decrease found (or a non-finite trial): stalled
      }

      x += ls.alpha * d;
      Fx = F(x);
      merit = Fx.squaredNorm();
      ++iter;
    }

    if (merit < params.meritTol) {
      converged = true;
    }
    return VIResult{ x, merit, iter, converged };
  }

} // namespace VINCP
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------

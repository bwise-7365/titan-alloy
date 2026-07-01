// Copyright Ben Paul Wise. All Rights Reserved.
#ifndef VINCP_LEVENBERGMARQUARDT_HPP
#define VINCP_LEVENBERGMARQUARDT_HPP

// ============================================================================
// Levenberg-Marquardt regularization -- a reusable building block for future
// solvers (e.g. a regularized/trust-region Newton for indefinite or singular
// Jacobians). It is intentionally NOT used by the current Josephy-Newton driver,
// which globalizes with an Armijo line search instead (armijo.hpp).
//
// It provides the damped operator  J + lambda I  and a policy for adapting
// lambda between iterations.
// ============================================================================

#include <Eigen/Dense>

namespace VINCP {

struct LevenbergMarquardtParams {
    double lambda0   = 1.0e-3;    // initial damping
    double increase  = 10.0;      // grow lambda after a rejected trial step
    double decrease  = 0.1;       // shrink lambda after an accepted trial step
    double lambdaMin = 1.0e-12;   // clamp (toward the pure Newton step)
    double lambdaMax = 1.0e+12;   // clamp (toward a short gradient-like step)
};

// Return J + lambda I (J must be square, lambda >= 0). Throws otherwise.
Eigen::MatrixXd levenbergMarquardtDamp(const Eigen::MatrixXd& J, double lambda);

// Adapt the damping: shrink toward Newton on an accepted step, grow toward a
// short, more conservative step on a rejected one, clamped to [lambdaMin,
// lambdaMax].
double levenbergMarquardtUpdate(double lambda, bool stepAccepted,
                                const LevenbergMarquardtParams& params = LevenbergMarquardtParams{});

} // namespace VINCP

#endif // VINCP_LEVENBERGMARQUARDT_HPP
// Copyright Ben Paul Wise. All Rights Reserved.

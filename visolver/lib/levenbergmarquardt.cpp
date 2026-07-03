// Copyright Ben Paul Wise. All Rights Reserved.
#include "levenbergmarquardt.hpp"

#include <algorithm>
#include <stdexcept>

namespace VINCP {

MatrixXd levenbergMarquardtDamp(const MatrixXd& J, double lambda) {
    if (lambda < 0.0) {
        throw std::invalid_argument("levenbergMarquardtDamp: lambda must be non-negative.");
    }
    // The damped Gauss-Newton normal-equations operator J^T J + lambda I (n x n) for
    // any m x n Jacobian J. Only J^T J -- always n x n and PSD -- appears, so there
    // is no squareness requirement; a square J is the special case m = n.
    const Index n = J.cols();
    return J.transpose() * J + lambda * MatrixXd::Identity(n, n);
}

double levenbergMarquardtUpdate(double lambda, bool stepAccepted,
                                const LevenbergMarquardtParams& params) {
    const double updated = stepAccepted ? (lambda * params.decrease)
                                        : (lambda * params.increase);
    return std::clamp(updated, params.lambdaMin, params.lambdaMax);
}

VIResult levenbergMarquardtSolve(const VectorField& F,
                                 const VectorXd& x0,
                                 const LevenbergMarquardtSolveParams& params) {
    if (!F) {
        throw std::invalid_argument("levenbergMarquardtSolve: F must be set.");
    }
    if (x0.size() <= 0) {
        throw std::invalid_argument("levenbergMarquardtSolve: x0 must be non-empty.");
    }

    VectorXd x = x0;
    VectorXd Fx = F(x);
    if (!Fx.allFinite()) {
        throw std::runtime_error("levenbergMarquardtSolve: F(x0) is non-finite.");
    }
    double merit = Fx.squaredNorm();
    double lambda = params.lambda.lambda0;
    int iter = 0;
    bool converged = false;

    while (iter < params.iterMax) {
        if (merit < params.meritTol) {
            converged = true;
            break;
        }

        // Gauss-Newton normal-equation pieces from the FD Jacobian. J is m x n for
        // F: R^n -> R^m; only J^T J (via Damp) and J^T F appear, so any m, n is fine.
        const MatrixXd J = centralDifferenceJacobian(F, x, params.fdStepRel);
        const VectorXd grad = J.transpose() * Fx; // grad of 1/2 ||F||^2

        // Grow lambda until a damped step reduces the merit (or give up).
        bool stepAccepted = false;
        for (int inner = 0; inner < params.innerMax; ++inner) {
            const MatrixXd damped = levenbergMarquardtDamp(J, lambda);  // J^T J + lambda I
            const VectorXd dx = damped.ldlt().solve(-grad);
            const VectorXd xTrial = x + dx;
            const VectorXd FTrial = F(xTrial);
            const double meritTrial = FTrial.squaredNorm();
            // A non-finite trial yields a NaN/Inf merit, which fails this test
            // and is rejected below -- the damping simply grows.
            if (meritTrial < merit) {
                x = xTrial;
                Fx = FTrial;
                merit = meritTrial;
                lambda = levenbergMarquardtUpdate(lambda, true, params.lambda);
                stepAccepted = true;
                break;
            }
            lambda = levenbergMarquardtUpdate(lambda, false, params.lambda);
        }

        ++iter;
        if (!stepAccepted) {
            break;   // no decrease even at maximum damping: stuck
        }
    }

    if (merit < params.meritTol) {
        converged = true;
    }
    return VIResult{ x, merit, iter, converged };
}

} // namespace VINCP
// Copyright Ben Paul Wise. All Rights Reserved.

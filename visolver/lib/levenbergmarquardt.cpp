// Copyright Ben Paul Wise. All Rights Reserved.
#include "levenbergmarquardt.hpp"

#include <algorithm>
#include <stdexcept>

namespace VINCP {

Eigen::MatrixXd levenbergMarquardtDamp(const Eigen::MatrixXd& J, double lambda) {
    if (J.rows() != J.cols()) {
        throw std::invalid_argument("levenbergMarquardtDamp: J must be square.");
    }
    if (lambda < 0.0) {
        throw std::invalid_argument("levenbergMarquardtDamp: lambda must be non-negative.");
    }
    return J + lambda * Eigen::MatrixXd::Identity(J.rows(), J.cols());
}

double levenbergMarquardtUpdate(double lambda, bool stepAccepted,
                                const LevenbergMarquardtParams& params) {
    const double updated = stepAccepted ? (lambda * params.decrease)
                                        : (lambda * params.increase);
    return std::clamp(updated, params.lambdaMin, params.lambdaMax);
}

VIResult levenbergMarquardtSolve(const VectorField& F,
                                 const Eigen::VectorXd& x0,
                                 const LevenbergMarquardtSolveParams& params) {
    if (!F) {
        throw std::invalid_argument("levenbergMarquardtSolve: F must be set.");
    }
    if (x0.size() <= 0) {
        throw std::invalid_argument("levenbergMarquardtSolve: x0 must be non-empty.");
    }

    Eigen::VectorXd x = x0;
    Eigen::VectorXd Fx = F(x);
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

        // Gauss-Newton normal-equation pieces from the FD Jacobian.
        const Eigen::MatrixXd J = centralDifferenceJacobian(F, x, params.fdStepRel);
        const Eigen::MatrixXd JtJ = J.transpose() * J;   // n x n, PSD
        const Eigen::VectorXd grad = J.transpose() * Fx; // grad of 1/2 ||F||^2

        // Grow lambda until a damped step reduces the merit (or give up).
        bool stepAccepted = false;
        for (int inner = 0; inner < params.innerMax; ++inner) {
            const Eigen::MatrixXd damped = levenbergMarquardtDamp(JtJ, lambda);
            const Eigen::VectorXd dx = damped.ldlt().solve(-grad);
            const Eigen::VectorXd xTrial = x + dx;
            const Eigen::VectorXd FTrial = F(xTrial);
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

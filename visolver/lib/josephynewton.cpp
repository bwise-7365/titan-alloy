// Copyright Ben Paul Wise. All Rights Reserved.
#include "josephynewton.hpp"

#include "fdjacobian.hpp"

#include <stdexcept>

namespace VINCP {

namespace {

void validateModel(const VIModel& model, const VectorXd& z0) {
    if (model.n < 0 || model.m < 0) {
        throw std::invalid_argument("solveVI: model.n and model.m must be non-negative.");
    }
    if (model.n + model.m <= 0) {
        throw std::invalid_argument("solveVI: model dimension n + m must be positive.");
    }
    if (z0.size() != model.n + model.m) {
        throw std::invalid_argument("solveVI: z0 length must equal n + m.");
    }
    if (!model.H) {
        throw std::invalid_argument("solveVI: model.H must be set.");
    }
    if (!model.G) {
        throw std::invalid_argument("solveVI: model.G must be set.");
    }
}

} // namespace

VIResult solveVI(const VIModel& model,
                 const VectorXd& z0,
                 const JosephyNewtonParams& params,
                 const OuterLogger& logger) {
    validateModel(model, z0);

    const Projector Pr = makeMixedProjector(model.n);

    // F as a single vector field for the finite-difference Jacobian.
    const VectorField F = [&model](const VectorXd& z) -> VectorXd {
        return evaluateF(model, z);
    };

    VectorXd z = z0;
    double residual = 0.0;
    int iter = 0;
    bool converged = false;

    while (true) {
        const VectorXd Fz = evaluateF(model, z);

        // Natural-residual merit r(z) = z - Pi_K(z - F(z)).
        const VectorXd r = z - Pr(z - Fz);
        residual = r.squaredNorm();

        if (params.outerIterFreq > 0 && (iter % params.outerIterFreq) == 0 && logger) {
            logger(iter, params.outerIterMax, residual, params.outerTol);
        }

        if (residual < params.outerTol) {
            converged = true;
            break;
        }
        if (iter >= params.outerIterMax) {
            break;
        }

        // Linearize: M = J(z), q = F(z) - J(z) z.
        const Eigen::MatrixXd jac = centralDifferenceJacobian(F, z, params.fdStepRel);
        const VectorXd q = Fz - jac * z;

        // Inner affine-VI solve over the same K gives the Josephy-Newton point.
        const VIResult inner = dHan06(z, jac, q, Pr,
                                      params.innerMagTol,
                                      params.innerIterMax,
                                      params.innerIterFreq,
                                      params.innerParams);

        // Damp the step with an Armijo line search on the natural-map merit
        // theta(w) = 1/2 ||w - Pi_K(w - F(w))||^2, so the undamped Newton step
        // cannot overshoot the non-smooth solution. 'residual' is ||r(z)||^2, so
        // theta at the base point is half of it.
        const Eigen::VectorXd stepDir = inner.z - z;
        const double theta0 = 0.5 * residual;
        const auto meritAt = [&](double alpha) -> double {
            const Eigen::VectorXd w = z + alpha * stepDir;
            const Eigen::VectorXd Fw = evaluateF(model, w);
            const Eigen::VectorXd rw = w - Pr(w - Fw);
            return 0.5 * rw.squaredNorm();
        };
        const ArmijoResult ls = armijoLineSearch(meritAt, theta0, params.armijo);
        z = z + ls.alpha * stepDir;
        ++iter;
    }

    return VIResult{ z, residual, iter, converged };
}

} // namespace VINCP
// Copyright Ben Paul Wise. All Rights Reserved.

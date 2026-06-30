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

        // Inner affine-VI solve over the same K; full Newton step.
        const VIResult inner = dHan06(z, jac, q, Pr,
                                      params.innerMagTol,
                                      params.innerIterMax,
                                      params.innerIterFreq,
                                      params.innerParams);
        z = inner.z;
        ++iter;
    }

    return VIResult{ z, residual, iter, converged };
}

} // namespace VINCP

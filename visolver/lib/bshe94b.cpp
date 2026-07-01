// Copyright Ben Paul Wise. All Rights Reserved.
#include "bshe94b.hpp"

#include <cmath>
#include <stdexcept>

namespace VINCP {

namespace {

void validateInputs(const VectorXd& x0,
                    const Eigen::MatrixXd& M,
                    const VectorXd& q,
                    const Projector& Pr,
                    const BsHe94bParams& params) {
    const Eigen::Index nd = x0.size();
    if (nd <= 0) {
        throw std::invalid_argument("bsHe94b: x0 must be non-empty.");
    }
    if (M.rows() != nd || M.cols() != nd) {
        throw std::invalid_argument("bsHe94b: M must be square and conformant with x0.");
    }
    if (q.size() != nd) {
        throw std::invalid_argument("bsHe94b: q must be conformant with x0.");
    }
    if (!Pr) {
        throw std::invalid_argument("bsHe94b: projector Pr must be set.");
    }
    if (!(params.gamma > 0.0 && params.gamma < 2.0)) {
        throw std::invalid_argument("bsHe94b: gamma must satisfy 0 < gamma < 2.");
    }
}

} // namespace

VIResult bsHe94b(const VectorXd& x0,
                 const Eigen::MatrixXd& M,
                 const VectorXd& q,
                 const Projector& Pr,
                 double magTol,
                 int iterMax,
                 int iterFreq,
                 const BsHe94bParams& params,
                 const IterationLogger& logger) {
    validateInputs(x0, M, q, Pr, params);

    const Eigen::Index nd = x0.size();

    // Fixed metric: factor (M + I) once and reuse it every iteration. The Octave
    // formed the explicit inverse; storing the LU factorization is the numerically
    // sounder equivalent with the same precompute-once advantage.
    const Eigen::MatrixXd identity = Eigen::MatrixXd::Identity(nd, nd);
    const Eigen::PartialPivLU<Eigen::MatrixXd> luMI = (M + identity).partialPivLu();

    VectorXd x = x0;

    double mag = magTol + 1.0;
    int iter = 0;
    bool doneP = false;
    double initMag = -1.0;

    while (!doneP) {
        // Equation (4): the residual map e = x - Pr(x - (M x + q)).
        const VectorXd e = x - Pr(x - (M * x + q));
        mag = e.squaredNorm();

        if (initMag < 0.0) {
            initMag = mag;
        }

        // Raise errors on NaN or out-of-control divergence, rather than
        // limping along with a corrupt iterate.
        if (std::isnan(mag)) {
            throw std::runtime_error("bsHe94b: residual magnitude is NaN.");
        }
        if (!(mag < 1.0 + params.divergenceFactor * initMag)) {
            throw std::runtime_error("bsHe94b: divergence detected; residual exceeded the guard.");
        }

        if (iterFreq > 0 && (iter % iterFreq) == 0 && logger) {
            logger(iter, iterMax, mag, magTol);
        }

        // Equation (16): x <- x - gamma (M + I)^{-1} e.
        const VectorXd step = luMI.solve(e);
        if (!step.allFinite()) {
            throw std::runtime_error("bsHe94b: solve against (M + I) produced non-finite values.");
        }
        x = x - params.gamma * step;

        ++iter;
        doneP = (mag < magTol) || (iterMax < iter);
    }

    return VIResult{ x, mag, iter, mag < magTol };
}

} // namespace VINCP
// Copyright Ben Paul Wise. All Rights Reserved.

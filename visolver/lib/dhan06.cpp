// Copyright Ben Paul Wise. All Rights Reserved.
#include "dhan06.hpp"

#include <cmath>
#include <stdexcept>

namespace VINCP {

double tau(double t0, int n, int k) {
    double tk = t0;
    if (n < k) {
        const double nn = static_cast<double>(n) * static_cast<double>(n);
        const double kk = static_cast<double>(k) * static_cast<double>(k);
        tk = (2.0 * t0 * nn) / (nn + kk);
    }
    return tk;
}

namespace {

void validateInputs(const VectorXd& x0,
                    const MatrixXd& M,
                    const VectorXd& q,
                    const Projector& Pr,
                    const DHan06Params& params) {
    validateLviInputs("dHan06", x0, M, q, Pr);
    if (!(params.gamma > 0.0 && params.gamma < 2.0)) {
        throw std::invalid_argument("dHan06: gamma must satisfy 0 < gamma < 2.");
    }
    if (!(params.mu > 0.0)) {
        throw std::invalid_argument("dHan06: mu must be positive.");
    }
    if (!(params.beta0 > 0.0)) {
        throw std::invalid_argument("dHan06: beta0 must be positive.");
    }
}

} // namespace

VIResult dHan06(const VectorXd& x0,
                    const MatrixXd& M,
                    const VectorXd& q,
                    const Projector& Pr,
                    double magTol,
                    int iterMax,
                    int iterFreq,
                    const DHan06Params& params,
                    const IterationLogger& logger) {
    validateInputs(x0, M, q, Pr, params);

    const Index nd = x0.size();
    const MatrixXd identity = MatrixXd::Identity(nd, nd);

    double bk = params.beta0;
    VectorXd x = x0;

    double mag = magTol + 1.0;
    int iter = 0;
    bool doneP = false;
    double initMag = -1.0;

    while (!doneP) {
        // Definition of 'e' (just before equation (3) of Han 2006).
        const VectorXd p = Pr(x - bk * (M * x + q));
        const VectorXd e = x - p;
        mag = e.squaredNorm();

        if (initMag < 0.0) {
            initMag = mag;
        }

        // Raise errors on NaN or out-of-control divergence, rather than
        // limping along with a corrupt iterate.
        if (std::isnan(mag)) {
            throw std::runtime_error("dHan06: residual magnitude is NaN.");
        }
        if (!(mag < params.divergenceFactor * initMag)) {
            throw std::runtime_error("dHan06: divergence detected; residual exceeded the guard.");
        }

        if (iterFreq > 0 && (iter % iterFreq) == 0 && logger) {
            logger(iter, iterMax, mag, magTol);
        }

        // Equations (10)-(11): the self-adaptive beta_k update -- the ONLY thing
        // dHan06 does that bsHe94b does not. Gated by adaptBeta so it can be turned
        // off (beta_k stays at beta0) to isolate its effect; the projection and the
        // (I + beta_k M) solve below are untouched and faithful to Han 2006.
        if (params.adaptBeta) {
            const double tk = tau(params.tau0, params.tauN, iter);
            // Equation (10): omega = |beta_k * M * e| / |e|.
            const VectorXd Me = M * e;
            const double omegaNum = (bk * Me).norm();
            const double omegaDnm = std::sqrt(mag);
            const double omega = omegaNum / omegaDnm;
            // Equation (11): raise beta_k when omega is small, lower it when large.
            if (omega < 1.0 / (1.0 + params.mu)) {
                bk = bk * (1.0 + tk);
            }
            if (omega > 1.0 + params.mu) {
                bk = bk / (1.0 + tk);
            }
        }

        // Equation (4): (I + beta_k M) y = e, then x <- x - gamma y.
        const VectorXd y = (identity + bk * M).partialPivLu().solve(e);
        if (!y.allFinite()) {
            throw std::runtime_error("dHan06: linear solve produced non-finite values.");
        }
        x = x - params.gamma * y;

        ++iter;
        doneP = (mag < magTol) || (iterMax < iter);
    }

    return VIResult{ x, mag, iter, mag < magTol };
}

} // namespace VINCP
// Copyright Ben Paul Wise. All Rights Reserved.

#include "dhan06.hpp"

#include <cmath>
#include <stdexcept>

namespace lvi {

double tau(double t0, int n, int k) {
    double tk = t0;
    if (n < k) {
        const double nn = static_cast<double>(n) * static_cast<double>(n);
        const double kk = static_cast<double>(k) * static_cast<double>(k);
        tk = (2.0 * t0 * nn) / (nn + kk);
    }
    return tk;
}

Eigen::VectorXd projectNonnegative(const Eigen::VectorXd& v) {
    return v.cwiseMax(0.0);
}

Projector makeMixedProjector(Eigen::Index numFree) {
    if (numFree < 0) {
        throw std::invalid_argument("makeMixedProjector: numFree must be non-negative.");
    }
    return [numFree](const Eigen::VectorXd& v) -> Eigen::VectorXd {
        if (numFree > v.size()) {
            throw std::invalid_argument("mixed projector: numFree exceeds vector length.");
        }
        Eigen::VectorXd out = v;
        const Eigen::Index tail = v.size() - numFree;
        out.tail(tail) = v.tail(tail).cwiseMax(0.0);
        return out;
    };
}

namespace {

void validateInputs(const Eigen::VectorXd& x0,
                    const Eigen::MatrixXd& M,
                    const Eigen::VectorXd& q,
                    const Projector& Pr,
                    const DHan06Params& params) {
    const Eigen::Index nd = x0.size();
    if (nd <= 0) {
        throw std::invalid_argument("dHan06: x0 must be non-empty.");
    }
    if (M.rows() != nd || M.cols() != nd) {
        throw std::invalid_argument("dHan06: M must be square and conformant with x0.");
    }
    if (q.size() != nd) {
        throw std::invalid_argument("dHan06: q must be conformant with x0.");
    }
    if (!Pr) {
        throw std::invalid_argument("dHan06: projector Pr must be set.");
    }
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

DHan06Result dHan06(const Eigen::VectorXd& x0,
                    const Eigen::MatrixXd& M,
                    const Eigen::VectorXd& q,
                    const Projector& Pr,
                    double magTol,
                    int iterMax,
                    int iterFreq,
                    const DHan06Params& params,
                    const IterationLogger& logger) {
    validateInputs(x0, M, q, Pr, params);

    const Eigen::Index nd = x0.size();
    const Eigen::MatrixXd identity = Eigen::MatrixXd::Identity(nd, nd);

    double bk = params.beta0;
    Eigen::VectorXd x = x0;

    double mag = magTol + 1.0;
    int iter = 0;
    bool doneP = false;
    double initMag = -1.0;

    while (!doneP) {
        const double tk = tau(params.tau0, params.tauN, iter);

        // Definition of 'e' (just before equation (3) of Han 2006).
        const Eigen::VectorXd p = Pr(x - bk * (M * x + q));
        const Eigen::VectorXd e = x - p;
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

        // Equation (10): omega = |beta_k * M * e| / |e|.
        const Eigen::VectorXd Me = M * e;
        const double omegaNum = (bk * Me).norm();
        const double omegaDnm = std::sqrt(mag);
        const double omega = omegaNum / omegaDnm;

        // Equation (11): self-adaptive update of beta_k.
        if (omega < 1.0 / (1.0 + params.mu)) {
            bk = bk * (1.0 + tk);
        }
        if (omega > 1.0 + params.mu) {
            bk = bk / (1.0 + tk);
        }

        // Equation (4): (I + beta_k M) y = e, then x <- x - gamma y.
        const Eigen::VectorXd y = (identity + bk * M).partialPivLu().solve(e);
        if (!y.allFinite()) {
            throw std::runtime_error("dHan06: linear solve produced non-finite values.");
        }
        x = x - params.gamma * y;

        ++iter;
        doneP = (mag < magTol) || (iterMax < iter);
    }

    return DHan06Result{ x, mag, iter };
}

} // namespace lvi

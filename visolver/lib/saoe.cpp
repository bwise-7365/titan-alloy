// Copyright Ben Paul Wise. All Rights Reserved.
#include "saoe.hpp"

#include "josephynewton.hpp"

#include <cmath>
#include <random>
#include <stdexcept>

namespace VINCP {

namespace {

// Column efforts E_j = sum_i e_{ij} + eps (length N).
Eigen::VectorXd columnEfforts(const Eigen::MatrixXd& e, double eps) {
    Eigen::VectorXd colEff = e.colwise().sum().transpose();
    colEff.array() += eps;
    return colEff;
}

} // namespace

double saoeEps(const Eigen::MatrixXd& R) {
    const double count = static_cast<double>(R.rows() * R.cols());
    return (R.norm() / std::sqrt(count)) / 10000.0;   // RMS(R) / 10000
}

Eigen::VectorXd saoeProbabilities(const Eigen::MatrixXd& e, double eps) {
    const Eigen::VectorXd colEff = columnEfforts(e, eps);
    return colEff / colEff.sum();
}

Eigen::VectorXd saoeUtilities(const Eigen::MatrixXd& R, const Eigen::MatrixXd& e,
                              double eps) {
    return R * saoeProbabilities(e, eps);
}

Eigen::VectorXd saoeRandomStart(const Eigen::MatrixXd& R, const Eigen::VectorXd& S,
                                std::mt19937_64& rng) {
    const Eigen::Index M = R.rows();
    const Eigen::Index N = R.cols();
    Eigen::VectorXd z0(N * M + M);
    for (Eigen::Index i = 0; i < M; ++i) {
        std::uniform_real_distribution<double> eDist(0.0, S(i) / static_cast<double>(N));
        for (Eigen::Index j = 0; j < N; ++j) {
            z0(i * N + j) = eDist(rng);
        }
    }
    std::uniform_real_distribution<double> lamDist(0.0, 1.0);
    for (Eigen::Index i = 0; i < M; ++i) {
        z0(N * M + i) = lamDist(rng);
    }
    return z0;
}

SaoeResult saoe(const Eigen::MatrixXd& R, const Eigen::VectorXd& S,
                const SaoeParams& params, const Eigen::VectorXd& startGuess) {
    if (R.rows() <= 0 || R.cols() <= 0) {
        throw std::invalid_argument("saoe: R must be non-empty.");
    }
    if (S.size() != R.rows()) {
        throw std::invalid_argument("saoe: S length must equal R.rows() (one strength per actor).");
    }

    const Eigen::Index M = R.rows();       // actors
    const Eigen::Index N = R.cols();       // options
    const Eigen::Index nEffort = N * M;    // effort variables
    const Eigen::Index dim = nEffort + M;  // + one multiplier per actor
    const double eps = saoeEps(R);

    // Complementarity map G(y), y = [ e (row-major), lambda ], all non-negative.
    const auto G = [R, S, eps, M, N, nEffort](const VectorXd& y) -> VectorXd {
        Eigen::MatrixXd e(M, N);
        for (Eigen::Index i = 0; i < M; ++i) {
            for (Eigen::Index j = 0; j < N; ++j) {
                e(i, j) = y(i * N + j);
            }
        }

        Eigen::VectorXd colEff = e.colwise().sum().transpose();
        colEff.array() += eps;
        const double D = colEff.sum();
        const VectorXd u = R * (colEff / D);   // u_i = sum_j R_ij P_j

        VectorXd g(nEffort + M);
        // Effort block: G_{ij} = lambda_i - dU_{ij}, dU_{ij} = (R_{ij} - u_i)/D.
        for (Eigen::Index i = 0; i < M; ++i) {
            const double lam = y(nEffort + i);
            for (Eigen::Index j = 0; j < N; ++j) {
                g(i * N + j) = lam - (R(i, j) - u(i)) / D;
            }
        }
        // Budget block: G_i = S_i - sum_j e_{ij}.
        for (Eigen::Index i = 0; i < M; ++i) {
            g(nEffort + i) = S(i) - e.row(i).sum();
        }
        return g;
    };

    const VIModel model = makeVIModel(0, dim, G);

    // Starting point: the caller's startGuess if supplied, else the default
    // e_{ij} = S_i/(N+1) (spends a little under budget), lambda = 0.
    VectorXd z0;
    if (startGuess.size() == dim) {
        z0 = startGuess;
    } else if (startGuess.size() == 0) {
        z0 = VectorXd::Zero(dim);
        for (Eigen::Index i = 0; i < M; ++i) {
            const double e0 = S(i) / static_cast<double>(N + 1);
            for (Eigen::Index j = 0; j < N; ++j) {
                z0(i * N + j) = e0;
            }
        }
    } else {
        throw std::invalid_argument("saoe: z0 length must be N*M + M, or empty for the default.");
    }

    const InnerSolver inner =
        (params.innerMethod == InnerMethod::He)
            ? makeBsHe94bSolver(params.innerMagTol, params.innerIterMax, 0)
            : makeDHan06Solver(params.innerMagTol, params.innerIterMax, 0);
    JosephyNewtonParams jn;
    jn.outerTol     = params.outerTol;
    jn.outerIterMax = params.outerIterMax;

    const VIResult res = solveVI(model, z0, inner, jn);

    // Unpack the solution vector back into e (M x N, row-major) and lambda (M).
    SaoeResult out;
    out.e = Eigen::MatrixXd(M, N);
    for (Eigen::Index i = 0; i < M; ++i) {
        for (Eigen::Index j = 0; j < N; ++j) {
            out.e(i, j) = res.z(i * N + j);
        }
    }
    out.lambda = res.z.tail(M);
    out.solve = res;
    return out;
}

} // namespace VINCP
// Copyright Ben Paul Wise. All Rights Reserved.

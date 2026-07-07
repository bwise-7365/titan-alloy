// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
// SAOE -- Strategic Allocation Of Effort: implementation (NCP build + solve).
// ----------------------------------------------
#include "saoe.hpp"

#include "josephynewton.hpp"

#include <cmath>
#include <random>
#include <stdexcept>

namespace VINCP {

  namespace {

    // Column efforts E_j = sum_i e_{ij} + eps (length N).
    VectorXd
    columnEfforts(const MatrixXd& e, double eps)
    {
      VectorXd colEff = e.colwise().sum().transpose();
      colEff.array() += eps;
      return colEff;
    }

  } // namespace

  double
  saoeEps(const MatrixXd& R)
  {
    const double count = static_cast<double>(R.rows() * R.cols());
    return (R.norm() / std::sqrt(count)) / 10000.0;   // RMS(R) / 10000
  }

  VectorXd
  saoeProbabilities(const MatrixXd& e, double eps)
  {
    const VectorXd colEff = columnEfforts(e, eps);
    return colEff / colEff.sum();
  }

  VectorXd
  saoeUtilities(const MatrixXd& R, const MatrixXd& e, double eps)
  {
    return R * saoeProbabilities(e, eps);
  }

  VectorXd
  saoeRandomStart(const MatrixXd& R, const VectorXd& S, std::mt19937_64& rng)
  {
    const Index M = R.rows();
    const Index N = R.cols();
    VectorXd z0(N * M + M);
    for (Index i = 0; i < M; ++i) {
      std::uniform_real_distribution<double> eDist(0.0, S(i) / static_cast<double>(N));
      for (Index j = 0; j < N; ++j) {
        z0(i * N + j) = eDist(rng);
      }
    }
    std::uniform_real_distribution<double> lamDist(0.0, 1.0);
    for (Index i = 0; i < M; ++i) {
      z0(N * M + i) = lamDist(rng);
    }
    return z0;
  }

  VectorXd
  saoePayoffVariance(const MatrixXd& R, const MatrixXd& e, double eps)
  {
    if (R.rows() != e.rows() || R.cols() != e.cols()) {
      throw std::invalid_argument("saoePayoffVariance: R and e must be the same shape.");
    }
    const VectorXd prob = saoeProbabilities(e, eps);
    const VectorXd mean = R * prob;
    const VectorXd meanSq = R.cwiseProduct(R) * prob;
    return meanSq - mean.cwiseProduct(mean);
  }

  VIModel
  saoeModel(const MatrixXd& R, const VectorXd& S, double riskAversion,
            double epsilon)
  {
    if (0 >= R.rows() || 0 >= R.cols()) {
      throw std::invalid_argument("saoeModel: R must be non-empty.");
    }
    if (S.size() != R.rows()) {
      throw std::invalid_argument(
          "saoeModel: S length must equal R.rows() (one strength per actor).");
    }

    const Index M = R.rows();       // actors
    const Index N = R.cols();       // options
    const Index nEffort = N * M;    // effort variables
    const Index dim = nEffort + M;  // + one multiplier per actor
    const double eps = (0.0 < epsilon) ? epsilon : saoeEps(R);

    // Per-actor risk coefficients, CONSTANT by construction (see the header):
    // alpha_i = a * ln2 / halfSpread_i, halfSpread_i = (max_j - min_j) R_ij / 2.
    VectorXd alpha = VectorXd::Zero(M);
    if (0.0 != riskAversion) {
      const double ln2 = std::log(2.0);
      for (Index i = 0; i < M; ++i) {
        const double halfSpread =
            0.5 * (R.row(i).maxCoeff() - R.row(i).minCoeff());
        // A constant reward row has nothing for risk to price (guarded;
        // impossible for generated instances, whose rows carry both signs).
        if (0.0 < halfSpread) {
          alpha(i) = riskAversion * ln2 / halfSpread;
        }
      }
    }

    // Complementarity map G(y), y = [ e (row-major), lambda ], all non-negative.
    const auto G = [R, S, eps, M, N, nEffort, riskAversion, alpha](const VectorXd& y) -> VectorXd {
      MatrixXd e(M, N);
      for (Index i = 0; i < M; ++i) {
        for (Index j = 0; j < N; ++j) {
          e(i, j) = y(i * N + j);
        }
      }

      VectorXd colEff = e.colwise().sum().transpose();
      colEff.array() += eps;
      const double D = colEff.sum();
      const VectorXd prob = colEff / D;

      // The reward each actor STEERS BY: R itself when risk-neutral, the
      // risk-adjusted S_ij = R_ij (1 - alpha_i (R_ij - mu_i)) otherwise (see
      // the header; alpha is the constant spread-calibrated coefficient, mu
      // the mean under the CURRENT probabilities). The a = 0 branch is
      // skipped outright so the risk-neutral model stays arithmetically
      // identical to the historical one.
      MatrixXd steer = R;
      if (0.0 != riskAversion) {
        const VectorXd mu = R * prob;
        for (Index i = 0; i < M; ++i) {
          for (Index j = 0; j < N; ++j) {
            steer(i, j) = R(i, j) * (1.0 - alpha(i) * (R(i, j) - mu(i)));
          }
        }
      }
      const VectorXd u = steer * prob;   // u_i = sum_j S_ij P_j

      VectorXd g(nEffort + M);
      // Effort block: G_{ij} = lambda_i - dU_{ij}, dU_{ij} = (S_{ij} - u_i)/D.
      for (Index i = 0; i < M; ++i) {
        const double lam = y(nEffort + i);
        for (Index j = 0; j < N; ++j) {
          g(i * N + j) = lam - (steer(i, j) - u(i)) / D;
        }
      }
      // Budget block: G_i = S_i - sum_j e_{ij}.
      for (Index i = 0; i < M; ++i) {
        g(nEffort + i) = S(i) - e.row(i).sum();
      }
      return g;
    };

    return makeVIModel(0, dim, G);
  }

  VectorXd
  saoeDefaultStart(const MatrixXd& R, const VectorXd& S)
  {
    if (0 >= R.rows() || 0 >= R.cols()) {
      throw std::invalid_argument("saoeDefaultStart: R must be non-empty.");
    }
    if (S.size() != R.rows()) {
      throw std::invalid_argument(
          "saoeDefaultStart: S length must equal R.rows() (one strength per actor).");
    }
    const Index M = R.rows();
    const Index N = R.cols();
    VectorXd z0 = VectorXd::Zero(N * M + M);
    for (Index i = 0; i < M; ++i) {
      const double e0 = S(i) / static_cast<double>(N + 1);
      for (Index j = 0; j < N; ++j) {
        z0(i * N + j) = e0;
      }
    }
    return z0;
  }

  VIResult
  saoe(const MatrixXd& R, const VectorXd& S,
       const SaoeParams& params, const VectorXd& startGuess)
  {
    const VIModel model = saoeModel(R, S);   // validates R, S
    const Index dim = model.m;

    // Starting point: the caller's startGuess if supplied, else the default.
    VectorXd z0;
    if (startGuess.size() == dim) {
      z0 = startGuess;
    }
    else if (0 == startGuess.size()) {
      z0 = saoeDefaultStart(R, S);
    }
    else {
      throw std::invalid_argument("saoe: z0 length must be N*M + M, or empty for the default.");
    }

    const InnerSolver inner =
        (params.innerMethod == InnerMethod::He)
            ? makeBsHe94bSolver(params.innerMagTol, params.innerIterMax, 0)
            : makeDHan06Solver(params.innerMagTol, params.innerIterMax, 0);
    JosephyNewtonParams jn;
    jn.outerTol              = params.outerTol;
    jn.outerIterMax          = params.outerIterMax;
    jn.logInnerDefiniteness  = params.logInnerDefiniteness;

    return solveVI(model, z0, inner, jn);
  }

  SaoeSolution
  saoeDecode(const VIResult& r, Index M, Index N)
  {
    // Mirror the packing used by the G map above: z = [e row-major (M x N), lambda (M)].
    SaoeSolution sol;
    sol.e = MatrixXd(M, N);
    for (Index i = 0; i < M; ++i) {
      for (Index j = 0; j < N; ++j) {
        sol.e(i, j) = r.z(i * N + j);
      }
    }
    sol.lambda = r.z.tail(M);
    return sol;
  }

} // namespace VINCP
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------

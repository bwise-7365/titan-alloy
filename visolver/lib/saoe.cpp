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

  VIResult
  saoe(const MatrixXd& R, const VectorXd& S,
       const SaoeParams& params, const VectorXd& startGuess)
  {
    if (0 >= R.rows() || 0 >= R.cols()) {
      throw std::invalid_argument("saoe: R must be non-empty.");
    }
    if (S.size() != R.rows()) {
      throw std::invalid_argument("saoe: S length must equal R.rows() (one strength per actor).");
    }

    const Index M = R.rows();       // actors
    const Index N = R.cols();       // options
    const Index nEffort = N * M;    // effort variables
    const Index dim = nEffort + M;  // + one multiplier per actor
    const double eps = saoeEps(R);

    // Complementarity map G(y), y = [ e (row-major), lambda ], all non-negative.
    const auto G = [R, S, eps, M, N, nEffort](const VectorXd& y) -> VectorXd {
      MatrixXd e(M, N);
      for (Index i = 0; i < M; ++i) {
        for (Index j = 0; j < N; ++j) {
          e(i, j) = y(i * N + j);
        }
      }

      VectorXd colEff = e.colwise().sum().transpose();
      colEff.array() += eps;
      const double D = colEff.sum();
      const VectorXd u = R * (colEff / D);   // u_i = sum_j R_ij P_j

      VectorXd g(nEffort + M);
      // Effort block: G_{ij} = lambda_i - dU_{ij}, dU_{ij} = (R_{ij} - u_i)/D.
      for (Index i = 0; i < M; ++i) {
        const double lam = y(nEffort + i);
        for (Index j = 0; j < N; ++j) {
          g(i * N + j) = lam - (R(i, j) - u(i)) / D;
        }
      }
      // Budget block: G_i = S_i - sum_j e_{ij}.
      for (Index i = 0; i < M; ++i) {
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
    }
    else if (0 == startGuess.size()) {
      z0 = VectorXd::Zero(dim);
      for (Index i = 0; i < M; ++i) {
        const double e0 = S(i) / static_cast<double>(N + 1);
        for (Index j = 0; j < N; ++j) {
          z0(i * N + j) = e0;
        }
      }
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

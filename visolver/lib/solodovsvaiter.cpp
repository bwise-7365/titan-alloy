// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
// Solodov-Svaiter 1999 double-projection method implementation.
// ----------------------------------------------
#include "solodovsvaiter.hpp"

#include <cmath>
#include <stdexcept>

namespace VIMCP {

  namespace {

    void
    validateParams(const SolodovSvaiterParams& params)
    {
      if (!(0.0 < params.mu)) {
        throw std::invalid_argument("solodovSvaiter: mu must be positive.");
      }
      if (!(0.0 < params.sigma && params.sigma < 1.0)) {
        throw std::invalid_argument(
            "solodovSvaiter: sigma must lie in (0, 1).");
      }
      if (!(0.0 < params.gamma && params.gamma < 1.0)) {
        throw std::invalid_argument(
            "solodovSvaiter: gamma must lie in (0, 1).");
      }
      if (1 > params.maxBacktracks) {
        throw std::invalid_argument(
            "solodovSvaiter: maxBacktracks must be >= 1.");
      }
      if (!(0.0 < params.divergenceFactor)) {
        throw std::invalid_argument(
            "solodovSvaiter: divergenceFactor must be positive.");
      }
      return;
    }

  } // namespace

  VIResult
  solodovSvaiter(const VectorXd& x0,
                 const MatrixXd& M,
                 const VectorXd& q,
                 const Projector& Pr,
                 double magTol,
                 int iterMax,
                 int iterFreq,
                 const SolodovSvaiterParams& params,
                 const IterationLogger& logger)
  {
    validateLviInputs("solodovSvaiter", x0, M, q, Pr);
    validateParams(params);
    if (!(0.0 < magTol) || 0 >= iterMax) {
      throw std::invalid_argument(
          "solodovSvaiter: magTol must be positive and iterMax > 0.");
    }

    const double sigmaOverMu = params.sigma / params.mu;
    VectorXd x = Pr(x0);                       // iterate feasibly from the start

    VIResult result;
    double initialMag = -1.0;
    for (int iter = 0; iter < iterMax; ++iter) {
      const VectorXd fieldAtX = M * x + q;
      const VectorXd projected = Pr(x - params.mu * fieldAtX);
      const VectorXd residual = x - projected;
      const double mag = residual.squaredNorm();

      if (std::isnan(mag)) {
        throw std::runtime_error("solodovSvaiter: residual became NaN.");
      }
      if (0 == iter) {
        initialMag = mag;
      }
      else
      if (mag > params.divergenceFactor * initialMag + 1.0) {
        throw std::runtime_error("solodovSvaiter: divergence detected.");
      }
      if (0 < iterFreq && logger && 0 == iter % iterFreq) {
        logger(iter, iterMax, mag, magTol);
      }

      result.z = x;
      result.residual = mag;
      result.iter = iter;
      if (mag <= magTol) {
        result.converged = true;
        return result;
      }

      // Armijo-style search back along the residual: y = x - t r, accepted
      // when <F(y), r> >= (sigma/mu) ||r||^2. Finite for continuous monotone
      // F (at t -> 0 the projection inequality gives <F(x), r> >= ||r||^2/mu).
      double t = 1.0;
      bool acceptedP = false;
      VectorXd y, fieldAtY;
      for (int m = 0; m < params.maxBacktracks; ++m) {
        y = x - t * residual;
        fieldAtY = M * y + q;
        if (!fieldAtY.allFinite()) {
          throw std::runtime_error(
              "solodovSvaiter: non-finite field at a trial point.");
        }
        if (fieldAtY.dot(residual) >= sigmaOverMu * mag) {
          acceptedP = true;
          break;
        }
        t *= params.gamma;
      }
      if (!acceptedP) {
        throw std::runtime_error(
            "solodovSvaiter: line search failed to terminate (is the "
            "problem pseudomonotone?).");
      }

      // Hyperplane step: project x onto {v : <F(y), v - y> = 0}, then onto K.
      const double denom = fieldAtY.squaredNorm();
      if (0.0 == denom) {
        x = y;                       // F(y) = 0: y itself solves the VI
        continue;
      }
      const double lambda = t * fieldAtY.dot(residual) / denom;
      x = Pr(x - lambda * fieldAtY);
    }

    // Cap reached: report the last iterate honestly (converged stays false).
    result.iter = iterMax;
    return result;
  }

} // namespace VIMCP
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------

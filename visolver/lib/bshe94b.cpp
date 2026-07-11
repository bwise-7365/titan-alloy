// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
// He's 1994 projection-contraction method (eq. 16): implementation (bsHe94b).
// ----------------------------------------------
#include "bshe94b.hpp"

#include <cmath>
#include <stdexcept>

namespace VIMCP {

  namespace {

    void
    validateInputs(const VectorXd& x0,
                   const MatrixXd& M,
                   const VectorXd& q,
                   const Projector& Pr,
                   const BsHe94bParams& params)
    {
      validateLviInputs("bsHe94b", x0, M, q, Pr);
      if (!(0.0 < params.gamma && params.gamma < 2.0)) {
        throw std::invalid_argument("bsHe94b: gamma must satisfy 0 < gamma < 2.");
      }
      return;
    }

  } // namespace

  VIResult
  bsHe94b(const VectorXd& x0,
          const MatrixXd& M,
          const VectorXd& q,
          const Projector& Pr,
          double magTol,
          int iterMax,
          int iterFreq,
          const BsHe94bParams& params,
          const IterationLogger& logger)
  {
    validateInputs(x0, M, q, Pr, params);

    const Index nd = x0.size();

    // Fixed metric: factor (M + I) once and reuse it every iteration. The Octave
    // formed the explicit inverse; storing the LU factorization is the numerically
    // sounder equivalent with the same precompute-once advantage.
    const MatrixXd identity = MatrixXd::Identity(nd, nd);
    const PartialPivLU<MatrixXd> luMI = (M + identity).partialPivLu();

    VectorXd x = x0;

    double mag = magTol + 1.0;
    int iter = 0;
    bool doneP = false;
    double initMag = -1.0;

    while (!doneP) {
      // Equation (4): the residual map e = x - Pr(x - (M x + q)).
      const VectorXd e = x - Pr(x - (M * x + q));
      mag = e.squaredNorm();

      if (0.0 > initMag) {
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

      if (0 < iterFreq && 0 == (iter % iterFreq) && logger) {
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

} // namespace VIMCP
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------

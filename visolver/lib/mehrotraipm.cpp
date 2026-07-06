// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
// Mehrotra predictor-corrector interior-point method: implementation.
// ----------------------------------------------
#include "mehrotraipm.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <stdexcept>

namespace VINCP {

  namespace {

    void
    validateInputs(const MatrixXd& M, const VectorXd& q, Index numFree,
                   double magTol, const MehrotraIpmParams& params)
    {
      if (0 == q.size()) {
        throw std::invalid_argument("mehrotraIpm: q must be non-empty.");
      }
      if (M.rows() != M.cols() || M.rows() != q.size()) {
        throw std::invalid_argument("mehrotraIpm: M must be square and conformant with q.");
      }
      if (0 > numFree || q.size() <= numFree) {
        throw std::invalid_argument(
            "mehrotraIpm: numFree must satisfy 0 <= numFree < dim (at least one "
            "complementarity component).");
      }
      if (!(0.0 < magTol)) {
        throw std::invalid_argument("mehrotraIpm: magTol must be positive.");
      }
      if (!(0.0 < params.tauFraction && params.tauFraction < 1.0)) {
        throw std::invalid_argument("mehrotraIpm: tauFraction must satisfy 0 < tau < 1.");
      }
      if (!(0.0 < params.sigmaMin && params.sigmaMin <= params.sigmaMax
            && params.sigmaMax <= 1.0)) {
        throw std::invalid_argument(
            "mehrotraIpm: centering clamp must satisfy 0 < sigmaMin <= sigmaMax <= 1.");
      }
      if (!(0.0 < params.stallStep)) {
        throw std::invalid_argument("mehrotraIpm: stallStep must be positive.");
      }
      if (!(0.0 < params.regEpsilon)) {
        throw std::invalid_argument("mehrotraIpm: regEpsilon must be positive.");
      }
      if (!(0.0 < params.divergenceFactor)) {
        throw std::invalid_argument("mehrotraIpm: divergenceFactor must be positive.");
      }
      return;
    }

    // Largest alpha >= 0 with y + alpha dy >= 0 and s + alpha ds >= 0
    // (+infinity when no component decreases).
    double
    maxStepToBoundary(const VectorXd& y, const VectorXd& dy,
                      const VectorXd& s, const VectorXd& ds)
    {
      double alpha = std::numeric_limits<double>::infinity();
      for (Index i = 0; i < y.size(); ++i) {
        if (0.0 > dy(i)) {
          alpha = std::min(alpha, -y(i) / dy(i));
        }
        if (0.0 > ds(i)) {
          alpha = std::min(alpha, -s(i) / ds(i));
        }
      }
      return alpha;
    }

  } // namespace

  // Each iteration solves two Newton systems that share one factorization.
  // With z = (x, y), rx = (M z + q)_x, and rs = (M z + q)_y - s, the Newton
  // equations for a complementarity target t are
  //     Mxx dx + Mxy dy       = -rx        (free rows carry no slack)
  //     Myx dx + Myy dy - ds  = -rs
  //     s.dy + y.ds           = t          (componentwise; t varies by phase)
  // Eliminating ds = (t - s.dy)./y from the middle row gives
  //     K [dx; dy] = [-rx; t./y - rs],   K = M + blockdiag(0, diag(s./y)).
  // The predictor uses t = -y.s (pure affine step toward complementarity);
  // the corrector uses t = sigma mu 1 - y.s - dyA.dsA, where the centering
  // weight sigma comes from Mehrotra's heuristic sigma = (muAff/mu)^3.
  VIResult
  mehrotraIpm(const MatrixXd& M,
              const VectorXd& q,
              Index numFree,
              double magTol,
              int iterMax,
              int iterFreq,
              const MehrotraIpmParams& params,
              const IterationLogger& logger)
  {
    validateInputs(M, q, numFree, magTol, params);

    const Index total = q.size();
    const Index n = numFree;
    const Index m = total - n;

    // Data-scaled strictly interior start (OOQP's simple rule): x = 0 and
    // y = s = beta 1.
    const double beta = std::max(1.0, std::sqrt(q.cwiseAbs().maxCoeff()));
    VectorXd x = VectorXd::Zero(n);
    VectorXd y = VectorXd::Constant(m, beta);
    VectorXd s = VectorXd::Constant(m, beta);
    VectorXd z(total);

    double mag = magTol + 1.0;
    double initMag = -1.0;
    int iter = 0;
    bool doneP = false;
    bool convergedP = false;
    bool regularizedP = false;   // sticky once the free block needs it

    while (!doneP) {
      z.head(n) = x;
      z.tail(m) = y;
      const VectorXd w = M * z + q;   // the exact field at z
      const VectorXd rx = w.head(n);  // free-block infeasibility
      const VectorXd wy = w.tail(m);
      const VectorXd rs = wy - s;     // infeasibility of s = (M z + q)_y

      // Termination and reporting use the shared natural-map convention
      // e = z - P_K(z - w) over K = R^n x R_+^m: the free block contributes
      // rx itself, the y block min(y, wy) componentwise (since y > 0).
      VectorXd e(total);
      e.head(n) = rx;
      e.tail(m) = y - (y - wy).cwiseMax(0.0);
      mag = e.squaredNorm();

      if (0.0 > initMag) {
        initMag = mag;
      }

      // Raise errors on NaN or out-of-control divergence, rather than
      // limping along with a corrupt iterate.
      if (std::isnan(mag)) {
        throw std::runtime_error("mehrotraIpm: residual magnitude is NaN.");
      }
      if (!(mag < 1.0 + params.divergenceFactor * initMag)) {
        throw std::runtime_error("mehrotraIpm: divergence detected; residual exceeded the guard.");
      }

      if (0 < iterFreq && 0 == (iter % iterFreq) && logger) {
        logger(iter, iterMax, mag, magTol);
      }

      convergedP = (mag < magTol);
      if (convergedP || iterMax <= iter) {
        doneP = true;
      }
      else {
        const double mu = y.dot(s) / static_cast<double>(m);
        const VectorXd sOverY = s.cwiseQuotient(y);

        // One factorization per iteration, shared by both solves below. For
        // PSD M the y block is positive definite (positive diagonal added);
        // conditioning grows like 1/mu near convergence, which is benign for
        // the computed step (Gondzio 2012, sec. 4) -- the honest response is
        // the magTol/iterMax termination, not a guard on it.
        MatrixXd K = M;
        K.diagonal().tail(m) += sOverY;
        if (regularizedP) {
          K.diagonal().head(n).array() += params.regEpsilon;
        }
        PartialPivLU<MatrixXd> luK(K);

        // Predictor (affine scaling): t = -y.s, so t./y - rs = -s - rs.
        VectorXd rhs(total);
        rhs.head(n) = -rx;
        rhs.tail(m) = -s - rs;
        VectorXd dA = luK.solve(rhs);
        if (!dA.allFinite()) {
          // A singular Newton matrix can only come from the free block (the
          // y block carries the positive diagonal): add the standard tiny
          // primal regularization there and refactor -- sticky for the rest
          // of the solve, since the deficiency is a property of M.
          if (regularizedP || 0 == n) {
            throw std::runtime_error("mehrotraIpm: predictor solve produced non-finite values.");
          }
          regularizedP = true;
          K.diagonal().head(n).array() += params.regEpsilon;
          luK.compute(K);
          dA = luK.solve(rhs);
          if (!dA.allFinite()) {
            throw std::runtime_error(
                "mehrotraIpm: predictor solve stayed non-finite after free-block regularization.");
          }
        }
        const VectorXd dyA = dA.tail(m);
        const VectorXd dsA = -s - sOverY.cwiseProduct(dyA);

        // Mehrotra centering: how much complementarity the pure affine step
        // could remove sets sigma for the corrector.
        const double alphaAff = std::min(1.0, maxStepToBoundary(y, dyA, s, dsA));
        const double muAff = (y + alphaAff * dyA).dot(s + alphaAff * dsA)
                             / static_cast<double>(m);
        const double sigma = std::clamp(std::pow(muAff / mu, 3.0),
                                        params.sigmaMin, params.sigmaMax);

        // Corrector: t = sigma mu 1 - y.s - dyA.dsA, same factorization and
        // the same free-block right-hand side.
        const VectorXd t = VectorXd::Constant(m, sigma * mu)
                           - y.cwiseProduct(s) - dyA.cwiseProduct(dsA);
        rhs.tail(m) = t.cwiseQuotient(y) - rs;
        const VectorXd d = luK.solve(rhs);
        if (!d.allFinite()) {
          throw std::runtime_error("mehrotraIpm: corrector solve produced non-finite values.");
        }
        const VectorXd dx = d.head(n);
        const VectorXd dy = d.tail(m);
        const VectorXd ds = t.cwiseQuotient(y) - sOverY.cwiseProduct(dy);

        // Damped step, common to all blocks (separate step lengths are an
        // LP-only device; for LCP they break the convergence theory).
        const double alpha = std::min(1.0, params.tauFraction
                                           * maxStepToBoundary(y, dy, s, ds));
        if (alpha < params.stallStep) {
          throw std::runtime_error("mehrotraIpm: step length collapsed; the iteration is stalled.");
        }
        x += alpha * dx;
        y += alpha * dy;
        s += alpha * ds;

        ++iter;
      }
    }

    return VIResult{ z, mag, iter, convergedP };
  }

} // namespace VINCP
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------

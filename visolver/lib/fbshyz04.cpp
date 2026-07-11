// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
// Forward-backward splitting (He-Yuan-Zhang 2004): implementation.
// ----------------------------------------------
#include "fbshyz04.hpp"

#include <algorithm>
#include <cmath>
#include <random>
#include <stdexcept>

namespace VIMCP {

  VIResult
  fbsHyz04(const VectorXd& x0,
           const VectorField& F,
           const Projector& Pr,
           double magTol,
           int iterMax,
           int iterFreq,
           const FbsHyz04Params& params,
           const IterationLogger& logger)
  {
    if (0 == x0.size()) {
      throw std::invalid_argument("fbsHyz04: x0 must be non-empty.");
    }
    if (!F) {
      throw std::invalid_argument("fbsHyz04: F must be set.");
    }
    if (!Pr) {
      throw std::invalid_argument("fbsHyz04: Pr must be set.");
    }
    if (!(0.0 < magTol)) {
      throw std::invalid_argument("fbsHyz04: magTol must be positive.");
    }
    if (0 >= iterMax) {
      throw std::invalid_argument("fbsHyz04: iterMax must be positive.");
    }
    if (0.0 > params.theta || 1.0 < params.theta) {
      throw std::invalid_argument("fbsHyz04: theta must lie in [0, 1].");
    }
    if (!(0.0 < params.lcFactor)) {
      throw std::invalid_argument("fbsHyz04: lcFactor must be positive.");
    }
    if (!(0.0 < params.divergenceFactor)) {
      throw std::invalid_argument("fbsHyz04: divergenceFactor must be positive.");
    }

    VectorXd x = Pr(x0);             // infeasible starts are fine
    VectorXd f = F(x);
    if (!f.allFinite()) {
      throw std::runtime_error("fbsHyz04: F(x0) is non-finite.");
    }

    // Initial step. Explicit gamma0 > 0 is honored; otherwise estimate the
    // Lipschitz constant by a few sampled secants at the start (pmedemo's
    // preflight: evaluate F at x0 and at perturbed points), deterministic
    // by fixed seed. Without this, gamma0 = 1 on a stiff field overshoots
    // past the divergence guard before the running estimate can react.
    double lc = 0.0;
    if (0.0 < params.gamma0) {
      lc = 1.0 / params.gamma0;
    }
    else {
      std::mt19937 rng(20260706u);
      std::uniform_real_distribution<double> unit(-1.0, 1.0);
      const double scale = 1.0e-3 * (1.0 + x.norm());
      const int probes = 8;
      for (int p = 0; p < probes; ++p) {
        VectorXd delta(x.size());
        for (Index i = 0; i < x.size(); ++i) {
          delta(i) = unit(rng);
        }
        delta *= scale / delta.norm();
        const VectorXd fp = F(x + delta);
        if (fp.allFinite()) {
          lc = std::max(lc, (fp - f).norm() / delta.norm());
        }
      }
      if (!(0.0 < lc)) {
        lc = 1.0;   // flat field near the start: any modest step is fine
      }
      // Random secants can miss the stiffest direction by a factor of ~2
      // on ill-conditioned fields; the auto estimate is therefore taken
      // CONSERVATIVELY (observed: the stiff constructed LCP tripped the
      // divergence guard without this). The running secant update then
      // relaxes gamma upward as the true local constant reveals itself.
      lc *= 2.0;
    }
    double gamma = 1.0 / (params.lcFactor * lc);

    double mag = magTol + 1.0;
    double initMag = -1.0;
    int iter = 0;
    bool doneP = false;
    bool convergedP = false;

    while (!doneP) {
      // Termination and reporting on the shared natural-map convention
      // (unit step, so the measure is gamma-independent and comparable
      // across engines).
      const VectorXd e = x - Pr(x - f);
      mag = e.squaredNorm();

      if (0.0 > initMag) {
        initMag = mag;
      }
      if (std::isnan(mag)) {
        throw std::runtime_error("fbsHyz04: residual magnitude is NaN.");
      }
      if (!(mag < 1.0 + params.divergenceFactor * initMag)) {
        throw std::runtime_error(
            "fbsHyz04: divergence detected; residual exceeded the guard.");
      }
      if (0 < iterFreq && 0 == (iter % iterFreq) && logger) {
        logger(iter, iterMax, mag, magTol);
      }

      convergedP = (mag < magTol);
      if (convergedP || iterMax <= iter) {
        doneP = true;
      }
      else {
        // Forward-backward step under a Tseng-style acceptance safeguard: a
        // step is taken only when gamma ||F(rho) - F(x)|| <= nu ||rho - x||
        // (nu = 0.9), else gamma is halved and the step retried. This is a
        // documented addition beyond the 2004 recipe (whose gentle fields
        // never needed it): it extends robustness to fields that are only
        // LOCALLY Lipschitz -- e.g. cubic maps over the unbounded orthant,
        // where no start-point estimate can be globally safe.
        const double kNu = 0.9;
        const int kBtMax = 60;
        VectorXd rho, fr;
        int bt = 0;
        while (true) {
          rho = Pr(x - gamma * f);
          fr = F(rho);
          const double dc0 = (rho - x).norm();
          if (fr.allFinite()
              && (0.0 == dc0 || gamma * (fr - f).norm() <= kNu * dc0)) {
            break;
          }
          gamma *= 0.5;
          if (kBtMax < ++bt) {
            throw std::runtime_error(
                "fbsHyz04: step backtracking failed to find a stable gamma.");
          }
        }
        if (0 < bt) {
          lc = 1.0 / (params.lcFactor * gamma);   // keep the estimate consistent
        }
        const VectorXd xNext = Pr(rho + gamma * (f - fr));
        const VectorXd fNext = F(xNext);
        if (!fNext.allFinite() || !xNext.allFinite()) {
          throw std::runtime_error("fbsHyz04: iterate or F value is non-finite.");
        }

        // Adaptive step: smoothed secant Lipschitz estimate (the pmedemo
        // rule). A zero displacement leaves the estimate untouched.
        const double dc = (xNext - x).norm();
        if (0.0 < dc) {
          const double l2 = (fNext - f).norm() / dc;
          lc = params.theta * l2 + (1.0 - params.theta) * lc;
          if (0.0 < lc) {
            gamma = 1.0 / (params.lcFactor * lc);
          }
        }

        x = xNext;
        f = fNext;
        ++iter;
      }
    }

    return VIResult{ x, mag, iter, convergedP };
  }

} // namespace VIMCP
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------

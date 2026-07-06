// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
// Semismooth Newton solver for the mixed NCP: implementation.
// ----------------------------------------------
#include "semismoothnewton.hpp"

#include "levenbergmarquardt.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <stdexcept>
#include <vector>

using std::vector;

namespace VINCP {

  namespace {

    constexpr double kGradientFloor = 1.0e-30;  // squared-gradient stall floor
    constexpr double kLmLambdaMin = 1.0e-12;    // clamps on the LM-tier damping
    constexpr double kLmLambdaMax = 1.0e+12;

    // Overflow-safe ||(a, b)|| (Munson et al. section 4.1): factoring out
    // s = |a| + |b| keeps the squares from overflowing for extreme inputs.
    double
    pairNorm(double a, double b)
    {
      const double s = std::abs(a) + std::abs(b);
      if (0.0 == s) {
        return 0.0;
      }
      const double as = a / s;
      const double bs = b / s;
      return s * std::sqrt(as * as + bs * bs);
    }

    // Cancellation-safe phi_FB(a, b) = a + b - ||(a, b)||: when a + b > 0 the
    // direct form subtracts nearly equal quantities near the positive axes;
    // (a+b)^2 - ||(a,b)||^2 = 2ab turns it into a stable quotient.
    double
    fbValue(double a, double b)
    {
      const double r = pairNorm(a, b);
      const double sum = a + b;
      if (sum <= 0.0) {
        return sum - r;
      }
      return 2.0 * a * b / (sum + r);
    }

    // Shared builder: lambda = 1 is plain Fischer-Burmeister (the penalty
    // coefficient 1 - lambda vanishes), lambda in (0, 1) the penalized form.
    NcpFunctionPair
    makeFbPair(double lambda)
    {
      NcpFunctionPair pair;

      pair.value = [lambda](const VectorXd& a, const VectorXd& b) -> VectorXd {
        if (a.size() != b.size()) {
          throw std::invalid_argument("NcpFunctionPair: a and b must have equal length.");
        }
        VectorXd v(a.size());
        for (Index i = 0; i < a.size(); ++i) {
          v(i) = lambda * fbValue(a(i), b(i))
                 + (1.0 - lambda) * std::max(0.0, a(i)) * std::max(0.0, b(i));
        }
        return v;
      };

      pair.jacobianDiagonals = [lambda](const VectorXd& a, const VectorXd& b,
                                        const VectorXd& gz) -> NcpJacobianDiagonals {
        if (a.size() != b.size() || a.size() != gz.size()) {
          throw std::invalid_argument(
              "NcpFunctionPair: a, b, and gz must have equal length.");
        }
        NcpJacobianDiagonals d;
        d.da.resize(a.size());
        d.db.resize(a.size());
        for (Index i = 0; i < a.size(); ++i) {
          const double r = pairNorm(a(i), b(i));
          if (0.0 == r) {
            // FB kink: the limiting direction (z_i, gz_i) = (1, gz_i) selects
            // a B-subdifferential element (Chen-Chen-Kanzow, Algorithm 4);
            // the penalty term contributes nothing at the origin.
            const double rk = pairNorm(1.0, gz(i));
            d.da(i) = lambda * (1.0 - 1.0 / rk);
            d.db(i) = lambda * (1.0 - gz(i) / rk);
          }
          else {
            d.da(i) = lambda * (1.0 - a(i) / r);
            d.db(i) = lambda * (1.0 - b(i) / r);
            if (0.0 < a(i) && 0.0 < b(i)) {
              // Penalty term active; on its boundary (a = 0 or b = 0) the
              // selection 0 from its subdifferential is valid and is what
              // falling through to the plain-FB branch produces.
              d.da(i) += (1.0 - lambda) * b(i);
              d.db(i) += (1.0 - lambda) * a(i);
            }
          }
        }
        return d;
      };

      return pair;
    }

    void
    validateInputs(const VIModel& model, const VectorXd& z0,
                   const SemismoothNewtonParams& params)
    {
      if (0 > model.n || 0 > model.m || 0 == model.n + model.m) {
        throw std::invalid_argument(
            "semismoothNewtonSolve: model dimensions must be non-negative and not both zero.");
      }
      if (z0.size() != model.n + model.m) {
        throw std::invalid_argument("semismoothNewtonSolve: z0 must have length n + m.");
      }
      if (!(0.0 < params.magTol)) {
        throw std::invalid_argument("semismoothNewtonSolve: magTol must be positive.");
      }
      if (0 >= params.iterMax) {
        throw std::invalid_argument("semismoothNewtonSolve: iterMax must be positive.");
      }
      if (!(0.0 < params.rho)) {
        throw std::invalid_argument("semismoothNewtonSolve: rho must be positive.");
      }
      if (!(2.0 < params.pExp)) {
        throw std::invalid_argument(
            "semismoothNewtonSolve: pExp must exceed 2 (the descent-test theory needs p > 2).");
      }
      if (1 > params.nonmonotoneMemory) {
        throw std::invalid_argument(
            "semismoothNewtonSolve: nonmonotoneMemory must be at least 1.");
      }
      if (!(0.0 < params.lmLambdaScale)) {
        throw std::invalid_argument("semismoothNewtonSolve: lmLambdaScale must be positive.");
      }
      if (!(0.0 < params.divergenceFactor)) {
        throw std::invalid_argument("semismoothNewtonSolve: divergenceFactor must be positive.");
      }
      if (!params.ncp.value || !params.ncp.jacobianDiagonals) {
        throw std::invalid_argument(
            "semismoothNewtonSolve: both members of the NCP function pair must be set.");
      }
      return;
    }

  } // namespace

  NcpFunctionPair
  fischerBurmeisterPair()
  {
    return makeFbPair(1.0);
  }

  NcpFunctionPair
  penalizedFischerBurmeisterPair(double lambda)
  {
    if (!(0.0 < lambda && lambda < 1.0)) {
      throw std::invalid_argument(
          "penalizedFischerBurmeisterPair: lambda must lie in (0, 1).");
    }
    return makeFbPair(lambda);
  }

  VIResult
  semismoothNewtonSolve(const VIModel& model,
                        const VectorXd& z0,
                        const SemismoothNewtonParams& params,
                        const IterationLogger& logger)
  {
    validateInputs(model, z0, params);

    const Index n = model.n;
    const Index m = model.m;
    const Index dTot = n + m;
    const Projector projectK = makeMixedProjector(n);
    const VectorField F = [&model](const VectorXd& z) { return evaluateF(model, z); };

    VectorXd z = z0;

    // Best-visited iterate in the natural-residual sense. The nonmonotone
    // line search may deliberately move ABOVE the best point seen and then
    // stall before recovering it (observed on the deploy_v07 game, 2026-07-06:
    // a run returned squared residual 1.7e4 after visiting 8.2e3), so the
    // result reports the best, not the last.
    VectorXd bestZ = z0;
    double bestMag = std::numeric_limits<double>::infinity();

    // Merit of the current point and its trailing window (nonmonotone baseline).
    vector<double> meritHistory;

    double mag = params.magTol + 1.0;
    double initMag = -1.0;
    int iter = 0;
    bool doneP = false;
    bool convergedP = false;

    while (!doneP) {
      const VectorXd Fz = F(z);   // throws on non-finite: accepted points stay honest
      const VectorXd a = z.tail(m);
      const VectorXd b = Fz.tail(m);

      VectorXd Phi(dTot);
      Phi.head(n) = Fz.head(n);
      Phi.tail(m) = params.ncp.value(a, b);
      const double psi = 0.5 * Phi.squaredNorm();

      meritHistory.push_back(psi);
      if (static_cast<int>(meritHistory.size()) > params.nonmonotoneMemory) {
        meritHistory.erase(meritHistory.begin());
      }

      // Termination and reporting use the shared natural-map convention over
      // the mixed projector; Psi steers only the globalization.
      const VectorXd e = z - projectK(z - Fz);
      mag = e.squaredNorm();

      if (0.0 > initMag) {
        initMag = mag;
      }

      // Raise errors on NaN or out-of-control divergence, rather than
      // limping along with a corrupt iterate.
      if (std::isnan(mag)) {
        throw std::runtime_error("semismoothNewtonSolve: residual magnitude is NaN.");
      }
      if (!(mag < 1.0 + params.divergenceFactor * initMag)) {
        throw std::runtime_error(
            "semismoothNewtonSolve: divergence detected; residual exceeded the guard.");
      }

      if (0 < params.iterFreq && 0 == (iter % params.iterFreq) && logger) {
        logger(iter, params.iterMax, mag, params.magTol);
      }

      if (mag < bestMag) {
        bestMag = mag;
        bestZ = z;
      }

      convergedP = (mag < params.magTol);
      if (convergedP || params.iterMax <= iter) {
        doneP = true;
      }
      else {
        // Full Jacobian of F, analytic when supplied, 4th-order FD otherwise.
        const MatrixXd J = params.jacobian
                               ? params.jacobian(z)
                               : centralDifferenceJacobian(F, z, params.fdStepRel);
        if (J.rows() != dTot || J.cols() != dTot) {
          throw std::runtime_error(
              "semismoothNewtonSolve: Jacobian must be (n + m) x (n + m).");
        }

        // Kink indicator and its image gz = J_G z, consumed only at kink rows.
        // The indicator lives in the FULL z-space (length n + m): kink
        // component i of the y block marks position n + i; the free block
        // stays zero.
        VectorXd kinkInd = VectorXd::Zero(dTot);
        bool anyKinkP = false;
        for (Index i = 0; i < m; ++i) {
          if (0.0 == pairNorm(a(i), b(i))) {
            kinkInd(n + i) = 1.0;
            anyKinkP = true;
          }
        }
        const VectorXd gz = anyKinkP ? VectorXd(J.bottomRows(m) * kinkInd)
                                     : VectorXd(VectorXd::Zero(m));

        const NcpJacobianDiagonals diag = params.ncp.jacobianDiagonals(a, b, gz);
        if (diag.da.size() != m || diag.db.size() != m) {
          throw std::runtime_error(
              "semismoothNewtonSolve: NCP diagonals must have length m.");
        }

        // Newton matrix: plain H rows for the free block, then
        // row n+i = da_i e_{n+i}^T + db_i (J_G)_i.
        MatrixXd Hk(dTot, dTot);
        Hk.topRows(n) = J.topRows(n);
        Hk.bottomRows(m) = diag.db.asDiagonal() * J.bottomRows(m);
        Hk.diagonal().tail(m) += diag.da;

        const VectorXd grad = Hk.transpose() * Phi;
        if (grad.squaredNorm() < kGradientFloor) {
          doneP = true;   // stationary merit at a non-solution: honest stop
        }
        else {
          // Direction ladder: Newton, then Levenberg-Marquardt (singular or
          // non-descent Newton matrices are COMMON on degenerate problems --
          // Munson et al. hit them in 12% of factorizations, and report the
          // gradient tier alone as unreliable), then the gradient last.
          VectorXd dir;
          bool haveDirP = false;

          const VectorXd dNewton = Hk.partialPivLu().solve(-Phi);
          if (dNewton.allFinite()
              && grad.dot(dNewton)
                     <= -params.rho * std::pow(dNewton.norm(), params.pExp)) {
            dir = dNewton;
            haveDirP = true;
          }
          if (!haveDirP) {
            const double lambda = std::clamp(params.lmLambdaScale * Phi.squaredNorm(),
                                             kLmLambdaMin, kLmLambdaMax);
            const MatrixXd damped = levenbergMarquardtDamp(Hk, lambda);
            const VectorXd dLm = damped.partialPivLu().solve(-grad);
            if (dLm.allFinite()
                && grad.dot(dLm) <= -params.rho * std::pow(dLm.norm(), params.pExp)) {
              dir = dLm;
              haveDirP = true;
            }
          }
          if (!haveDirP) {
            dir = -grad;
          }

          const double slope = grad.dot(dir);
          if (!(slope < 0.0)) {
            doneP = true;   // no descent available: honest stop
          }
          else {
            // Directional Armijo on Psi against the trailing-window baseline.
            // A trial point where the model throws (domain violation) counts
            // as +infinity, i.e. is backtracked away -- line search only.
            const double baseline =
                *std::max_element(meritHistory.begin(), meritHistory.end());
            const auto meritAt = [&](double alpha) -> double {
              const VectorXd trial = z + alpha * dir;
              try {
                const VectorXd Ft = evaluateF(model, trial);
                VectorXd PhiT(dTot);
                PhiT.head(n) = Ft.head(n);
                PhiT.tail(m) = params.ncp.value(trial.tail(m), Ft.tail(m));
                return 0.5 * PhiT.squaredNorm();
              }
              catch (const std::runtime_error&) {
                return std::numeric_limits<double>::infinity();
              }
            };

            const ArmijoResult step =
                armijoLineSearchDirectional(meritAt, baseline, slope, params.armijo);
            if (!step.accepted) {
              doneP = true;   // numerical floor reached: honest stop
            }
            else {
              z += step.alpha * dir;
              ++iter;
            }
          }
        }
      }
    }

    return VIResult{ bestZ, bestMag, iter, convergedP };
  }

} // namespace VINCP
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------

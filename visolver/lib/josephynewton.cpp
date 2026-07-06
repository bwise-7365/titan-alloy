// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
// Outer Josephy-Newton driver: implementation (linearize, inner solve, Armijo damp).
// ----------------------------------------------
#include "josephynewton.hpp"

#include "fdjacobian.hpp"

#include <cstdio>
#include <limits>
#include <stdexcept>

namespace VINCP {

  namespace {

    void
    validateModel(const VIModel& model, const VectorXd& z0)
    {
      if (0 > model.n || 0 > model.m) {
        throw std::invalid_argument("solveVI: model.n and model.m must be non-negative.");
      }
      if (0 >= model.n + model.m) {
        throw std::invalid_argument("solveVI: model dimension n + m must be positive.");
      }
      if (z0.size() != model.n + model.m) {
        throw std::invalid_argument("solveVI: z0 length must equal n + m.");
      }
      if (!model.H) {
        throw std::invalid_argument("solveVI: model.H must be set.");
      }
      if (!model.G) {
        throw std::invalid_argument("solveVI: model.G must be set.");
      }
      return;
    }

  } // namespace

  InnerSolver
  makeDHan06Solver(double magTol, int iterMax, int iterFreq,
                   const DHan06Params& params,
                   const IterationLogger& logger)
  {
    return [magTol, iterMax, iterFreq, params, logger](
               const VectorXd& x0, const MatrixXd& M,
               const VectorXd& q, const Projector& Pr) -> VIResult {
      return dHan06(x0, M, q, Pr, magTol, iterMax, iterFreq, params, logger);
    };
  }

  InnerSolver
  makeBsHe94bSolver(double magTol, int iterMax, int iterFreq,
                    const BsHe94bParams& params,
                    const IterationLogger& logger)
  {
    return [magTol, iterMax, iterFreq, params, logger](
               const VectorXd& x0, const MatrixXd& M,
               const VectorXd& q, const Projector& Pr) -> VIResult {
      return bsHe94b(x0, M, q, Pr, magTol, iterMax, iterFreq, params, logger);
    };
  }

  InnerSolver
  makeSolodovSvaiterSolver(double magTol, int iterMax, int iterFreq,
                           const SolodovSvaiterParams& params,
                           const IterationLogger& logger)
  {
    return [magTol, iterMax, iterFreq, params, logger](
               const VectorXd& x0, const MatrixXd& M,
               const VectorXd& q, const Projector& Pr) -> VIResult {
      return solodovSvaiter(x0, M, q, Pr, magTol, iterMax, iterFreq, params,
                            logger);
    };
  }

  InnerSolver
  makeChainedSolver(double magTol, int iterMax, int iterFreq,
                    const ChainedSolverParams& params,
                    const IterationLogger& logger)
  {
    return [magTol, iterMax, iterFreq, params, logger](
               const VectorXd& x0, const MatrixXd& M,
               const VectorXd& q, const Projector& Pr) -> VIResult {
      return chainedSolodovHe(x0, M, q, Pr, magTol, iterMax, iterFreq, params,
                              logger);
    };
  }

  InnerSolver
  makeMehrotraIpmSolver(Index numFree,
                        double magTol, int iterMax, int iterFreq,
                        const MehrotraIpmParams& params,
                        const IterationLogger& logger)
  {
    // The start and the projector are deliberately unnamed: the engine has
    // its own interior start, and K is fixed by numFree (see the header).
    return [numFree, magTol, iterMax, iterFreq, params, logger](
               const VectorXd&, const MatrixXd& M,
               const VectorXd& q, const Projector&) -> VIResult {
      return mehrotraIpm(M, q, numFree, magTol, iterMax, iterFreq, params,
                         logger);
    };
  }

  VIResult
  solveVI(const VIModel& model,
          const VectorXd& z0,
          const InnerSolver& innerSolver,
          const JosephyNewtonParams& params,
          const OuterLogger& logger,
          const Projector& projector)
  {
    validateModel(model, z0);
    if (!innerSolver) {
      throw std::invalid_argument("solveVI: innerSolver must be set.");
    }
    if (0 > params.stallIterMax) {
      throw std::invalid_argument("solveVI: stallIterMax must be non-negative.");
    }
    if (0.0 > params.stallRelDecrease || 1.0 <= params.stallRelDecrease) {
      throw std::invalid_argument("solveVI: stallRelDecrease must lie in [0, 1).");
    }

    // K enters only through its projector; default to the mixed free/non-negative
    // set matching the model's (x, y) split when the caller supplies none.
    const Projector Pr = projector ? projector : makeMixedProjector(model.n);

    // F as a single vector field for the finite-difference Jacobian.
    const VectorField F = [&model](const VectorXd& z) -> VectorXd {
      return evaluateF(model, z);
    };

    VectorXd z = z0;
    double residual = 0.0;
    int iter = 0;
    int innerIters = 0;
    bool converged = false;
    double bestResidual = std::numeric_limits<double>::infinity();
    int stallCount = 0;

    while (true) {
      const VectorXd Fz = evaluateF(model, z);

      // Natural-residual merit r(z) = z - Pi_K(z - F(z)).
      const VectorXd r = z - Pr(z - Fz);
      residual = r.squaredNorm();

      if (0 < params.outerIterFreq && 0 == (iter % params.outerIterFreq) && logger) {
        logger(iter, params.outerIterMax, residual, params.outerTol);
      }

      if (residual < params.outerTol) {
        converged = true;
        break;
      }
      if (iter >= params.outerIterMax) {
        break;
      }

      // No-progress cutoff (see JosephyNewtonParams::stallIterMax): break out
      // honestly, BEFORE spending another Jacobian + inner solve, once too many
      // consecutive iterations have failed to improve the best residual.
      if (0 < params.stallIterMax) {
        if (residual <= (1.0 - params.stallRelDecrease) * bestResidual) {
          stallCount = 0;
        }
        else {
          ++stallCount;
          if (stallCount >= params.stallIterMax) {
            break;
          }
        }
      }
      if (residual < bestResidual) {
        bestResidual = residual;
      }

      // Linearize: M = J(z), q = F(z) - J(z) z.
      const MatrixXd jac = centralDifferenceJacobian(F, z, params.fdStepRel);
      const VectorXd q = Fz - jac * z;

      // Diagnostic probe: monotonicity of the inner matrix M = jac. dHan06
      // (Han 2006 Thm 2.4) converges only when M is positive semidefinite, i.e.
      // the smallest eigenvalue of its symmetric part is >= 0. A negative value
      // here is the expected signature of the SAOE inner problem on which dHan06
      // diverges while bsHe94b still contracts.
      if (params.logInnerDefiniteness) {
        const MatrixXd symJac = 0.5 * (jac + jac.transpose());
        const SelfAdjointEigenSolver<MatrixXd> es(symJac, EigenvaluesOnly);
        if (es.info() == Success) {
          const double lmin = es.eigenvalues().minCoeff();
          std::printf("[probe] outer iter %3d: min eig(sym M) = %+.6e  ->  %s\n",
                      iter, lmin,
                      (0.0 <= lmin) ? "monotone (PSD)" : "NON-monotone (indefinite)");
        }
        else {
          std::printf("[probe] outer iter %3d: symmetric eigensolve failed\n", iter);
        }
      }

      // Inner affine-VI solve over the same K gives the Josephy-Newton point.
      const VIResult inner = innerSolver(z, jac, q, Pr);
      innerIters += inner.iter;

      // Damp the step with an Armijo line search on the natural-map merit
      // theta(w) = 1/2 ||w - Pi_K(w - F(w))||^2, so the undamped Newton step
      // cannot overshoot the non-smooth solution. 'residual' is ||r(z)||^2, so
      // theta at the base point is half of it.
      const VectorXd stepDir = inner.z - z;
      const double theta0 = 0.5 * residual;
      const auto meritAt = [&](double alpha) -> double {
        const VectorXd w = z + alpha * stepDir;
        const VectorXd Fw = evaluateF(model, w);
        const VectorXd rw = w - Pr(w - Fw);
        return 0.5 * rw.squaredNorm();
      };
      const ArmijoResult ls = armijoLineSearch(meritAt, theta0, params.armijo);
      z = z + ls.alpha * stepDir;
      ++iter;
    }

    return VIResult{ z, residual, iter, converged, innerIters };
  }

} // namespace VINCP
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------

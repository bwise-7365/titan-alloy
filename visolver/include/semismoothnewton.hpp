// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
// Semismooth Newton solver for the mixed nonlinear complementarity problem.
// ----------------------------------------------
#ifndef VIMCP_SEMISMOOTHNEWTON_HPP
#define VIMCP_SEMISMOOTHNEWTON_HPP

// Solves the mixed nonlinear complementarity problem (the library's core case)
//     H(x, y) = 0,   0 <= G(x, y)  _|_  y >= 0,   K = R^n x R_+^m,
// DIRECTLY: the structure of K is compiled into a nonsmooth equation instead
// of being projected onto. With an NCP function phi (phi(a, b) = 0 iff
// a >= 0, b >= 0, ab = 0), the system
//     Phi(z) = [ H(x, y) ; phi(y_i, G_i(x, y)) ] = 0,   z = (x, y),
// is solved by a generalized-Jacobian (semismooth) Newton method, globalized
// by a directional Armijo search on Psi(z) = 1/2 ||Phi(z)||^2 with a
// three-tier direction ladder (Newton -> Levenberg-Marquardt -> gradient).
// Contrast with smoothingnewton.hpp, which needs a mu-continuation precisely
// because plain Newton stalls at the exact-FB kink: the semismooth method
// works AT the kink via the generalized Jacobian, with a locally quadratic
// tail (Phi is strongly semismooth) under standard regularity, and the
// Levenberg-Marquardt tier retaining fast convergence under a local error
// bound even when solutions are degenerate/non-isolated. For monotone F,
// every stationary point of Psi is a solution.
//
// Sources (all open access; local copies in doc/):
//   T. De Luca, F. Facchinei, C. Kanzow, "A Theoretical and Numerical
//     Comparison of Some Semismooth Algorithms for Complementarity Problems",
//     Comput. Optim. Appl. 16 (2000) 173-205.  (The global scheme.)
//   T. S. Munson, F. Facchinei, M. C. Ferris, A. Fischer, C. Kanzow, "The
//     Semismooth Algorithm for Large Scale Complementarity Problems",
//     INFORMS J. Computing 13 (2001).  (Mixed treatment, stable evaluation,
//     singularity recovery, practical parameters.)
//   B. Chen, X. Chen, C. Kanzow, "A Penalized Fischer-Burmeister
//     NCP-Function", Math. Programming 88 (2000) 211-216.  (The default NCP
//     function and its generalized gradient.)
//   M. Ahookhosh, F. J. Aragon Artacho, R. M. T. Fleming, P. T. Vuong,
//     "Local convergence of the Levenberg-Marquardt method under Holder
//     metric subregularity", Math. Programming (2019); arXiv:1703.07461.
//     (LM convergence without nonsingularity.)

#include "vimcp.hpp"
#include "armijo.hpp"
#include "fdjacobian.hpp"

#include <Eigen/Dense>
#include <functional>

namespace VIMCP {

  // ---------------------------------------------------------------------------
  // NCP-function seam
  // ---------------------------------------------------------------------------

  // Diagonals (da, db) of one C-subdifferential element of the elementwise NCP
  // function at (a, b): row n+i of the semismooth Newton matrix is
  //     da_i e_{n+i}^T + db_i (J_G)_i ,
  // where (J_G)_i is row i of G's Jacobian block.
  struct NcpJacobianDiagonals {
    VectorXd da;
    VectorXd db;
  };

  // Elementwise NCP-function value over equal-length a, b.
  using NcpValueFn = function<VectorXd(const VectorXd& a, const VectorXd& b)>;

  // C-subdifferential diagonals at (a, b). 'gz' carries J_G z with z the
  // kink-set indicator (z_i = 1 exactly where a_i = b_i = 0); it is consumed
  // only at kink rows, where the limiting direction (z_i, gz_i) selects a
  // valid B-subdifferential element (Chen-Chen-Kanzow, Algorithm 4).
  using NcpJacobianFn = function<NcpJacobianDiagonals(
      const VectorXd& a, const VectorXd& b, const VectorXd& gz)>;

  // An NCP function paired with its generalized-Jacobian diagonals: the
  // swappable component of semismoothNewtonSolve (the same seam pattern as
  // SmoothingFunction in smoothingnewton.hpp).
  struct NcpFunctionPair {
    NcpValueFn    value;
    NcpJacobianFn jacobianDiagonals;
  };

  // Plain Fischer-Burmeister, phi(a, b) = a + b - sqrt(a^2 + b^2), with the
  // cancellation- and overflow-safe evaluation of Munson et al. section 4.1.
  // Kept as the classical/restart option; see the penalized variant below.
  NcpFunctionPair fischerBurmeisterPair();

  // Penalized Fischer-Burmeister (Chen-Chen-Kanzow 2000), the DEFAULT:
  //     phi_lam(a, b) = lam * phi_FB(a, b) + (1 - lam) * max(0, a) * max(0, b).
  // The penalty term repairs plain FB's two documented weaknesses (merit level
  // sets unbounded for merely monotone F; near-flatness deep in the positive
  // orthant); on the MCPLIB suite it solved every problem where plain FB had
  // outright failures. lambda must lie in (0, 1); practical guidance from the
  // SEMI production code: 0.8 default, 0.95 as a restart, and NEVER below 0.5
  // (the penalty term then dominates the merit and a merit-based stop can
  // accept sign-violating points -- our natural-residual stop is immune, but
  // small lambda still degrades the search direction). Throws
  // std::invalid_argument if lambda is outside (0, 1).
  NcpFunctionPair penalizedFischerBurmeisterPair(double lambda = 0.8);

  // ---------------------------------------------------------------------------
  // Jacobian seam
  // ---------------------------------------------------------------------------

  // Analytic Jacobian hook: given z, return F'(z) as an (n+m) x (n+m) matrix
  // (rows = (H rows; G rows), matching evaluateF's stacking). Leave empty to
  // use the 4th-order finite-difference Jacobian (fdjacobian.hpp) -- the
  // black-box default; bind an exact Jacobian when one is available (e.g. a
  // constant M for affine problems).
  using JacobianFn = function<MatrixXd(const VectorXd& z)>;

  // ---------------------------------------------------------------------------
  // Solver
  // ---------------------------------------------------------------------------

  struct SemismoothNewtonParams {
    double magTol = 1.0e-12;    // stop on the SQUARED natural residual (see below)
    int    iterMax = 200;       // Newton iteration cap
    int    iterFreq = 0;        // logging frequency (<= 0 disables logging)

    // Direction acceptance test (De Luca-Facchinei-Kanzow):
    //     grad(Psi) . d <= -rho * ||d||^pExp,   pExp > 2.
    double rho  = 1.0e-10;
    double pExp = 2.1;

    // Directional Armijo on Psi (constants from the sources: shrink 0.5,
    // sufficient decrease 1e-4).
    ArmijoParams armijo = ArmijoParams{ 1.0, 0.5, 1.0e-4, 40 };

    // Line-search baseline memory: 1 = monotone (the citable theory); the
    // SEMI production code uses 4 (nonmonotone) to escape merit plateaus.
    int nonmonotoneMemory = 1;

    // Levenberg-Marquardt tier damping: lambda = lmLambdaScale * ||Phi||^2
    // (clamped internally) -- the error-bound rule that keeps fast local
    // convergence on degenerate solution sets.
    double lmLambdaScale = 1.0;

    double divergenceFactor = 100.0;  // guard: mag must stay below 1 + factor * initialMag
    double fdStepRel = -1.0;          // FD Jacobian step (<= 0 => default); unused
                                      //   when 'jacobian' is set

    NcpFunctionPair ncp = penalizedFischerBurmeisterPair();  // swappable NCP function
    JacobianFn jacobian = JacobianFn{};                      // empty => FD Jacobian
  };

  // Solve the mixed NCP given as a VIModel (the same input type solveVI takes;
  // n = model.n free variables x, m = model.m complementarity variables y)
  // from the start z0 = (x0, y0). Any start is admissible -- the iterates are
  // NOT confined to K; H and G must therefore tolerate evaluation at arbitrary
  // points. A trial point where evaluateF throws (non-finite value) is treated
  // as merit +infinity DURING THE LINE SEARCH ONLY, i.e. the step is
  // backtracked away from domain violations (Munson et al.'s handling); at
  // accepted iterates a non-finite evaluation still throws.
  //
  // Termination and VIResult::residual use the library-standard SQUARED
  // natural-map residual ||z - P_K(z - F(z))||^2 over the mixed projector --
  // comparable across every engine -- while the line search steers by
  // Psi = 1/2 ||Phi||^2. 'converged' is honest: a vanishing merit gradient or
  // a failed line search returns converged = false rather than throwing.
  //
  // The returned z and residual are the BEST-VISITED iterate in the
  // natural-residual sense, not the last one: the nonmonotone line search may
  // deliberately climb above the best point seen and then stall before
  // recovering it (observed on the deploy_v07 game: a run ended at squared
  // residual 1.7e4 after visiting 8.2e3). 'iter' still counts all iterations
  // performed. On a converged run best and last coincide up to the final step.
  //
  // Throws std::invalid_argument on an inconsistent model/start or parameters
  // outside their ranges, and std::runtime_error on a NaN residual, detected
  // divergence, a Jacobian of the wrong shape, or NCP diagonals of the wrong
  // length. It never silently substitutes a result.
  VIResult semismoothNewtonSolve(const VIModel& model,
                                 const VectorXd& z0,
                                 const SemismoothNewtonParams& params =
                                     SemismoothNewtonParams{},
                                 const IterationLogger& logger = IterationLogger{});

} // namespace VIMCP

#endif // VIMCP_SEMISMOOTHNEWTON_HPP
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------

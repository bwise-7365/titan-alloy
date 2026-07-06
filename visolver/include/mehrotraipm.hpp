// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
// Mehrotra predictor-corrector interior-point method for the monotone LCP.
// ----------------------------------------------
#ifndef VINCP_MEHROTRAIPM_HPP
#define VINCP_MEHROTRAIPM_HPP

// Solves the monotone MIXED linear complementarity problem over
// K = R^n x R_+^m: with z = (x, y) and M, q partitioned conformally,
//     (M z + q)_x = 0,          s = (M z + q)_y,   0 <= y  _|_  s >= 0
// (numFree = 0 gives the pure LCP), by an infeasible primal-dual
// predictor-corrector interior-point method (Mehrotra-style path following):
// iterates keep y > 0, s > 0 strictly and follow the central path
// y_i s_i = mu down to mu -> 0, with a second-order corrector and adaptive
// centering per iteration. This is exactly the KKT-of-convex-QP problem
// class: x holds the primal/free variables, y the inequality multipliers.
//
// Contrast with the projection engines (dhan06.hpp, bshe94b.hpp,
// solodovsvaiter.hpp): the iteration count is ~10-40 essentially independent
// of dimension AND of degeneracy of the solution set -- the central path ends
// at the analytic center of the (possibly degenerate) optimal face, so the
// near-tied instances that drive a projection-contraction rate toward 1 cost
// an interior-point method only a few extra iterations. The price: one dense
// LU of (M + diag(s./y)) per iteration (reused for the predictor and the
// corrector solves), no warm start (the start is a scaled interior point,
// never a caller iterate), and the feasible set is structurally
// R^n x R_+^m rather than an arbitrary Projector.
//
// Convergence theory assumes M is positive semidefinite (monotone), the same
// standing assumption as dHan06/bsHe94b; this is not verified at runtime.
//
// Sources (all open access; implementation follows their recipes):
//   F. A. Potra, S. J. Wright, "Interior-point methods", J. Comput. Appl.
//     Math. 124 (2000) 281-302.
//   J. Gondzio, "Interior point methods 25 years later", Eur. J. Oper. Res.
//     218 (2012) 587-601.  (Practical guidance; why the growing conditioning
//     of the Newton matrix as mu -> 0 is benign.)
//   E. M. Gertz, S. J. Wright, "Object-oriented software for quadratic
//     programming", ACM TOMS 29 (2003) 58-81.  (OOQP: the reference
//     implementation pattern for this algorithm class.)
//   F. A. Potra, X. Liu, "Corrector-predictor methods for sufficient linear
//     complementarity problems", Comput. Optim. Appl. 48 (2009).
//     (Superlinear convergence on degenerate problems.)

#include "vincp.hpp"

namespace VINCP {

  // Tunable constants of the predictor-corrector method. Defaults are the
  // standard values from the sources above.
  struct MehrotraIpmParams {
    double tauFraction = 0.995;       // fraction-to-boundary damping, 0 < tau < 1
    double sigmaMin = 1.0e-6;         // centering clamp: 0 < sigmaMin <= sigma
    double sigmaMax = 1.0;            //   <= sigmaMax <= 1
    double stallStep = 1.0e-10;       // throw when the damped step length falls below this
    double regEpsilon = 1.0e-8;       // free-block diagonal regularization, applied only
                                      //   (and then stickily) if the Newton solve is singular
    double divergenceFactor = 100.0;  // guard: mag must stay below 1 + factor * initialMag
  };

  // Solve the mixed LCP by the Mehrotra predictor-corrector interior-point method.
  //
  //   M        square matrix (positive semidefinite for the convergence theory)
  //   q        vector conformant with M
  //   numFree  dimension n of the leading free block x of z = (x, y); must
  //            satisfy 0 <= numFree < dim (at least one complementarity
  //            component). A rank-deficient free block (e.g. a flat objective
  //            direction no constraint touches) makes the Newton matrix
  //            singular; the solver then adds regEpsilon to the free diagonal
  //            and refactors, once, stickily, rather than failing.
  //   magTol   termination tolerance on the SQUARED natural-map residual
  //            ||y - P_+(y - (M y + q))||^2, the same convention every other
  //            engine uses (internally the method steers by the plain
  //            complementarity measure mu = y.s / m, but it terminates and
  //            reports on the shared squared-residual convention)
  //   iterMax  iteration cap (each iteration is one LU factorization)
  //   iterFreq logging frequency (<= 0 disables logging)
  //   params   tunable constants
  //   logger   optional logging hook
  //
  // There is deliberately no start vector: an interior-point method cannot use
  // a caller iterate (it needs y, s strictly positive and near the central
  // path) and starts from a data-scaled interior point instead.
  //
  // Throws std::invalid_argument on inconsistent dimensions or invalid
  // parameters, and std::runtime_error on a NaN residual, detected
  // divergence, a non-finite Newton solve, or a collapsed step length.
  // It never silently substitutes a default result.
  VIResult mehrotraIpm(const MatrixXd& M,
                       const VectorXd& q,
                       Index numFree,
                       double magTol,
                       int iterMax,
                       int iterFreq,
                       const MehrotraIpmParams& params = MehrotraIpmParams{},
                       const IterationLogger& logger = IterationLogger{});

} // namespace VINCP

#endif // VINCP_MEHROTRAIPM_HPP
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------

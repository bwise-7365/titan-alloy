// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
// Solodov-Svaiter 1999 double-projection (hyperplane) method for the LVI.
// ----------------------------------------------
#ifndef VINCP_SOLODOVSVAITER_HPP
#define VINCP_SOLODOVSVAITER_HPP

// Solves the linear variational inequality (LVI)
//     find x in K such that (M x + q) . (w - x) >= 0  for all w in K
// by the projection method of:
//     M. V. Solodov and B. F. Svaiter, "A new projection method for
//     variational inequality problems", SIAM J. Control Optim. 37(3), 1999.
//
// Each iteration: one projection gives the residual r = x - P_K(x - mu F(x));
// an Armijo-style search along r finds y with <F(y), r> >= (sigma/mu)||r||^2;
// the solution set then lies in the halfspace behind the hyperplane through y
// with normal F(y), and the next iterate is the projection of x moved onto
// that hyperplane, re-projected onto K.
//
// Contrasts with the two existing inner solvers (dhan06.hpp, bshe94b.hpp):
//   - MATRIX-FREE: M enters only through products; nothing is factored
//     (dHan06 factors (I + beta_k M) every iteration, bsHe94b factors (M + I)
//     once). Constant memory beyond the iterate.
//   - Converges globally for merely PSEUDOMONOTONE continuous F (weaker than
//     the monotonicity Han's method needs), with no Lipschitz constant or
//     line-search-free metric required.
// It is the globally-safe fallback engine of the planned hybrid (task E3).
//
// The interface matches dHan06/bsHe94b exactly (same argument order and the
// shared VectorXd / MatrixXd / Projector / IterationLogger / VIResult types);
// only the tunable-parameter struct differs. The start x0 is projected onto K
// before iterating, so an infeasible start is fine.

#include "vincp.hpp"

namespace VINCP {

  // Tunable constants of the Solodov-Svaiter method.
  struct SolodovSvaiterParams {
    double mu = 1.0;                  // residual step: r = x - P_K(x - mu F(x)), mu > 0
    double sigma = 0.5;               // line-search acceptance, 0 < sigma < 1
    double gamma = 0.5;               // line-search backtracking factor, 0 < gamma < 1
    int    maxBacktracks = 60;        // cap on the (provably finite) search
    double divergenceFactor = 100.0;  // guard: mag must stay below factor * initialMag
  };

  // Solve the LVI by the Solodov-Svaiter double-projection method.
  //
  //   x0       starting point (projected onto K first; fixes the dimension n)
  //   M        n-by-n matrix
  //   q        n-vector
  //   Pr       projector onto K
  //   magTol   termination tolerance on the SQUARED residual norm ||r||^2
  //   iterMax  iteration cap
  //   iterFreq logging frequency (<= 0 disables logging)
  //   params   tunable constants
  //   logger   optional logging hook
  //
  // Throws std::invalid_argument on inconsistent dimensions or invalid
  // parameters, and std::runtime_error on a NaN residual, detected
  // divergence, a non-finite trial point, or a line search that fails to
  // terminate (impossible for continuous monotone F; the cap is a guard).
  // It never silently substitutes a default result.
  VIResult solodovSvaiter(const VectorXd& x0,
                          const MatrixXd& M,
                          const VectorXd& q,
                          const Projector& Pr,
                          double magTol,
                          int iterMax,
                          int iterFreq,
                          const SolodovSvaiterParams& params = SolodovSvaiterParams{},
                          const IterationLogger& logger = IterationLogger{});

} // namespace VINCP

#endif // VINCP_SOLODOVSVAITER_HPP
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------

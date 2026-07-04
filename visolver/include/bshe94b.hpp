// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
// He's 1994 projection-contraction method (eq. 16) for the linear VI (bsHe94b).
// ----------------------------------------------
#ifndef VINCP_BSHE94B_HPP
#define VINCP_BSHE94B_HPP

// Solves the linear variational inequality (LVI)
//     find x in K such that (M x + q) . (w - x) >= 0  for all w in K
// by the projection-contraction method (equation 16) of:
//     Bingsheng He, "A new method for a class of linear variational
//     inequalities", 1994.
//
// Unlike Han's self-adaptive method (dhan06.hpp), this uses a FIXED metric: it
// factors (M + I) once and reuses it every iteration, so each step is a single
// solve against that decomposition. He's equation (16) search direction (this
// routine) needs far fewer iterations than his equation (6) (bsHe94a).
//
// The interface matches dHan06 exactly (same argument order and the shared
// VectorXd / MatrixXd / Projector / IterationLogger / VIResult types), so the
// two inner solvers are interchangeable inside the same outer loop. Only the
// tunable-parameter struct differs.

#include <Eigen/Dense>
#include "vincp.hpp"

namespace VINCP {

  // Tunable constants of He's method. Defaults match the Octave source.
  struct BsHe94bParams {
    double gamma = 1.6;               // relaxation factor, 0 < gamma < 2 (>1 recommended)
    double divergenceFactor = 100.0;  // guard: mag must stay below 1 + factor * initialMag
  };

  // Solve the LVI by He's 1994 projection-contraction method (equation 16).
  //
  //   x0       starting point (also fixes the problem dimension n)
  //   M        n-by-n matrix
  //   q        n-vector
  //   Pr       projector onto K
  //   magTol   termination tolerance on the squared residual norm
  //   iterMax  iteration cap
  //   iterFreq logging frequency (<= 0 disables logging)
  //   params   tunable constants
  //   logger   optional logging hook
  //
  // Throws std::invalid_argument on inconsistent dimensions or invalid parameters,
  // and std::runtime_error on a NaN residual, detected divergence, or a non-finite
  // solve against (M + I). It never silently substitutes a default result.
  VIResult bsHe94b(const VectorXd& x0,
                   const MatrixXd& M,
                   const VectorXd& q,
                   const Projector& Pr,
                   double magTol,
                   int iterMax,
                   int iterFreq,
                   const BsHe94bParams& params = BsHe94bParams{},
                   const IterationLogger& logger = IterationLogger{});

} // namespace VINCP

#endif // VINCP_BSHE94B_HPP
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------

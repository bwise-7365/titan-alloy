// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
// Han's 2006 self-adaptive projection method for the linear VI (port of dHan06).
// ----------------------------------------------
#ifndef VIMCP_DHAN06_HPP
#define VIMCP_DHAN06_HPP

// Solves the linear variational inequality (LVI)
//     find x in K such that (M x + q) . (w - x) >= 0  for all w in K
// by the self-adaptive projection method of:
//     Deren Han, "Solving linear variational inequality problems by a
//     self-adaptive projection method", 2006.
//
// The set K enters only through the projector Pr.  The translation preserves
// the numerics of the reference implementation; the algorithmic constants are
// exposed as a parameter struct whose defaults reproduce the Octave source.

#include <Eigen/Dense>
#include <functional>
#include "vimcp.hpp"

namespace VIMCP {

  // IterationLogger is the shared inner-solver logging hook, declared in vimcp.hpp.

  // Tunable constants of Han's method. Defaults match the Octave source.
  struct DHan06Params {
    double gamma = 1.6;               // relaxation factor, must satisfy 0 < gamma < 2
    double mu = 1.05;                 // Han's constant (mu > 0)
    double beta0 = 1.0;               // initial beta, must be > 0. 1.0 makes the first
                                      // step's metric (I + beta0 M) = (M + I), i.e. bsHe94b's
                                      // proven-contractive metric, so Han starts stably even on
                                      // an indefinite M and the self-adaptive rule tunes upward
                                      // from there. Han 2006 permits any beta0 > 0 (the Octave
                                      // used 0.5, which only converges on a monotone M).
    double tau0 = 0.5;                // initial tau
    int    tauN = 10;                 // 'n' in the tau schedule
    double divergenceFactor = 100.0;  // guard: mag must stay below factor * initialMag

    // DIAGNOSTIC: when false, freeze beta_k at beta0 and skip the eq.(11) update.
    // With beta0 = 1 this makes dHan06 numerically identical to bsHe94b (which IS
    // this method with beta frozen at 1), so the self-adaptive beta -- the only
    // thing dHan06 does that bsHe94b does not -- can be A/B-compared in isolation.
    // Leave true for the faithful Han 2006 method.
    bool   adaptBeta = true;
  };

  // One element of the tau(k) schedule. Mirrors the Octave sub-function: returns
  // t0 for k <= n, and 2*t0*n^2 / (n^2 + k^2) for k > n.
  double tau(double t0, int n, int k);

  // Solve the LVI by Han's self-adaptive projection method.
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
  // Throws std::invalid_argument on inconsistent dimensions or invalid
  // parameters, and std::runtime_error on a NaN residual, detected divergence,
  // or a non-finite linear solve. It never silently substitutes a default result.
  VIResult dHan06(const VectorXd& x0,
                  const MatrixXd& M,
                  const VectorXd& q,
                  const Projector& Pr,
                  double magTol,
                  int iterMax,
                  int iterFreq,
                  const DHan06Params& params = DHan06Params{},
                  const IterationLogger& logger = IterationLogger{});

} // namespace VIMCP

#endif // VIMCP_DHAN06_HPP
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------

// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
// Forward-backward splitting (He-Yuan-Zhang 2004) for the general monotone VI.
// ----------------------------------------------
#ifndef VINCP_FBSHYZ04_HPP
#define VINCP_FBSHYZ04_HPP

// Solves the GENERAL variational inequality
//     find x in K such that F(x) . (w - x) >= 0  for all w in K
// by the two-projection forward-backward splitting scheme of:
//     B. He, X. Yuan, J. J. Z. Zhang, "Comparison of Two Kinds of
//     Prediction-Correction Methods for Monotone Variational Inequalities",
//     Computational Optimization and Applications 27, 247-267, 2004,
// as used by the author's earlier pmedemo implementation (whose adaptive
// step rule this port preserves). Each iteration:
//     rho = P_K(x - gamma F(x)),                 (forward-backward step)
//     x+  = P_K(rho + gamma (F(x) - F(rho))),    (correction)
// with gamma adapted online from a smoothed secant estimate of the
// Lipschitz constant: L2 = ||F(x+) - F(x)|| / ||x+ - x||,
// Lc <- theta L2 + (1 - theta) Lc, gamma = 1 / (lcFactor * Lc).
// Two safeguards beyond the 2004 recipe (both documented departures): an
// auto preflight for gamma0 (sampled secants at the start, taken
// conservatively) and Tseng-style step backtracking (halve gamma until
// gamma ||F(rho) - F(x)|| <= 0.9 ||rho - x||), which extends robustness to
// fields that are only LOCALLY Lipschitz (e.g. cubic maps), where no global
// constant exists.
//
// Contrast with the other engines at this level: like solodovSvaiter it is
// MATRIX-FREE and takes any projector-defined K, but it takes F ITSELF (a
// VectorField), not an affine (M, q) -- it solves the NONLINEAR VI directly,
// with no Josephy-Newton wrapper and no Jacobian of any kind. Convergence
// theory needs monotone, Lipschitz F. An affine adapter for the
// Josephy-Newton InnerSolver seam is makeFbsHyz04Solver (josephynewton.hpp).

#include "vincp.hpp"
#include "fdjacobian.hpp"   // VectorField

namespace VINCP {

  // Tunable constants; defaults are the pmedemo values.
  struct FbsHyz04Params {
    double gamma0 = -1.0;              // initial step. <= 0 (the default) =
                                       //   AUTO: estimate the Lipschitz
                                       //   constant by sampled secants at the
                                       //   start (pmedemo's own preflight) and
                                       //   set gamma0 = 1/(lcFactor * L).
                                       //   Explicit > 0 is honored as given.
    double theta = 0.5;                // Lipschitz-estimate smoothing, in [0, 1]
    double lcFactor = 1.618034;        // step safety: gamma = 1 / (lcFactor * Lc), > 0
    double divergenceFactor = 100.0;   // guard: mag must stay below 1 + factor * initialMag
  };

  // Solve the VI by adaptive forward-backward splitting. x0 is projected
  // onto K first (an infeasible start is fine). Termination and
  // VIResult::residual use the library-standard SQUARED natural residual
  // ||x - P_K(x - F(x))||^2. Costs two F evaluations and two projections
  // per iteration; nothing is factored.
  //
  // Throws std::invalid_argument on an empty x0, unset F or Pr, or
  // parameters outside their ranges, and std::runtime_error on a NaN
  // residual, detected divergence, or a non-finite F value. It never
  // silently substitutes a result.
  VIResult fbsHyz04(const VectorXd& x0,
                    const VectorField& F,
                    const Projector& Pr,
                    double magTol,
                    int iterMax,
                    int iterFreq,
                    const FbsHyz04Params& params = FbsHyz04Params{},
                    const IterationLogger& logger = IterationLogger{});

} // namespace VINCP

#endif // VINCP_FBSHYZ04_HPP
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------

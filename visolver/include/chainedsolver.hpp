// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
// Two-phase chained LVI solver: Solodov-Svaiter (rough, global) warm-starting
// bsHe94b (tight finish).
// ----------------------------------------------
#ifndef VIMCP_CHAINEDSOLVER_HPP
#define VIMCP_CHAINEDSOLVER_HPP

// Solves the linear variational inequality by playing each method only in
// its strong regime (the E2b calibration, network/plan.md 2026-07-05):
//
//   Phase 1  solodovSvaiter to a LOOSE tolerance. Matrix-free, globally
//            convergent under mere pseudomonotonicity, safe from any start --
//            but its hyperplane step collapses like ||r||^2/||F||^2, so its
//            tail is O(1/sqrt(k)): a globalizer, not a finisher.
//   Phase 2  bsHe94b warm-started from phase 1's iterate, run to the caller's
//            tight tolerance. Factors (M + I) once; linear contraction
//            finishes what the globalizer started.
//
// Phase 1 hitting its iteration cap is NOT an error: whatever iterate it
// reached is handed to phase 2 (an honest warm start is the whole point).
// Divergence/NaN in either phase still throws, per the library stance.

#include "bshe94b.hpp"
#include "solodovsvaiter.hpp"

namespace VIMCP {

  // Tunables. The rough (phase-1) budget is deliberately small and its
  // tolerance loose; the tight tolerance and cap are the caller's magTol /
  // iterMax arguments, which govern phase 2.
  struct ChainedSolverParams {
    double roughMagTol = 1.0e-4;      // phase-1 SQUARED-residual target
    int roughIterMax = 20000;         // phase-1 cap (cap-and-hand-off is fine)
    SolodovSvaiterParams rough;       // phase-1 tunables
    BsHe94bParams finish;             // phase-2 tunables
  };

  // Solve the LVI by the chain. Returns phase 2's result with composite
  // accounting: iter is phase 2's iteration count and innerIters is the total
  // across both phases (the VIResult composite convention). converged and
  // residual are phase 2's, against the caller's magTol.
  //
  // Throws std::invalid_argument on bad params or inputs (validated by the
  // component solvers) and propagates their std::runtime_error on NaN or
  // divergence. Never silently substitutes a result.
  VIResult chainedSolodovHe(const VectorXd& x0,
                            const MatrixXd& M,
                            const VectorXd& q,
                            const Projector& Pr,
                            double magTol,
                            int iterMax,
                            int iterFreq,
                            const ChainedSolverParams& params = ChainedSolverParams{},
                            const IterationLogger& logger = IterationLogger{});

} // namespace VIMCP

#endif // VIMCP_CHAINEDSOLVER_HPP
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------

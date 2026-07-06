// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
// chooseEngine: problem-feature inspection and automatic engine dispatch.
// ----------------------------------------------
#ifndef VINCP_CHOOSEENGINE_HPP
#define VINCP_CHOOSEENGINE_HPP

// Mechanizes the engine-selection decision table (report Part II) that the
// 2026 engine program validated empirically: projection-contraction for
// warm-started or non-orthant monotone affine problems, the interior-point
// engine for monotone affine problems without a warm start (its iteration
// count is insensitive to dimension and degeneracy), the Solodov-Svaiter
// chain when monotonicity fails or is in doubt, the semismooth Newton solver
// for the nonlinear mixed NCP, and the alternating chain as the nonmonotone
// fallback when the semismooth solver stalls honestly.
//
// Three layers, separately usable:
//   - probeMonotone: a cheap decision procedure for "is sym(M) PSD (within
//     tolerance)" -- one Cholesky attempt, not an eigensolve.
//   - chooseEngine: the PURE decision function ProblemTraits -> EngineChoice.
//     Deterministic and side-effect free, so the policy itself is unit-tested.
//   - solveAffineAuto / solveModelAuto: executors that assemble the traits,
//     call chooseEngine, run the chosen engine, and (for the model form)
//     fall back per the evidence. The choice is observable through the
//     ChoiceLogger hook, never silent.

#include "vincp.hpp"
#include "bshe94b.hpp"
#include "chainedsolver.hpp"
#include "mehrotraipm.hpp"
#include "semismoothnewton.hpp"
#include "alternatingchain.hpp"

#include <functional>

namespace VINCP {

  // The engines the dispatcher can select. JosephyNewton appears for
  // completeness of the pure decision function (nonlinear over a non-orthant
  // K); the executors below cover the orthant/mixed cases only and say so.
  enum class EngineChoice
  {
    BsHe94b,            // fixed-metric projection-contraction
    ChainedSolodovHe,   // Solodov-Svaiter globalizer -> bsHe94b finisher
    MehrotraIpm,        // predictor-corrector interior point
    SemismoothNewton,   // direct mixed-NCP Newton
    AlternatingChain,   // project -> globalize -> finish rounds
    JosephyNewton       // outer linearization over a general projector K
  };

  const char* engineChoiceName(EngineChoice choice);

  // Called by the executors when a choice (or a fallback) is made: the
  // selected engine and a one-line reason. Empty = silent.
  using ChoiceLogger = function<void(EngineChoice choice, const char* reason)>;

  // ---------------------------------------------------------------------------
  // Features and the pure decision function
  // ---------------------------------------------------------------------------

  // What the decision depends on. The executors fill these from their
  // arguments (probing monotonicity); callers of the pure function fill them
  // by hand.
  struct ProblemTraits {
    Index dimension = 0;      // total problem dimension
    Index numFree = 0;        // free-block size (mixed problems)
    bool affineP = true;      // F(z) = M z + q?
    bool orthantKP = true;    // K = R^numFree x R_+^rest? (false: arbitrary projector)
    bool monotoneP = false;   // affine: sym(M) PSD; nonlinear: caller's knowledge
    bool warmStartP = false;  // is the start a previous, nearby solution?
  };

  // Is sym(M) = (M + M^T)/2 positive semidefinite, within a relative shift?
  // Decided by ONE Cholesky attempt on sym(M) + shift*I with
  // shift = relShift * (1 + max |sym(M)_ij|): success means every eigenvalue
  // exceeds -shift, i.e. monotone to working precision (the shift also
  // admits exactly singular PSD matrices, e.g. rank-deficient A^T A). One
  // O(d^3/3) factorization -- far cheaper than an eigensolve, and the answer
  // is only ever consumed as a boolean. Throws std::invalid_argument on an
  // empty or non-square M or a negative relShift.
  bool probeMonotone(const MatrixXd& M, double relShift = 1.0e-10);

  // The decision table (evidence in report Part III; the numbered rules are
  // in the .cpp beside their citations):
  //   affine, orthant K, monotone, no warm start  -> MehrotraIpm
  //   affine, orthant K, monotone, warm start     -> BsHe94b
  //   affine, monotone, non-orthant K             -> BsHe94b
  //   affine, not (known) monotone                -> ChainedSolodovHe
  //   nonlinear, orthant K                        -> SemismoothNewton
  //   nonlinear, non-orthant K                    -> JosephyNewton
  // Pure and deterministic; throws std::invalid_argument on nonsensical
  // traits (non-positive dimension, numFree out of range).
  EngineChoice chooseEngine(const ProblemTraits& traits);

  // ---------------------------------------------------------------------------
  // Executors
  // ---------------------------------------------------------------------------

  // Controls for solveAffineAuto. Iteration caps are per engine CLASS because
  // an "iteration" costs a back-substitution for the projection engines and a
  // full factorization for the interior-point engine.
  struct AutoAffineParams {
    double magTol = 1.0e-12;          // squared-residual stop (all engines)
    int projectionIterMax = 150000;   // bsHe94b / chain cap
    int ipmIterMax = 200;             // mehrotraIpm cap (counts factorizations)
    int iterFreq = 0;                 // logging frequency (<= 0 disables)
    double probeShiftRel = 1.0e-10;   // probeMonotone relative shift

    bool warmStartP = false;          // x0 is a previous, nearby solution
                                      //   (the caller knows; it cannot be probed)

    BsHe94bParams he;                 // per-engine tunables, used by whichever runs
    ChainedSolverParams chain;
    MehrotraIpmParams ipm;
    NewtonSolverFactory newtonFactory;  // optional structured factory for the IPM

    ChoiceLogger onChoice;            // observability hook (empty = silent)
  };

  // Solve the mixed affine LCP over K = R^numFree x R_+^rest with the engine
  // chooseEngine picks after probing monotonicity: MehrotraIpm (monotone, no
  // warm start; x0 is then ignored -- an interior-point method cannot use
  // it), BsHe94b (monotone, warm start), or ChainedSolodovHe (probe failed:
  // the globalizer's pseudomonotone theory is the widest net the affine menu
  // has; no convergence guarantee beyond it). For a NON-orthant K, pick an
  // engine directly -- this entry point is deliberately orthant-only, like
  // the interior-point engine it dispatches to.
  //
  // Throws std::invalid_argument on inconsistent inputs, and propagates the
  // chosen engine's exceptions unchanged (the dispatcher adds no rescue at
  // the affine level).
  VIResult solveAffineAuto(const VectorXd& x0,
                           const MatrixXd& M,
                           const VectorXd& q,
                           Index numFree,
                           const AutoAffineParams& params = AutoAffineParams{});

  // Controls for solveModelAuto. The chain sub-block configures the fallback
  // exactly as the deploy/SAOE tests run it: Josephy-Newton over the
  // interior-point inner solver as globalizer (under the no-progress
  // cutoff), the semismooth solver with nonmonotone memory as finisher.
  struct AutoModelParams {
    double magTol = 1.0e-10;          // squared natural-residual stop
    int ssnIterMax = 300;             // first-attempt semismooth cap
    int ssnNonmonotoneMemory = 4;     // SEMI production value (also the finisher's)

    int jnOuterIterMax = 50;          // chain globalizer: JN outer cap
    int jnStallIterMax = 5;           //   and no-progress cutoff
    double ipmInnerMagTol = 1.0e-12;  //   inner IPM tolerance
    int ipmInnerIterMax = 200;        //   inner IPM cap (factorizations)

    int chainRoundsMax = 8;           // fallback chain rounds
    double chainPerturbScale = 0.1;   // perturb-restart on stagnation

    ChoiceLogger onChoice;            // observability hook (empty = silent)
  };

  // Solve the nonlinear mixed NCP given as a VIModel, the way the evidence
  // says to: semismoothNewtonSolve first (it wins outright on problems up to
  // mildly nonmonotone); if it STALLS honestly (converged = false) or throws
  // its divergence guard (std::runtime_error), fall back to the alternating
  // chain -- the only configuration that has solved the genuinely nonmonotone
  // problems. std::invalid_argument (caller errors) propagates immediately;
  // the fallback is reported through onChoice, never taken silently. Returns
  // the fallback's result when it runs, the first attempt's otherwise.
  VIResult solveModelAuto(const VIModel& model,
                          const VectorXd& z0,
                          const AutoModelParams& params = AutoModelParams{});

} // namespace VINCP

#endif // VINCP_CHOOSEENGINE_HPP
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------

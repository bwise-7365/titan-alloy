// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
// chooseEngine: implementation of the probe, the decision table, and the
// executors.
// ----------------------------------------------
#include "chooseengine.hpp"

#include "josephynewton.hpp"

#include <exception>
#include <stdexcept>

namespace VIMCP {

  const char*
  engineChoiceName(EngineChoice choice)
  {
    switch (choice) {
      case EngineChoice::BsHe94b:          return "bsHe94b";
      case EngineChoice::ChainedSolodovHe: return "chainedSolodovHe";
      case EngineChoice::MehrotraIpm:      return "mehrotraIpm";
      case EngineChoice::SemismoothNewton: return "semismoothNewton";
      case EngineChoice::AlternatingChain: return "alternatingChain";
      case EngineChoice::JosephyNewton:    return "josephyNewton";
    }
    return "unknown";
  }

  bool
  probeMonotone(const MatrixXd& M, double relShift)
  {
    if (0 >= M.rows() || M.rows() != M.cols()) {
      throw std::invalid_argument("probeMonotone: M must be non-empty and square.");
    }
    if (0.0 > relShift) {
      throw std::invalid_argument("probeMonotone: relShift must be non-negative.");
    }
    MatrixXd sym = 0.5 * (M + M.transpose());
    const double shift = relShift * (1.0 + sym.cwiseAbs().maxCoeff());
    sym.diagonal().array() += shift;
    const LLT<MatrixXd> llt(sym);
    return (Success == llt.info());
  }

  EngineChoice
  chooseEngine(const ProblemTraits& traits)
  {
    if (0 >= traits.dimension) {
      throw std::invalid_argument("chooseEngine: dimension must be positive.");
    }
    if (0 > traits.numFree || traits.dimension < traits.numFree) {
      throw std::invalid_argument("chooseEngine: numFree must lie in [0, dimension].");
    }

    // The decision table, each rule with its evidence (report Part III):
    if (!traits.affineP) {
      // Nonlinear. The mixed NCP has a purpose-built direct solver (alloceff:
      // 14 iterations where both projection rows diverge); an arbitrary K
      // needs the projector-carrying outer driver.
      return traits.orthantKP ? EngineChoice::SemismoothNewton
                              : EngineChoice::JosephyNewton;
    }
    if (!traits.monotoneP) {
      // Monotonicity failed or is unknown: the Solodov-Svaiter globalizer's
      // pseudomonotone theory is the widest affine net; the contraction
      // engines' divergence guards fire off-monotone (lcp_random_test), and
      // the IPM's theory needs PSD outright.
      return EngineChoice::ChainedSolodovHe;
    }
    if (!traits.orthantKP) {
      // Monotone over a general projector K (e.g. an ellipsoid): the
      // interior-point engine is structurally orthant-only, so the
      // factor-once contraction engine is the workhorse.
      return EngineChoice::BsHe94b;
    }
    if (traits.warmStartP) {
      // A previous, nearby solution is worth more than the central path: the
      // IPM cannot use it (data-scaled interior start only), the contraction
      // engine exploits it fully.
      return EngineChoice::BsHe94b;
    }
    // Monotone, orthant, cold start: the interior-point engine's iteration
    // count is insensitive to dimension AND degeneracy (9/19/10 on clean/
    // degenerate/rank-deficient; 35-36 iterations at dimensions 10k-14.6k),
    // where projection-contraction stalls on flat optimal faces (the
    // 150,000-iteration banded cap).
    return EngineChoice::MehrotraIpm;
  }

  VIResult
  solveAffineAuto(const VectorXd& x0,
                  const MatrixXd& M,
                  const VectorXd& q,
                  Index numFree,
                  const AutoAffineParams& params)
  {
    validateLviInputs("solveAffineAuto", x0, M, q, makeMixedProjector(numFree));
    if (0 > numFree || M.rows() <= numFree) {
      throw std::invalid_argument(
          "solveAffineAuto: numFree must satisfy 0 <= numFree < dim.");
    }

    ProblemTraits traits;
    traits.dimension = M.rows();
    traits.numFree = numFree;
    traits.affineP = true;
    traits.orthantKP = true;
    traits.monotoneP = probeMonotone(M, params.probeShiftRel);
    traits.warmStartP = params.warmStartP;

    const EngineChoice choice = chooseEngine(traits);
    if (params.onChoice) {
      params.onChoice(choice,
                      traits.monotoneP
                          ? (traits.warmStartP
                                 ? "monotone with a warm start: contraction engine"
                                 : "monotone, cold start: interior point")
                          : "monotonicity probe failed: globalizer chain");
    }

    const Projector Pr = makeMixedProjector(numFree);
    switch (choice) {
      case EngineChoice::MehrotraIpm:
        return mehrotraIpm(M, q, numFree, params.magTol, params.ipmIterMax,
                           params.iterFreq, params.ipm, IterationLogger{},
                           params.newtonFactory);
      case EngineChoice::ChainedSolodovHe:
        return chainedSolodovHe(x0, M, q, Pr, params.magTol,
                                params.projectionIterMax, params.iterFreq,
                                params.chain);
      default:   // BsHe94b (the only other affine choice this entry can yield)
        return bsHe94b(x0, M, q, Pr, params.magTol, params.projectionIterMax,
                       params.iterFreq, params.he);
    }
  }

  VIResult
  solveModelAuto(const VIModel& model,
                 const VectorXd& z0,
                 const AutoModelParams& params)
  {
    // First attempt: the direct semismooth solver (the evidence-backed
    // opener for the mixed NCP). Its caller-error exceptions propagate; its
    // numerical failures -- an honest stall OR a runtime_error such as the
    // divergence guard -- trigger the fallback instead of ending the solve
    // (the same stance the alternating chain takes toward its stages).
    SemismoothNewtonParams ssnParams;
    ssnParams.magTol = params.magTol;
    ssnParams.iterMax = params.ssnIterMax;
    ssnParams.nonmonotoneMemory = params.ssnNonmonotoneMemory;

    if (params.onChoice) {
      params.onChoice(EngineChoice::SemismoothNewton,
                      "mixed NCP: direct semismooth Newton first");
    }
    bool fallBackP = false;
    const char* why = "";
    VIResult first;
    try {
      first = semismoothNewtonSolve(model, z0, ssnParams);
      if (first.converged) {
        return first;
      }
      fallBackP = true;
      why = "semismooth solver stalled honestly: alternating chain";
    }
    catch (const std::runtime_error&) {
      fallBackP = true;
      why = "semismooth solver hit a numerical guard: alternating chain";
    }
    if (!fallBackP) {
      return first;   // unreachable; keeps control flow explicit
    }

    if (params.onChoice) {
      params.onChoice(EngineChoice::AlternatingChain, why);
    }

    JosephyNewtonParams jnParams;
    jnParams.outerTol = params.magTol;
    jnParams.outerIterMax = params.jnOuterIterMax;
    jnParams.stallIterMax = params.jnStallIterMax;
    const InnerSolver inner = makeMehrotraIpmSolver(
        model.n, params.ipmInnerMagTol, params.ipmInnerIterMax, 0);
    const StageSolver globalizer = [model, inner, jnParams](const VectorXd& start) {
      return solveVI(model, start, inner, jnParams);
    };
    const StageSolver finisher = [model, ssnParams](const VectorXd& start) {
      return semismoothNewtonSolve(model, start, ssnParams);
    };

    AlternatingChainParams chainParams;
    chainParams.magTol = params.magTol;
    chainParams.roundsMax = params.chainRoundsMax;
    chainParams.perturbScale = params.chainPerturbScale;

    return alternatingChainSolve(model, z0, globalizer, finisher, chainParams);
  }

} // namespace VIMCP
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------

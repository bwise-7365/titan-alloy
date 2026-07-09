// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
// SAOE problem class implementation: dispatch to the library's solver engines
// and decode the equilibrium into the pinned aggregates plus one effort split.
// ----------------------------------------------
#include "saoeproblem.hpp"

#include "alternatingchain.hpp"
#include "chooseengine.hpp"
#include "josephynewton.hpp"
#include "saoe.hpp"
#include "semismoothnewton.hpp"

#include <cmath>
#include <cstdio>
#include <random>
#include <stdexcept>
#include <utility>

namespace VINCP::App {

  SAOE::SAOE(SaoeData instanceData)
    : data(std::move(instanceData))
  {
    return;
  }

  SaoeData
  SAOE::generate(const SaoeRandomSpec& spec)
  {
    if (spec.numActors <= 0 || spec.numOptions <= 0) {
      throw std::invalid_argument(
          "SAOE::generate: numActors and numOptions must be positive.");
    }
    if (spec.rewardHi < spec.rewardLo || spec.strengthHi < spec.strengthLo) {
      throw std::invalid_argument(
          "SAOE::generate: an upper bound is below its lower bound.");
    }

    std::mt19937_64 rng(spec.seed);
    std::uniform_real_distribution<double> rewardDist(spec.rewardLo, spec.rewardHi);
    std::uniform_real_distribution<double> strengthDist(spec.strengthLo, spec.strengthHi);

    SaoeData out;
    out.R.resize(spec.numActors, spec.numOptions);
    for (int i = 0; i < spec.numActors; ++i) {
      for (int j = 0; j < spec.numOptions; ++j) {
        out.R(i, j) = rewardDist(rng);
      }
    }
    out.S.resize(spec.numActors);
    for (int i = 0; i < spec.numActors; ++i) {
      double s = strengthDist(rng);
      if (spec.roundStrengthTenthsP) {
        s = std::round(s * 10.0) / 10.0;
      }
      out.S(i) = s;
    }
    return out;
  }

  const char*
  SAOE::answerContractNote()
  {
    return
        "At the SAOE equilibrium the per-option aggregates -- the option\n"
        "probabilities, and hence each actor's expected reward -- are unique\n"
        "(pinned). The per-actor effort split that realizes them is NOT unique:\n"
        "the effort matrix below is one representative of the solution set.";
  }

  // ---------------------------------------------------------------------------
  // The robust default: the alternating globalizer/finisher chain, in exactly
  // the configuration the saoe_chain_test proves reaches equilibrium E. This is
  // a production copy of the test-only makeSaoeChain recipe (test/saoesupport.hpp):
  // Josephy-Newton over an interior-point inner solver as globalizer (under the
  // no-progress cutoff), then semismooth Newton with nonmonotone memory as
  // finisher.
  // ---------------------------------------------------------------------------
  static VIResult
  solveSaoeChain(const VIModel& model, const VectorXd& z0, const SaoeParams& params)
  {
    JosephyNewtonParams jnParams;
    jnParams.outerTol     = params.magTol;
    jnParams.outerIterMax = params.jnOuterIterMax;
    jnParams.stallIterMax = params.jnStallIterMax;
    const InnerSolver inner =
        makeMehrotraIpmSolver(model.n, params.ipmInnerMagTol, params.ipmInnerIterMax, 0);
    const StageSolver globalizer = [&model, inner, jnParams](const VectorXd& start) {
      return solveVI(model, start, inner, jnParams);
    };

    SemismoothNewtonParams ssnParams;
    ssnParams.magTol            = params.magTol;
    ssnParams.iterMax           = params.ssnIterMax;
    ssnParams.nonmonotoneMemory = params.ssnNonmonotoneMemory;
    const StageSolver finisher = [&model, ssnParams](const VectorXd& start) {
      return semismoothNewtonSolve(model, start, ssnParams);
    };

    AlternatingChainParams chainParams;
    chainParams.magTol       = params.magTol;
    chainParams.roundsMax    = params.chainRoundsMax;
    chainParams.perturbScale = params.chainPerturbScale;

    ChainStageLogger stageLog = ChainStageLogger{};
    if (params.verbose) {
      stageLog = [](int round, const char* stage, double stageResidual,
                    double bestResidual, const string& note) {
        if (note.empty()) {
          std::printf("    saoe-chain round %d %s: residual^2 %.3e (best %.3e)\n",
                      round, stage, stageResidual, bestResidual);
        }
        else {
          std::printf("    saoe-chain round %d %s threw (stalled stage): %s\n",
                      round, stage, note.c_str());
        }
        std::fflush(stdout);
        return;
      };
    }

    return alternatingChainSolve(model, z0, globalizer, finisher, chainParams, stageLog);
  }

  // The chooseEngine dispatcher: semismooth first, alternating-chain fallback.
  static VIResult
  solveSaoeAuto(const VIModel& model, const VectorXd& z0, const SaoeParams& params)
  {
    AutoModelParams autoParams;
    autoParams.magTol               = params.magTol;
    autoParams.ssnIterMax           = params.ssnIterMax;
    autoParams.ssnNonmonotoneMemory = params.ssnNonmonotoneMemory;
    autoParams.jnOuterIterMax       = params.jnOuterIterMax;
    autoParams.jnStallIterMax       = params.jnStallIterMax;
    autoParams.ipmInnerMagTol       = params.ipmInnerMagTol;
    autoParams.ipmInnerIterMax      = params.ipmInnerIterMax;
    autoParams.chainRoundsMax       = params.chainRoundsMax;
    autoParams.chainPerturbScale    = params.chainPerturbScale;
    if (params.verbose) {
      autoParams.onChoice = [](EngineChoice, const char* reason) {
        std::printf("    saoe-auto: %s\n", reason);
        std::fflush(stdout);
        return;
      };
    }
    return solveModelAuto(model, z0, autoParams);
  }

  SAOE::Solution
  SAOE::solve(const Params& params) const
  {
    const VIModel model =
        saoeModel(data.R, data.S, params.riskAversion, params.epsilon);
    const VectorXd z0 = saoeDefaultStart(data.R, data.S);

    VIResult vi;
    switch (params.engine) {
      case ProblemBase::Engine::Default:
      case ProblemBase::Engine::Chain:
        vi = solveSaoeChain(model, z0, params);
        break;
      case ProblemBase::Engine::Auto:
        vi = solveSaoeAuto(model, z0, params);
        break;
      default:
        throw std::invalid_argument(
            "SAOE::solve: this problem honors only Engine::Chain (default) or "
            "Engine::Auto.");
    }

    const Index M = data.R.rows();
    const Index N = data.R.cols();
    const SaoeSolution decoded = saoeDecode(vi, M, N);
    const double eps = (0.0 < params.epsilon) ? params.epsilon : saoeEps(data.R);

    SaoeResult result;
    result.probabilities = saoeProbabilities(decoded.e, eps);
    result.utilities     = saoeUtilities(data.R, decoded.e, eps);
    result.e             = decoded.e;
    result.lambda        = decoded.lambda;

    return Solution{ vi, result };
  }

} // namespace VINCP::App
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------

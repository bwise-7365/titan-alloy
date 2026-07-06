// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
// Alternating globalizer/finisher chain: implementation.
// ----------------------------------------------
#include "alternatingchain.hpp"

#include <cmath>
#include <exception>
#include <limits>
#include <random>
#include <stdexcept>

namespace VINCP {

  namespace {

    void
    validateInputs(const VIModel& model, const VectorXd& z0,
                   const StageSolver& globalizer, const StageSolver& finisher,
                   const AlternatingChainParams& params)
    {
      if (0 > model.n || 0 > model.m || 0 == model.n + model.m) {
        throw std::invalid_argument(
            "alternatingChainSolve: model dimensions must be non-negative and not both zero.");
      }
      if (!model.H || !model.G) {
        throw std::invalid_argument("alternatingChainSolve: model.H and model.G must be set.");
      }
      if (z0.size() != model.n + model.m) {
        throw std::invalid_argument("alternatingChainSolve: z0 must have length n + m.");
      }
      if (!globalizer || !finisher) {
        throw std::invalid_argument(
            "alternatingChainSolve: both stage solvers must be set.");
      }
      if (!(0.0 < params.magTol)) {
        throw std::invalid_argument("alternatingChainSolve: magTol must be positive.");
      }
      if (1 > params.roundsMax) {
        throw std::invalid_argument("alternatingChainSolve: roundsMax must be at least 1.");
      }
      if (!(0.0 < params.improveFactor && params.improveFactor <= 1.0)) {
        throw std::invalid_argument(
            "alternatingChainSolve: improveFactor must lie in (0, 1].");
      }
      if (0.0 > params.perturbScale) {
        throw std::invalid_argument(
            "alternatingChainSolve: perturbScale must be non-negative.");
      }
      return;
    }

  } // namespace

  VIResult
  alternatingChainSolve(const VIModel& model,
                        const VectorXd& z0,
                        const StageSolver& globalizer,
                        const StageSolver& finisher,
                        const AlternatingChainParams& params,
                        const ChainStageLogger& logger,
                        const Projector& projector)
  {
    validateInputs(model, z0, globalizer, finisher, params);

    const double kInf = std::numeric_limits<double>::infinity();
    const Projector Pr = projector ? projector : makeMixedProjector(model.n);

    // The chain judges every point itself, by the library-standard squared
    // natural residual over Pr -- stage solvers' own conventions cannot skew
    // the best-point comparison. Non-finite model values away from the start
    // count as +infinity (the point is simply never "best").
    const auto residualAt = [&](const VectorXd& z) -> double {
      try {
        const VectorXd Fz = evaluateF(model, z);
        const VectorXd e = z - Pr(z - Fz);
        return e.squaredNorm();
      }
      catch (const std::exception&) {
        return kInf;
      }
    };

    // Best-visited point, seeded with the projected start. A non-finite model
    // value HERE still throws (a broken start is the caller's error -- no
    // stage has run yet); everywhere later, non-finite points simply never
    // become "best".
    VectorXd bestZ = Pr(z0);
    const VectorXd F0 = evaluateF(model, bestZ);
    double bestMag = (bestZ - Pr(bestZ - F0)).squaredNorm();

    int iterTotal = 0;
    int innerTotal = 0;

    // Run one stage from 'start': absorb a throw as a stalled stage, fold the
    // result into the best point, report, and hand back the stage's end point
    // (or 'start' unchanged if the stage threw / went non-finite).
    const auto runStage = [&](const StageSolver& stage, const char* tag,
                              int round, const VectorXd& start) -> VectorXd {
      try {
        const VIResult r = stage(start);
        iterTotal += r.iter;
        innerTotal += r.innerIters;
        const double mag = residualAt(r.z);
        if (mag < bestMag) {
          bestMag = mag;
          bestZ = r.z;
        }
        if (logger) {
          logger(round, tag, mag, bestMag, string{});
        }
        if (mag < kInf) {
          return r.z;
        }
        return start;
      }
      catch (const std::exception& ex) {
        if (logger) {
          logger(round, tag, kInf, bestMag, string(ex.what()));
        }
        return start;
      }
    };

    // Perturb-restart source: fixed seed, so a chain run is reproducible.
    std::mt19937 rng(20260706u);
    std::uniform_real_distribution<double> unit(-1.0, 1.0);

    bool perturbNextP = false;   // last round stagnated: jiggle this one's start
    for (int round = 1; round <= params.roundsMax; ++round) {
      const double bestBefore = bestMag;

      VectorXd start = bestZ;
      if (perturbNextP) {
        // Componentwise jiggle proportional to the natural-residual NORM, so
        // the perturbation shrinks as the chain closes in; the projection
        // below returns it to K.
        const double scale = params.perturbScale * std::sqrt(bestMag);
        for (Index i = 0; i < start.size(); ++i) {
          start(i) += scale * unit(rng);
        }
      }
      const VectorXd roundStart = Pr(start);
      const VectorXd handoff = runStage(globalizer, "globalize", round, roundStart);
      if (bestMag < params.magTol) {
        break;
      }

      runStage(finisher, "finish", round, handoff);
      if (bestMag < params.magTol) {
        break;
      }

      const bool improvedP = (bestMag < params.improveFactor * bestBefore);
      if (!improvedP && !(0.0 < params.perturbScale)) {
        break;   // stagnation with perturbation disabled ends the chain
      }
      perturbNextP = !improvedP;
    }

    const bool convergedP = (bestMag < params.magTol);
    return VIResult{ bestZ, bestMag, iterTotal, convergedP, innerTotal };
  }

} // namespace VINCP
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------

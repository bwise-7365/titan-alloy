// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
// Alternating globalizer/finisher chain for nonmonotone mixed NCPs.
// ----------------------------------------------
#ifndef VIMCP_ALTERNATINGCHAIN_HPP
#define VIMCP_ALTERNATINGCHAIN_HPP

// Composes two solver engines into rounds of
//     project onto K  ->  globalize  ->  finish
// with best-point memory, for mixed NCPs H(x,y) = 0, 0 <= G(x,y) _|_ y >= 0
// that are NONMONOTONE -- where no single engine in this library carries a
// global convergence theory and, empirically, none converges alone.
//
// The composition exploits COMPLEMENTARY failure modes (established on the
// deploy_v07 pre-position/deploy game, 2026-07-06, where this chain found the
// GAMS-verified equilibrium that every standalone engine missed):
//   - A Josephy-Newton/interior-point globalizer makes large gains from far
//     away (its central-path steps are insensitive to degeneracy) but stalls
//     where the linearized LCP goes indefinite -- a property of the point it
//     linearizes at.
//   - A semismooth Newton finisher descends sharply near the solution set
//     (locally superlinear once inside a basin) but its iterates are not
//     confined to K: on models whose vector field has poles just outside the
//     orthant (e.g. eps-guarded ratio denominators), a stalled finisher is
//     typically camped beside a pole where its line search dies against
//     merit walls, NOT at a merit-stationary point.
// Each round therefore (1) PROJECTS the iterate back onto K -- repairing
// exactly the way the finisher poisons a handoff -- then (2) re-runs the
// globalizer, re-linearizing at a point the finisher improved, then (3)
// re-runs the finisher from the globalizer's result. A stage that THROWS is
// treated as a stalled stage, not a failure of the chain (the same attitude
// PATH takes to subsolver failure: switch strategy, do not abort); the round
// simply continues with its other stage from the last good point.
//
// The chain keeps the BEST point visited (in the library-standard squared
// natural-residual sense, recomputed by the chain itself so the stage
// solvers' own conventions cannot skew it) and returns that point, not the
// last one. It stops on convergence (best residual < magTol) or at the round
// cap. A round that fails to improve the best is STAGNATION: since the chain
// is deterministic, retrying it verbatim would repeat identically, so with
// perturbScale > 0 the next round restarts from the best point jiggled in
// proportion to the current error (PATH's perturbation strategy), and with
// perturbScale = 0 stagnation ends the chain. 'converged' is honest either
// way.
//
// Sources / precedents: T. S. Munson, "Algorithms and Environments for
// Complementarity" and Ferris-Munson (PATH's crash phase and restart
// strategies; UW-Madison TR 98-12); De Luca-Facchinei-Kanzow 2000 and Munson
// et al. 2001 for the semismooth finisher's theory; this library's
// chainedSolodovHe (chainedsolver.hpp) is the same globalizer->finisher
// composition one level down, without rounds or projection.

#include "vimcp.hpp"

#include <functional>
#include <string>

using std::string;

namespace VIMCP {

  // A stage solver: the caller binds a complete engine (model, parameters,
  // inner solvers, per-iteration logging) leaving only the start point. Both
  // stages receive starts INSIDE K when the chain calls them (the round start
  // is projected; the finisher starts from the globalizer's result, or from
  // the projected round start if the globalizer stalled by throwing).
  using StageSolver = function<VIResult(const VectorXd& start)>;

  // Per-stage report hook, called once after each stage of each round:
  // 1-based round, stage tag ("globalize" / "finish"), the squared natural
  // residual of the stage's result (+infinity if the stage threw), the best
  // residual so far, and a note (empty normally; the exception message when
  // the stage threw). Empty logger = silent.
  using ChainStageLogger =
      function<void(int round, const char* stage, double stageResidual,
                    double bestResidual, const string& note)>;

  struct AlternatingChainParams {
    double magTol = 1.0e-12;      // acceptance on the SQUARED natural residual
    int    roundsMax = 5;         // hard cap on rounds
    double improveFactor = 1.0;   // a round COUNTS AS IMPROVING only if it made
                                  //   bestResidual < improveFactor * previous
                                  //   best. The default 1.0 = ANY strict
                                  //   improvement (roundsMax alone bounds the
                                  //   cost); smaller values demand proportional
                                  //   gains per round -- brittle near a
                                  //   solution, where rounds grind small gains
                                  //   just before the finisher's quadratic
                                  //   tail engages. In (0, 1].
    double perturbScale = 0.0;    // perturb-restart on stagnation (PATH-style).
                                  //   The chain is deterministic, so a round
                                  //   that starts from the projected best point
                                  //   and fails to improve would repeat
                                  //   IDENTICALLY forever; with perturbScale
                                  //   > 0 the next round instead starts from
                                  //   bestZ jiggled componentwise by
                                  //   U[-1, 1] * perturbScale * sqrt(bestMag)
                                  //   (i.e. proportional to the current
                                  //   natural-residual NORM -- vanishing as the
                                  //   chain closes in), then projected onto K.
                                  //   The jiggle is drawn from a fixed-seed
                                  //   generator, so runs stay reproducible.
                                  //   0 disables: a non-improving round ends
                                  //   the chain. Must be non-negative.
  };

  // Run the alternating chain on 'model' from 'z0'. K enters through
  // 'projector' (default: makeMixedProjector(model.n), matching the model's
  // free/non-negative split); the natural residual the chain steers and
  // reports by is taken over that projector. VIResult::z and ::residual are
  // the best point visited; ::iter and ::innerIters accumulate the stages'
  // own counts across all rounds.
  //
  // Throws std::invalid_argument on an inconsistent model or start, unset
  // stage solvers, or out-of-range parameters. Stage exceptions
  // (std::exception) are absorbed as stalled stages -- see above; they are
  // reported through the logger, never rethrown. A non-finite model value at
  // the PROJECTED START is still an error (propagated from evaluateF): the
  // caller handed the chain a broken start, and no stage has run yet.
  VIResult alternatingChainSolve(const VIModel& model,
                                 const VectorXd& z0,
                                 const StageSolver& globalizer,
                                 const StageSolver& finisher,
                                 const AlternatingChainParams& params =
                                     AlternatingChainParams{},
                                 const ChainStageLogger& logger = ChainStageLogger{},
                                 const Projector& projector = Projector{});

} // namespace VIMCP

#endif // VIMCP_ALTERNATINGCHAIN_HPP
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------

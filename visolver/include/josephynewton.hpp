// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
// Outer Josephy-Newton driver for the nonlinear variational inequality.
// ----------------------------------------------
#ifndef VIMCP_JOSEPHYNEWTON_HPP
#define VIMCP_JOSEPHYNEWTON_HPP

//     find z in K such that F(z) . (w - z) >= 0 for all w in K.
//
// K enters only through its projector Pi_K, supplied to solveVI (default: the
// mixed set K = R^n x R_+^m with z = (x, y), x free and y non-negative, matching
// the model's H(x,y) = 0, 0 <= G(x,y) _|_ y >= 0 split; F = (H, G)). Any other
// projector -- e.g. an ellipsoid -- solves the same VI over a different K.
//
// Each outer step linearizes F at the current iterate with a finite-difference
// Jacobian J(z_k) and solves the resulting affine VI over the same K:
//     M = J(z_k),  q = F(z_k) - J(z_k) z_k,  Pr = the supplied projector,
//     z_{k+1} = innerSolver(z_k, M, q, Pr, ...).z
// The structure of K is carried entirely by Pr; no Schur complement or
// elimination of any block is needed.

#include "vimcp.hpp"
#include "dhan06.hpp"
#include "bshe94b.hpp"
#include "solodovsvaiter.hpp"
#include "chainedsolver.hpp"
#include "mehrotraipm.hpp"
#include "fbshyz04.hpp"
#include "fdjacobian.hpp"
#include "armijo.hpp"

#include <Eigen/Dense>
#include <functional>

namespace VIMCP {

  // Optional per-outer-iteration logging hook (opt-in, like the inner solver's).
  using OuterLogger =
      function<void(int iter, int iterMax, double residual, double tol)>;

  // Inner LVI solver seam. Each outer step hands the linearized affine VI to this
  // functor: given the start x0, matrix M, vector q, and projector Pr, it returns
  // the solution as a VIResult. Any inner solver (dHan06, bsHe94b, ...) is adapted
  // to it by binding its tolerances/caps/params/logger -- see the make*Solver
  // helpers below.
  using InnerSolver =
      function<VIResult(const VectorXd& x0,
                        const MatrixXd& M,
                        const VectorXd& q,
                        const Projector& Pr)>;

  // Adapters that bind an inner solver's controls into an InnerSolver functor.
  InnerSolver makeDHan06Solver(double magTol, int iterMax, int iterFreq,
                               const DHan06Params& params = DHan06Params{},
                               const IterationLogger& logger = IterationLogger{});
  InnerSolver makeBsHe94bSolver(double magTol, int iterMax, int iterFreq,
                                const BsHe94bParams& params = BsHe94bParams{},
                                const IterationLogger& logger = IterationLogger{});
  InnerSolver makeSolodovSvaiterSolver(double magTol, int iterMax, int iterFreq,
                                       const SolodovSvaiterParams& params =
                                           SolodovSvaiterParams{},
                                       const IterationLogger& logger =
                                           IterationLogger{});
  InnerSolver makeChainedSolver(double magTol, int iterMax, int iterFreq,
                                const ChainedSolverParams& params =
                                    ChainedSolverParams{},
                                const IterationLogger& logger =
                                    IterationLogger{});
  InnerSolver makeFbsHyz04Solver(double magTol, int iterMax, int iterFreq,
                                 const FbsHyz04Params& params = FbsHyz04Params{},
                                 const IterationLogger& logger =
                                     IterationLogger{});

  // Inner-solver FACTORY seam: given the squared inner tolerance to solve to,
  // return a bound InnerSolver. Lets the outer loop choose the inner
  // tolerance PER ITERATION (the inexact-Newton forcing sequence below);
  // adapt any engine with a one-line lambda over its make*Solver adapter,
  //     [=](double tol) { return makeBsHe94bSolver(tol, iterMax, 0); }.
  using InnerSolverFactory = function<InnerSolver(double innerMagTol)>;

  // Adapter for the interior-point engine. Unlike the projection adapters it
  // needs the free/complementarity split up front, and the returned functor
  // IGNORES both the start x0 (an interior-point method cannot warm-start)
  // and the projector Pr: it structurally assumes K = R^numFree x R_+^rest.
  // Bind numFree = model.n when driving solveVI with its default mixed
  // projector; pairing this adapter with any other K (e.g. an ellipsoid)
  // silently solves the wrong problem, so use it only where the mixed/orthant
  // structure it encodes is the K actually intended.
  InnerSolver makeMehrotraIpmSolver(Index numFree,
                                    double magTol, int iterMax, int iterFreq,
                                    const MehrotraIpmParams& params =
                                        MehrotraIpmParams{},
                                    const IterationLogger& logger =
                                        IterationLogger{});

  // Tunable controls for the outer loop.  Defaults are reasonable starting points,
  // not verified against a particular model; expect to tune per problem. (The
  // inner LVI solver and its tolerances are supplied separately as the InnerSolver
  // argument to solveVI.)
  struct JosephyNewtonParams {
    double outerTol      = 1.0e-12;  // on the SQUARED natural residual ||r(z)||^2
    int    outerIterMax  = 200;      // outer iteration cap
    int    outerIterFreq = 0;        // logging frequency (<= 0 disables logging)

    // Central-difference Jacobian relative step (<= 0 => default eps^(1/5)).
    double fdStepRel = -1.0;

    // Inexact-Newton forcing sequence (Dembo-Eisenstat-Steihaug; the exact
    // rule of the reference Octave scripts, min(5e-4, n0/100)). Consumed
    // ONLY by the InnerSolverFactory overload of solveVI: at each outer
    // iteration the inner SQUARED tolerance is
    //     innerTol_k = clamp(forcingRatio * residual_k,
    //                        forcingFloor, forcingCap),
    // with residual_k the current squared natural residual -- loose inner
    // solves far from the solution (where precision would be wasted: the
    // linearization is about to move anyway), tight ones near it (where the
    // outer floor is set by the inner tolerance). Validated by that
    // overload: all three positive, forcingFloor <= forcingCap,
    // forcingRatio < 1.
    double forcingCap   = 5.0e-4;
    double forcingRatio = 1.0e-2;
    double forcingFloor = 1.0e-14;

    // Cheap no-progress cutoff: stop honestly (converged = false) after this
    // many CONSECUTIVE outer iterations in which the residual fails to improve
    // the best value seen so far by at least the relative factor
    // stallRelDecrease (progress means residual <= (1 - stallRelDecrease) *
    // bestResidual). 0 disables the guard (historical behavior). Motivation:
    // on a nonmonotone problem every linearized step can be rejected by the
    // Armijo damping, and without this guard the loop burns outerIterMax
    // expensive inner solves at a frozen residual (observed on the deploy_v07
    // GAMS model, 2026-07-06). stallIterMax must be non-negative and
    // stallRelDecrease must lie in [0, 1); std::invalid_argument otherwise.
    int    stallIterMax     = 0;
    double stallRelDecrease = 1.0e-3;

    // Armijo damping of the outer (Josephy-Newton) step on the natural-map
    // merit, to prevent overshoot near the non-smooth solution.
    ArmijoParams armijo = ArmijoParams{};

    // Diagnostic probe: when true, print (to stdout) the smallest eigenvalue of
    // the symmetric part of the linearized inner matrix M = J(z_k) at each outer
    // step. M is monotone (Han 2006 Thm 2.4's hypothesis for dHan06 convergence)
    // iff that eigenvalue is >= 0; a negative value flags a non-monotone inner
    // problem on which dHan06 may diverge while bsHe94b still contracts. Off by
    // default; it costs one dense symmetric eigensolve per outer iteration.
    bool logInnerDefiniteness = false;
  };

  // Solve the nonlinear VI over a feasible set K by Josephy-Newton with an
  // Armijo-damped step, using 'innerSolver' for each linearized affine VI.
  // The merit is the natural residual r(z) = z - Pi_K(z - F(z)); the loop stops when
  // ||r(z)||^2 < outerTol.  Returns the shared VIResult: its 'residual' is that
  // squared natural residual and 'converged' is true iff it fell below outerTol
  // within outerIterMax steps (no error thrown). A run cut off by the
  // no-progress guard (stallIterMax above) likewise returns honestly with
  // converged = false at the stalled iterate.
  //
  // K enters only through 'projector' (Pi_K), used both for the merit and by the
  // inner solver. If left empty it defaults to makeMixedProjector(model.n) -- the
  // mixed free/non-negative set matching the model's (x, y) split -- so existing
  // callers are unaffected. Pass any Projector (e.g. makeEllipsoidProjector) to solve
  // the same VI over a different K.
  //
  // Throws std::invalid_argument on an inconsistent model, starting point, or an
  // unset innerSolver, and propagates the inner solver's std::runtime_error on a
  // failed linear solve, a NaN, or detected divergence. It never silently
  // substitutes a result.
  VIResult solveVI(const VIModel& model,
                   const VectorXd& z0,
                   const InnerSolver& innerSolver,
                   const JosephyNewtonParams& params = JosephyNewtonParams{},
                   const OuterLogger& logger = OuterLogger{},
                   const Projector& projector = Projector{});

  // The FORCING-SEQUENCE overload: identical outer loop, but the inner
  // solver is rebuilt each outer iteration by 'innerFactory' at the
  // tolerance the forcing schedule dictates (see JosephyNewtonParams).
  // Compared with binding one tight tolerance up front, the early inner
  // solves are dramatically cheaper at no cost to the final accuracy --
  // the profile of the reference Octave runs (thousands of inner
  // iterations early, ~a hundred late). Throws std::invalid_argument on an
  // unset factory or forcing parameters outside their ranges, and
  // std::runtime_error if the factory returns an empty solver.
  VIResult solveVI(const VIModel& model,
                   const VectorXd& z0,
                   const InnerSolverFactory& innerFactory,
                   const JosephyNewtonParams& params = JosephyNewtonParams{},
                   const OuterLogger& logger = OuterLogger{},
                   const Projector& projector = Projector{});

  // Plain-vanilla Josephy-Newton in ONE call, for callers who want simple
  // and fast: bsHe94b inner (fixed contractive metric, factored once per
  // linearization) under the forcing sequence, every other control at its
  // default. Deliberately no basin control: on problems with multiple
  // equilibria it converges quickly to WHICHEVER equilibrium its trajectory
  // enters -- deliberate equilibrium selection is the alternating chain's
  // job (alternatingchain.hpp). Throws as solveVI does.
  VIResult solveVIVanilla(const VIModel& model,
                          const VectorXd& z0,
                          double outerTol = 1.0e-10,
                          int outerIterMax = 100);

} // namespace VIMCP

#endif // VIMCP_JOSEPHYNEWTON_HPP
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------

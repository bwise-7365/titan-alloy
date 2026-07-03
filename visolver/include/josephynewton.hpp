// Copyright Ben Paul Wise. All Rights Reserved.
#ifndef VINCP_JOSEPHYNEWTON_HPP
#define VINCP_JOSEPHYNEWTON_HPP

// ============================================================================
// Outer Josephy-Newton driver for the variational inequality
//
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
// ============================================================================

#include "vincp.hpp"
#include "dhan06.hpp"
#include "bshe94b.hpp"
#include "fdjacobian.hpp"
#include "armijo.hpp"

#include <Eigen/Dense>
#include <functional>

namespace VINCP {

// Optional per-outer-iteration logging hook (opt-in, like the inner solver's).
using OuterLogger =
    std::function<void(int iter, int iterMax, double residual, double tol)>;

// Inner LVI solver seam. Each outer step hands the linearized affine VI to this
// functor: given the start x0, matrix M, vector q, and projector Pr, it returns
// the solution as a VIResult. Any inner solver (dHan06, bsHe94b, ...) is adapted
// to it by binding its tolerances/caps/params/logger -- see the make*Solver
// helpers below.
using InnerSolver =
    std::function<VIResult(const VectorXd& x0,
                           const Eigen::MatrixXd& M,
                           const VectorXd& q,
                           const Projector& Pr)>;

// Adapters that bind an inner solver's controls into an InnerSolver functor.
InnerSolver makeDHan06Solver(double magTol, int iterMax, int iterFreq,
                             const DHan06Params& params = DHan06Params{},
                             const IterationLogger& logger = IterationLogger{});
InnerSolver makeBsHe94bSolver(double magTol, int iterMax, int iterFreq,
                              const BsHe94bParams& params = BsHe94bParams{},
                              const IterationLogger& logger = IterationLogger{});

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
// within outerIterMax steps (no error thrown).
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

} // namespace VINCP

#endif // VINCP_JOSEPHYNEWTON_HPP
// Copyright Ben Paul Wise. All Rights Reserved.

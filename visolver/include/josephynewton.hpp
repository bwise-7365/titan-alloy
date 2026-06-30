#ifndef VINCP_JOSEPHYNEWTON_HPP
#define VINCP_JOSEPHYNEWTON_HPP

// ============================================================================
// Outer Josephy-Newton driver for the mixed variational inequality
//
//     find z in K such that F(z) . (w - z) >= 0 for all w in K,
//
// over K = R^n x R_+^m, with z = (x, y), x in R^n free and y in R^m
// non-negative.  Equivalently  H(x, y) = 0  and  0 <= G(x, y) _|_ y >= 0,
// where F = (H, G).
//
// Each outer step linearizes F at the current iterate with a finite-difference
// Jacobian J(z_k) and solves the resulting affine VI over the same K:
//     M = J(z_k),  q = F(z_k) - J(z_k) z_k,  Pr = makeMixedProjector(n),
//     z_{k+1} = dHan06(z_k, M, q, Pr, ...).z
// The mixed free/non-negative structure of K is carried entirely by Pr; no
// Schur complement or elimination of the free block is needed.
// ============================================================================

#include "vincp.hpp"
#include "dhan06.hpp"
#include "fdjacobian.hpp"

#include <Eigen/Dense>
#include <functional>

namespace VINCP {

// Optional per-outer-iteration logging hook (opt-in, like the inner solver's).
using OuterLogger =
    std::function<void(int iter, int iterMax, double residual, double tol)>;

// Tunable controls for the outer loop and the inner LVI solves.  Defaults are
// reasonable starting points, not verified against a particular model; expect
// to tune the tolerances per problem.
struct JosephyNewtonParams {
    // Outer loop.
    double outerTol      = 1.0e-12;  // on the SQUARED natural residual ||r(z)||^2
    int    outerIterMax  = 200;      // outer iteration cap
    int    outerIterFreq = 0;        // logging frequency (<= 0 disables logging)

    // Central-difference Jacobian relative step (<= 0 => default eps^(1/3)).
    double fdStepRel = -1.0;

    // Inner LVI solver (dHan06) controls.
    double       innerMagTol  = 1.0e-14;
    int          innerIterMax = 100000;
    int          innerIterFreq = 0;
    DHan06Params innerParams  = DHan06Params{};
};

// Solve the mixed nonlinear complementarity VI by Josephy-Newton with
// a full (undamped) step.
// The merit is the natural residual r(z) = z - Pi_K(z - F(z)); the loop stops when
// ||r(z)||^2 < outerTol.  Returns the shared VIResult: its 'residual' is that
// squared natural residual and 'converged' is true iff it fell below outerTol
// within outerIterMax steps (no error thrown).
//
// Throws std::invalid_argument on an inconsistent model or starting point, and
// propagates the inner solver's std::runtime_error on a failed linear solve,
// a NaN, or detected divergence.  It never silently substitutes a result.
VIResult solveVI(const VIModel& model,
                 const VectorXd& z0,
                 const JosephyNewtonParams& params = JosephyNewtonParams{},
                 const OuterLogger& logger = OuterLogger{});

} // namespace VINCP

#endif // VINCP_JOSEPHYNEWTON_HPP

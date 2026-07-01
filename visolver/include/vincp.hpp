// Copyright Ben Paul Wise. All Rights Reserved.
#ifndef VINCP_HPP
#define VINCP_HPP

// ============================================================================
// Core domain types for the variational inequality / mixed nonlinear complementarity
// (VINCP) solvers: shared by every solver and independent of any one of them.
// Solves H(x,y) = 0, 0 <= G(x,y) ⊥ y => 0.
//
//   - VIResult : the single result type returned by all solvers.
//   - VIModel  : the problem the solvers act on (free block x, non-negative
//                block y, with F = (H, G)).
//   - Projector and the ready-made projections onto K.
//
// The solvers (dhan06.hpp, josephynewton.hpp) include this header; this header
// includes none of them.
// ============================================================================

#include <Eigen/Dense>
#include <functional>

namespace VINCP {

    using Eigen::VectorXd;

// ---------------------------------------------------------------------------
// Result
// ---------------------------------------------------------------------------

// The single result type for every solver in this library.  'residual' is a
// SQUARED Euclidean norm (the same quantity the solvers compare their
// tolerances against), not a plain norm.  'converged' reports whether the
// residual fell below the tolerance within the iteration cap; it is set
// honestly rather than thrown on, so a non-converged run still returns.
struct VIResult {
    VectorXd z;          // solution vector
    double residual  = 0.0;     // squared residual norm at termination
    int    iter      = 0;       // iterations performed
    bool   converged = false;   // residual fell below tolerance within the cap
};

// ---------------------------------------------------------------------------
// Feasible set K and its projections
// ---------------------------------------------------------------------------

// Projection onto the convex set K that defines the variational inequality.
// Given a point v in R^n, returns the Euclidean projection of v onto K.
using Projector = std::function<VectorXd(const VectorXd&)>;

// Optional per-iteration logging hook for the inner LVI solvers (dHan06,
// bsHe94b, ...), called at the requested frequency. If empty, no logging is
// performed. Shared here so every inner solver takes the same logger type.
using IterationLogger =
    std::function<void(int iter, int iterMax, double mag, double magTol)>;

// Projection onto the non-negative orthant R_+^n: componentwise max(v, 0).
VectorXd projectNonnegative(const VectorXd& v);

// Projector onto K = R^numFree x R_+^(n - numFree): the first 'numFree'
// components are 'x'; the remaining components are 'y'.
// The 'x' terms can be + or - (often Lagrange multipliers of equality constraints)
// and the 'y' terms must be non-negative (often choices of physical quantities).
// This matches the set K for the (x, y) partition, with numFree equal to
// the dimension of the free block x.
Projector makeMixedProjector(Eigen::Index numFree);

// ---------------------------------------------------------------------------
// Model
// ---------------------------------------------------------------------------

// Caller-supplied model.  The full variable is z = (x, y) with the free block
// x in R^n and the non-negative block y in R^m.  H and G receive the current
// split z = (x, y) and return the free-block and non-negative-block components of
// F(z) = (H(x,y), G(x,y)), respectively.
struct VIModel {
    Eigen::Index n = 0;   // dimension of the free block x
    Eigen::Index m = 0;   // dimension of the non-negative block y

    std::function<VectorXd(const VectorXd& x,
                                  const VectorXd& y)> H;   // -> R^n
    std::function<VectorXd(const VectorXd& x,
                                  const VectorXd& y)> G;   // -> R^m
};

// Evaluate F(z) = (H(x, y), G(x, y)) with x = z.head(n), y = z.tail(m).
// Validates the model and the returned block lengths; throws on a dimension
// mismatch or a non-finite value.
VectorXd evaluateF(const VIModel& model, const VectorXd& z);

// Build a VIModel from a single full field F: R^(n+m) -> R^(n+m), splitting the
// output into H = F(z).head(n) and G = F(z).tail(m) with z = (x, y). Convenience
// for the common case where the map is defined on the whole vector at once.
VIModel makeVIModel(Eigen::Index n, Eigen::Index m,
                    std::function<VectorXd(const VectorXd&)> F);

// ---------------------------------------------------------------------------
// Inner (linear-VI) solver input validation
// ---------------------------------------------------------------------------

// Validate the inputs common to every inner LVI solver. 'who' prefixes the error
// message (e.g. "dHan06"). Throws std::invalid_argument on an empty x0, an M that
// is not square and conformant with x0, a non-conformant q, or an unset Pr.
void validateLviInputs(const char* who,
                       const VectorXd& x0, const Eigen::MatrixXd& M,
                       const VectorXd& q, const Projector& Pr);

} // namespace VINCP

#endif // VINCP_HPP
// Copyright Ben Paul Wise. All Rights Reserved.

#ifndef VINCP_DHAN06_HPP
#define VINCP_DHAN06_HPP

// ============================================================================
// C++20 / Eigen translation of the GNU Octave routine dHan06.
//
// Solves the linear variational inequality (LVI)
//     find x in K such that (M x + q) . (w - x) >= 0  for all w in K
// by the self-adaptive projection method of:
//     Deren Han, "Solving linear variational inequality problems by a
//     self-adaptive projection method", 2006.
//
// The set K enters only through the projector Pr.  The translation preserves
// the numerics of the reference implementation; the algorithmic constants are
// exposed as a parameter struct whose defaults reproduce the Octave source.
// ============================================================================

#include <Eigen/Dense>
#include <functional>
#include "vincp.hpp"

namespace VINCP {

// Optional per-iteration logging hook, called at the requested frequency.
// If empty, no logging is performed (the Octave printf is opt-in here).
using IterationLogger =
    std::function<void(int iter, int iterMax, double mag, double magTol)>;

// Tunable constants of Han's method. Defaults match the Octave source.
struct DHan06Params {
    double gamma = 1.6;               // relaxation factor, must satisfy 0 < gamma < 2
    double mu = 1.05;                 // Han's constant (mu > 0)
    double beta0 = 0.5;               // initial beta, must be > 0
    double tau0 = 0.5;                // initial tau
    int    tauN = 10;                 // 'n' in the tau schedule
    double divergenceFactor = 100.0;  // guard: mag must stay below factor * initialMag
};

// One element of the tau(k) schedule. Mirrors the Octave sub-function: returns
// t0 for k <= n, and 2*t0*n^2 / (n^2 + k^2) for k > n.
double tau(double t0, int n, int k);

// Solve the LVI by Han's self-adaptive projection method.
//
//   x0       starting point (also fixes the problem dimension n)
//   M        n-by-n matrix
//   q        n-vector
//   Pr       projector onto K
//   magTol   termination tolerance on the squared residual norm
//   iterMax  iteration cap
//   iterFreq logging frequency (<= 0 disables logging)
//   params   tunable constants
//   logger   optional logging hook
//
// Throws std::invalid_argument on inconsistent dimensions or invalid
// parameters, and std::runtime_error on a NaN residual, detected divergence,
// or a non-finite linear solve. It never silently substitutes a default result.
VIResult dHan06(const VectorXd& x0,
                    const Eigen::MatrixXd& M,
                    const VectorXd& q,
                    const Projector& Pr,
                    double magTol,
                    int iterMax,
                    int iterFreq,
                    const DHan06Params& params = DHan06Params{},
                    const IterationLogger& logger = IterationLogger{});

} // namespace VINCP

#endif // VINCP_DHAN06_HPP

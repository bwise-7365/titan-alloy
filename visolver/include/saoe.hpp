// Copyright Ben Paul Wise. All Rights Reserved.
#ifndef VINCP_SAOE_HPP
#define VINCP_SAOE_HPP

// ============================================================================
// SAOE -- Strategic Allocation Of Effort.
//
// M actors each split a strength S_i across N options to maximize expected
// reward given every other actor. With reward matrix R (M x N), effort e_{ij},
// column effort E_j = sum_i e_{ij} + eps, D = sum_k E_k, option probability
// P_j = E_j / D, and utility u_i = sum_j R_{ij} P_j, the Nash equilibrium is a
// nonlinear complementarity problem: all variables non-negative, no free block.
//
// Variables y = [ e (row-major, N*M), lambda (M) ], all >= 0. The complementarity
// map G(y) (0 <= G _|_ y >= 0) is
//     effort block:  G_{ij} = lambda_i - dU_{ij},  dU_{ij} = (R_{ij} - u_i)/D
//     budget block:  G_i    = S_i - sum_j e_{ij}
// solved via the library's VI machinery (makeVIModel with a zero free block).
// ============================================================================

#include "vincp.hpp"

#include <Eigen/Dense>
#include <random>

namespace VINCP {

// Which inner LVI solver the Josephy-Newton loop uses.
enum class InnerMethod { Han, He };

// Controls for the SAOE solve (forwarded to the Josephy-Newton outer loop and
// the inner LVI solver).
struct SaoeParams {
    double      outerTol     = 1.0e-10;         // squared natural-residual stop (solveVI)
    int         outerIterMax = 250;             // outer cap (a contest may need > the default)
    double      innerMagTol  = 1.0e-14;         // inner LVI squared-residual tolerance
    int         innerIterMax = 1000;          // inner iteration cap
    InnerMethod innerMethod  = InnerMethod::Han; // dHan06 (default) or bsHe94b

    // Diagnostic probe forwarded to solveVI: print the smallest eigenvalue of the
    // symmetric part of each outer Jacobian, to check whether the inner problem is
    // monotone (a prerequisite for dHan06's convergence). Off by default.
    bool        logInnerDefiniteness = false;
};

// Result of a SAOE solve.
struct SaoeResult {
    Eigen::MatrixXd e;        // M x N equilibrium efforts
    Eigen::VectorXd lambda;   // M multipliers (marginal value of strength)
    VIResult        solve;    // raw solver result (z, residual, iter, converged)
};

// The model's small effort constant, eps = RMS(R)/10000 = ||R||_F / sqrt(M N) / 1e4.
double saoeEps(const Eigen::MatrixXd& R);

// Option probabilities P_j from an effort matrix e (M x N): E_j = sum_i e_{ij} + eps,
// P_j = E_j / sum_k E_k. Returns a length-N vector.
Eigen::VectorXd saoeProbabilities(const Eigen::MatrixXd& e, double eps);

// Per-actor expected rewards u_i = sum_j R_{ij} P_j. Returns a length-M vector.
Eigen::VectorXd saoeUtilities(const Eigen::MatrixXd& R, const Eigen::MatrixXd& e,
                              double eps);

// A random starting point y0 = [e (row-major), lambda], all non-negative: efforts
// e_{ij} ~ U[0, S_i/N] and lambda_i ~ U[0, 1]. Use it to explore which of the
// (possibly many) equilibria the solver reaches from different basins.
Eigen::VectorXd saoeRandomStart(const Eigen::MatrixXd& R, const Eigen::VectorXd& S,
                                std::mt19937_64& rng);

// Solve the SAOE Nash equilibrium for reward matrix R (M x N) and strengths S (M).
// Builds the NCP (0 free components, N*M + M non-negative) and solves with solveVI.
// The optional z0 is the starting point (layout as saoeRandomStart); if empty, the
// default start e_{ij} = S_i/(N+1), lambda = 0 is used.
// Throws std::invalid_argument on an empty R, S.size() != R.rows(), or a z0 whose
// length is neither 0 nor N*M + M.
SaoeResult saoe(const Eigen::MatrixXd& R, const Eigen::VectorXd& S,
                const SaoeParams& params = SaoeParams{},
                const Eigen::VectorXd& z0 = Eigen::VectorXd());

} // namespace VINCP

#endif // VINCP_SAOE_HPP
// Copyright Ben Paul Wise. All Rights Reserved.

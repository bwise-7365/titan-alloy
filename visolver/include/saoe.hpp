// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
// SAOE -- Strategic Allocation Of Effort (Nash equilibrium as an NCP).
// ----------------------------------------------
#ifndef VINCP_SAOE_HPP
#define VINCP_SAOE_HPP

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

#include "vincp.hpp"

#include <Eigen/Dense>
#include <random>

namespace VINCP {

  // Which inner LVI solver the Josephy-Newton loop uses.
  enum class InnerMethod { Han, He };

  // ---------------------------------------------------------------------------
  // Risk aversion (the PME framework paper, section "Risk Aversion in
  // Coalition Formation", eq. 4.53; functional form as in the reference
  // Octave esJ.m). Each actor evaluates a RISK-ADJUSTED reward in place of R:
  //     S_ij = R_ij * (1 - alpha_i * (R_ij - mu_i)),
  // where mu_i = sum_j P_j R_ij is the actor's probability-weighted mean
  // reward under the CURRENT option probabilities, and alpha_i is a
  // CONSTANT per actor, set from the spread of its potential rewards
  // (revised 2026-07-06, replacing the Octave's state-dependent
  // alpha_i = a / stdev_i(P) after that feedback produced threshold
  // behavior in the parametric runs):
  //     halfSpread_i = (max_j R_ij - min_j R_ij) / 2,
  //     alpha_i      = a * (ln 2 / halfSpread_i).
  // Interpretation of the dimensionless knob 'a': at a = 1, losing half the
  // spread hurts twice as much as gaining half the spread helps (the
  // exponential-utility calibration u(x) ~ -exp(-alpha x), where
  // |u(-h)-u(0)| / |u(h)-u(0)| = e^{alpha h} = 2 at alpha h = ln 2).
  // a = 0 is risk-neutral, a > 0 risk-averse, a < 0 risk-seeking. Guard: a
  // constant reward row (halfSpread_i = 0 -- impossible for generated
  // instances, whose rows carry both signs) has nothing for risk to price,
  // and alpha_i is taken as 0.
  //
  // At a = 0 the adjustment is skipped outright, so the risk-neutral model
  // is ARITHMETICALLY IDENTICAL to the unparameterized one -- the a = 0
  // acceptance criterion is exact equality, not a tolerance match.
  // ---------------------------------------------------------------------------

  // Per-actor variance of the ORIGINAL reward R under the option
  // probabilities implied by the effort matrix e (M x N):
  //     var_i = sum_j P_j R_ij^2 - (sum_j P_j R_ij)^2.
  // The risk-aversion acceptance criterion evaluates this at competing
  // equilibria: for a > 0 every actor's variance must be no more than at
  // the a = 0 equilibrium of the same problem. Throws std::invalid_argument
  // on a size mismatch between R and e.
  VectorXd saoePayoffVariance(const MatrixXd& R, const MatrixXd& e, double eps);

  // Controls for the SAOE solve (forwarded to the Josephy-Newton outer loop and
  // the inner LVI solver).
  struct SaoeParams {
    double      outerTol     = 1.0e-10;          // squared natural-residual stop (solveVI)
    int         outerIterMax = 250;              // outer cap (a contest may need > the default)
    double      innerMagTol  = 1.0e-14;          // inner LVI squared-residual tolerance
    int         innerIterMax = 1000;             // inner iteration cap
    InnerMethod innerMethod  = InnerMethod::Han; // dHan06 (default) or bsHe94b

    // Diagnostic probe forwarded to solveVI: print the smallest eigenvalue of the
    // symmetric part of each outer Jacobian, to check whether the inner problem is
    // monotone (a prerequisite for dHan06's convergence). Off by default.
    bool        logInnerDefiniteness = false;
  };

  // The SAOE solution decoded from a solver VIResult, whose packed z is
  // [ e (row-major, M x N), lambda (M) ]. saoe() returns the raw VIResult (uniform
  // with every other solver); call saoeDecode to recover efforts and multipliers.
  struct SaoeSolution {
    MatrixXd e;        // M x N equilibrium efforts
    VectorXd lambda;   // M multipliers (marginal value of strength)
  };

  // Decode a SAOE VIResult (z = [e row-major, lambda]) into efforts and multipliers.
  SaoeSolution saoeDecode(const VIResult& r, Index M, Index N);

  // The model's small effort constant, eps = RMS(R)/10000 = ||R||_F / sqrt(M N) / 1e4.
  double saoeEps(const MatrixXd& R);

  // Option probabilities P_j from an effort matrix e (M x N): E_j = sum_i e_{ij} + eps,
  // P_j = E_j / sum_k E_k. Returns a length-N vector.
  VectorXd saoeProbabilities(const MatrixXd& e, double eps);

  // Per-actor expected rewards u_i = sum_j R_{ij} P_j. Returns a length-M vector.
  VectorXd saoeUtilities(const MatrixXd& R, const MatrixXd& e, double eps);

  // A random starting point y0 = [e (row-major), lambda], all non-negative: efforts
  // e_{ij} ~ U[0, S_i/N] and lambda_i ~ U[0, 1]. Use it to explore which of the
  // (possibly many) equilibria the solver reaches from different basins.
  VectorXd saoeRandomStart(const MatrixXd& R, const VectorXd& S, std::mt19937_64& rng);

  // Build the SAOE NCP as a VIModel (zero free block; m = N*M + M) WITHOUT
  // solving it -- for callers that drive their own engine or composition on
  // the same problem (e.g. the alternating chain in the equilibrium-selection
  // tests). Identical packing and G map to what saoe() uses internally.
  // riskAversion is the fractional risk aversion 'a' above (default 0 =
  // risk-neutral, the historical model, skipped-adjustment identical).
  // epsilon overrides the model's strength floor: <= 0 (the default) means
  // saoeEps(R) = RMS(R)/1e4; a positive value is used as given. The override
  // exists for the eps-regime experiments: the 2022 pmedemo runs used the
  // much larger eps = RMS(weights)/1e3 ~ 0.1 on the reference instance, and
  // the smoothing appears to govern whether INTERIOR (split-effort)
  // equilibria exist beside the all-in vertex ones.
  // Throws std::invalid_argument on an empty R or S.size() != R.rows().
  VIModel saoeModel(const MatrixXd& R, const VectorXd& S,
                    double riskAversion = 0.0, double epsilon = -1.0);

  // The deterministic start saoe() uses when given no z0: e_{ij} = S_i/(N+1)
  // (spends a little under budget), lambda = 0.
  VectorXd saoeDefaultStart(const MatrixXd& R, const VectorXd& S);

  // Solve the SAOE Nash equilibrium for reward matrix R (M x N) and strengths S (M).
  // Builds the NCP (0 free components, N*M + M non-negative) and solves with solveVI.
  // The optional z0 is the starting point (layout as saoeRandomStart); if empty, the
  // default start e_{ij} = S_i/(N+1), lambda = 0 is used.
  // Throws std::invalid_argument on an empty R, S.size() != R.rows(), or a z0 whose
  // length is neither 0 nor N*M + M. Returns the raw VIResult; decode it with saoeDecode.
  VIResult saoe(const MatrixXd& R, const VectorXd& S,
                const SaoeParams& params = SaoeParams{},
                const VectorXd& startGuess = VectorXd());

} // namespace VINCP

#endif // VINCP_SAOE_HPP
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------

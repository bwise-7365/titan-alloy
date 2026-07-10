// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
// PFORM: parliament-formation sub-model over the SAOE class. A spatial model of
// politics builds the reward (utility) matrix; SAOE then estimates which parties
// support which parliament.
// ----------------------------------------------
#ifndef VINCP_APPS_PFORMPROBLEM_HPP
#define VINCP_APPS_PFORMPROBLEM_HPP

// PFORM (parliament formation) is a sub-model followed by SAOE. There are M
// parties with positive weights w_m, and D political issues; each party has a
// preferred position P_dm in [0, 1] on issue d and a non-negative salience
// S_dm (with sum_d S_dm >= 1). A "parliament" assigns each issue to exactly one
// controlling party, so there are K = M^D parliaments; parliament k has the
// composite position X_dk = P_{d, f_d(k)}, where f_d(k) is the party controlling
// issue d. The (risk-averse, quadratic) utility of parliament k to party m is
//     U_km = 1 - (sum_d S_dm (P_dm - X_dk)^2) / (sum_d S_dm),
// and the risk-neutral value is the same with |.| in place of (.)^2 (V_km).
//
// The M x K matrix R(m, k) = U_km is the SAOE reward matrix and w is the SAOE
// strength vector; solving the SAOE Nash equilibrium estimates the parties'
// support (effort) for each parliament and each parliament's probability. The
// deterministic Central-Position guess is the parliament that sorts first in
// the descending lexicographic order of (eta_k, phi_k, k), where
//     eta_k = sum_m w_m U_km,   phi_k = sum_m w_m V_km.
//
// SAOE's effort floor eps is NOT an input; instead the caller sets the
// UNSELECTED PROBABILITY q in (0, (K-1)/K): with total weight W the probability
// of any parliament carrying no effort is eps/(K eps + W), so the K-1 unsupported
// parliaments together carry q = (K-1) eps / (K eps + W). Inverting,
//     eps = q W / (K (1 - q) - 1),
// which is positive on the admissible range and diverges as q -> (K-1)/K.

#include "problem.hpp"
#include "saoeproblem.hpp"

#include <cstdint>
#include <vector>

namespace VINCP::App {

  using std::vector;

  // The parliament instance: weights, preferred positions, and saliences.
  // position and salience are D issues (rows) x M parties (columns); weight is M.
  struct PformData {
    VectorXd weight;     // w_m > 0                 (M)
    MatrixXd position;   // P_dm in [0, 1]          (D x M)
    MatrixXd salience;   // S_dm >= 0, colsum >= 1  (D x M)
  };

  // Controls. unselectedProb is q above; engine selects the SAOE solver (SAOE
  // honors Chain (default) or Auto). Risk aversion is already baked into the
  // quadratic utility, so the inner SAOE runs risk-neutral.
  struct PformParams {
    double unselectedProb = 0.05;   // q; must satisfy 0 < q < (K-1)/K
    ProblemBase::Engine engine = ProblemBase::Engine::Default;   // -> SAOE Chain
    bool verbose = false;
  };

  // The PFORM answer. effort/probabilities/utilities come from the SAOE solve;
  // eta/phi and the deterministic guess come from the sub-model directly.
  struct PformResult {
    MatrixXd effort;         // M x K party effort on each parliament (SAOE e)
    VectorXd probabilities;  // K parliament probabilities (pinned at equilibrium)
    VectorXd utilities;      // M party expected utilities (from SAOE)
    VectorXd eta;            // K: sum_m w_m U_km
    VectorXd phi;            // K: sum_m w_m V_km
    Index    deterministic = 0;   // k of the parliament first in the (eta,phi,k) sort
    double   epsilon = 0.0;       // the effort floor eps derived from q (reported)
  };

  // Random-instance recipe: positions ~ U[0,1]; weights ~ U[weightLo, weightHi].
  // Saliences are SPARSE: each party has positive salience ~ U[salienceLo,
  // salienceHi] on half its issues (chosen at random) and ZERO on the rest, so
  // parties hold complementary interests and can profit from ceding the issues
  // they do not care about. The >= 1 total-salience constraint holds since each
  // positive draw is at least salienceLo (>= 1) and every party keeps at least
  // one positive issue; and every issue is salient to at least one party (no
  // all-zero row, which would make that issue irrelevant). Fixed seed =
  // reproducible.
  struct PformRandomSpec {
    int           numParties = 3;
    int           numIssues  = 4;
    std::uint64_t seed       = 0;
    double        weightLo   = 1.0;
    double        weightHi   = 10.0;
    double        salienceLo = 5.0;    // positive-salience draw range U[lo, hi]
    double        salienceHi = 15.0;
  };

  // Number of parliaments K = M^D. Throws std::invalid_argument on M < 2 or
  // D < 1, and std::runtime_error if K exceeds a sanity cap (the SAOE problem
  // has K*M + M variables, so very large K is intractable).
  Index pformParliamentCount(Index numParties, Index numIssues);

  // The matching for parliament k: a length-D vector whose entry d is the party
  // controlling issue d, f_d = (k / M^d) mod M (mixed-radix, issue 0 least
  // significant). Throws std::invalid_argument on an out-of-range k.
  vector<Index> pformMatching(Index k, Index numParties, Index numIssues);

  // The single validator for a PformData, applied to EVERY instance (random,
  // GMS, or hand-built) -- solve calls it, and the generator validates its own
  // output. Enforces: M >= 2 parties, D >= 1 issues, position and salience are
  // D x M matching weight's M, weights > 0, positions in [0, 1], saliences >= 0,
  // each PARTY's total salience (column sum, sum_d S_dm) >= 1 (no actor with no
  // interest in anything), and each ISSUE's total salience (row sum) > 0 (no
  // all-zero row -- no issue of interest to nobody). Throws std::invalid_argument
  // on the first violation.
  void validatePformData(const PformData& data);

  class PForm : public Problem<PformParams, PformResult> {
  public:
    explicit PForm(PformData data);

    static PformData generate(const PformRandomSpec& spec);

    // Build the reward matrix, derive eps from q, run SAOE, and package the
    // supports/probabilities with the deterministic Central-Position guess.
    // Throws std::invalid_argument on a malformed instance, q outside
    // (0, (K-1)/K), or an engine SAOE does not honor.
    Solution solve(const Params& params) const override;

    // The Problem-framework hook. Like SAOE, PForm's effort attribution is the
    // non-unique part, so this is NON-trivial (it would sparsify the effort
    // matrix exactly as SAOE does, leaving the parliament probabilities fixed).
    // Not yet implemented; throws std::logic_error rather than return a spread
    // result.
    PformResult sparsify(const PformResult& result) const override;

    Index numParties() const;
    Index numIssues() const;

    const PformData& instance() const { return data; }

  protected:
  private:
    PformData data;
  };

} // namespace VINCP::App

#endif // VINCP_APPS_PFORMPROBLEM_HPP
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------

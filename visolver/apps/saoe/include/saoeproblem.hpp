// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
// SAOE problem class: a Problem-template wrapper over the library's Strategic
// Allocation Of Effort Nash-equilibrium solver, for use in focused apps.
// ----------------------------------------------
#ifndef VIMCP_APPS_SAOEPROBLEM_HPP
#define VIMCP_APPS_SAOEPROBLEM_HPP

// SAOE packages the existing SAOE machinery (include/saoe.hpp plus the solver
// engines) behind the app-framework contract: build an SAOE from its DATA (the
// reward matrix R and strength vector S), call solve(params), and receive a
// tuple (VIResult, SaoeResult). It reuses the library entry points unchanged;
// nothing under include/ or lib/ is touched.
//
// ANSWER CONTRACT (answerContractNote): at the SAOE equilibrium the per-option
// AGGREGATES -- the option probabilities and hence each actor's expected reward
// -- are pinned, but the per-actor EFFORT SPLIT that realizes them is NOT
// unique. SaoeResult therefore leads with the pinned quantities; its effort
// matrix is one representative of the solution set.

#include "problem.hpp"

#include <cstdint>

namespace VIMCP::App {

  // The SAOE problem instance: M actors (rows of R) x N options (columns).
  //   R(i, j) = reward to actor i from option j; S(i) = actor i's strength.
  struct SaoeData {
    MatrixXd R;   // M x N rewards
    VectorXd S;   // M strengths (effort budgets)
  };

  // How to solve, plus the two modeling knobs. Every solve-process default is
  // the configuration the saoe_chain_test proves reaches equilibrium E; override
  // only to trade robustness for speed deliberately.
  // SAOE honors these engines (any other throws): Default / Chain (the robust
  // alternating globalizer/finisher chain), Auto (the chooseEngine dispatcher),
  // SmoothingNewton (non-interior smoothing, Zhang-Liu-Liu), and Fbs
  // (forward-backward splitting). The last two are matrix-free, non-interior-path
  // alternatives -- useful for checking whether a non-central-path solver
  // concentrates the (non-unique) effort attribution rather than spreading it
  // over the optimal face, as the chain's interior-point globalizer does.
  struct SaoeParams {
    ProblemBase::Engine engine = ProblemBase::Engine::Default;  // Default => Chain

    // Modeling knobs (these change the equilibrium, not merely the path to it).
    double riskAversion = 0.0;    // 'a' in saoeModel: 0 = risk-neutral
    double epsWeight      = -1.0;   // effort floor; <= 0 => saoeEps(R)

    // Solve-process controls (all tolerances are SQUARED natural residuals).
    double magTol               = 1.0e-10;   // chain / auto acceptance
    int    ssnIterMax           = 300;       // semismooth finisher cap
    int    ssnNonmonotoneMemory = 4;         // finisher nonmonotone memory (SEMI)
    int    jnOuterIterMax       = 50;        // globalizer Josephy-Newton outer cap
    int    jnStallIterMax       = 5;         //   and its no-progress cutoff
    double ipmInnerMagTol       = 1.0e-12;   //   inner interior-point tolerance
    int    ipmInnerIterMax      = 200;       //   inner interior-point cap (LU count)
    int    chainRoundsMax       = 8;         // alternating-chain round cap
    double chainPerturbScale    = 0.1;       // perturb-restart on stagnation

    bool   verbose              = false;     // print per-stage chain progress
  };

  // The decoded SAOE answer. The pinned aggregates come first; the effort matrix
  // is ONE member of a non-unique split (see answerContractNote). Solver
  // telemetry (converged / residual / iterations) is the VIResult half of the
  // solve tuple, so it is not duplicated here.
  struct SaoeResult {
    VectorXd probabilities;   // N option probabilities P_j (pinned at E)
    VectorXd utilities;       // M per-actor expected rewards u_i (pinned at E)
    MatrixXd e;               // M x N efforts (one member of the solution set)
    VectorXd lambda;          // M multipliers (marginal value of strength)
  };

  // A random-instance recipe (the "internally generated data" source, beside a
  // GMS file). Rewards ~ U[rewardLo, rewardHi], strengths ~ U[strengthLo,
  // strengthHi] (optionally rounded to tenths, as the saoe_demo does); a fixed
  // seed makes runs reproducible.
  struct SaoeRandomSpec {
    int           numActors  = 5;
    int           numOptions = 7;
    std::uint64_t seed       = 0;
    double        rewardLo   = -100.0;
    double        rewardHi   =  200.0;
    double        strengthLo =   10.0;
    double        strengthHi =   30.0;
    bool          roundStrengthTenthsP = true;
  };

  // Drive a non-negative effort matrix (parties x options) to a VERTEX of the
  // transportation polytope with the SAME row and column sums, by cycle-
  // cancelling its bipartite support down to a forest. IDENTITY if the input is
  // already a vertex (acyclic support); otherwise it projects to one, leaving
  // every row and column sum (hence the SAOE probabilities / utilities) fixed.
  // Shared by SAOE::sparsify and PForm::sparsify. Throws std::logic_error if it
  // cannot reach a vertex (a non-corner input left non-corner is an error).
  MatrixXd sparsifyEffortMatrix(const MatrixXd& effort);

  // The SAOE problem: constructed from its data, solved under its params.
  class SAOE : public Problem<SaoeParams, SaoeResult> {
  public:
    // Build the problem from its instance data. Validation of R/S is deferred
    // to solve (via saoeModel), so an SAOE can be constructed cheaply.
    explicit SAOE(SaoeData data);

    // Generate a random instance. Throws std::invalid_argument on a non-positive
    // dimension or an inverted range.
    static SaoeData generate(const SaoeRandomSpec& spec);

    // The engines SAOE honors, in preference order (the first is its default;
    // Engine::Default resolves to it). THE single source of truth: solve's guard
    // and any CLI derive their accepted set, help text, and error messages from
    // this list. Any other engine throws from solve.
    static const std::vector<ProblemBase::Engine>& honoredEngines();

    // Solve the Nash equilibrium: build the NCP via saoeModel (risk aversion and
    // epsilon from params), start from the deterministic analytic-center point,
    // run the selected engine (Default/Chain = the robust alternating chain;
    // Auto = the chooseEngine dispatcher; SmoothingNewton / Fbs = the
    // non-interior-path alternatives), and decode. Throws std::invalid_argument
    // on an empty R, S.size() != R.rows() (via saoeModel), or an engine SAOE
    // does not honor.
    Solution solve(const Params& params) const override;

    // The Problem-framework hook. SAOE's effort attribution is NON-trivial to
    // sparsify (it is the non-unique part -- the effort matrix is a point of a
    // transportation polytope whose margins are the pinned aggregates), so this
    // must NOT be a pass-through. The real vertex-finding process is not yet
    // implemented; this currently throws std::logic_error rather than return a
    // spread result.
    SaoeResult sparsify(const SaoeResult& result) const override;

    // One-paragraph statement of what SaoeResult guarantees (the non-uniqueness
    // of the effort split), for an app to print in its output header. Single
    // source of truth so every SAOE app says the same thing.
    static const char* answerContractNote();

  protected:
  private:
    SaoeData data;
  };

} // namespace VIMCP::App

#endif // VIMCP_APPS_SAOEPROBLEM_HPP
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------

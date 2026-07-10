// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
// Fleet problem class: a Problem-template wrapper over the network library's
// distribution-planning QP and its greedy / swap / sparsify companions.
// ----------------------------------------------
#ifndef VINCP_APPS_FLEETPROBLEM_HPP
#define VINCP_APPS_FLEETPROBLEM_HPP

// Fleet packages the existing fleet machinery (network/, namespace
// VINCP::Network) behind the app-framework contract: build a Fleet from its
// DATA (a FleetInstance), call solve(params) for the large conservative QP, and
// receive a tuple (VIResult, FleetResult). The greedy planner, the 2-exchange
// swap, and the sparsify (purify) pass -- which are NOT part of the Problem
// template -- are exposed as extra methods that delegate to the same library
// functions the fleet_viewer uses. Nothing under network/ is modified; this
// class only links vincpnet and calls its public API.

#include "problem.hpp"

#include "fleetgreedy.hpp"
#include "fleetinstance.hpp"
#include "fleetplan.hpp"
#include "fleetsolve.hpp"
#include "fleetswap.hpp"

#include <cstdint>
#include <string>

namespace VINCP::App {

  // How to solve the fleet QP. Defaults mirror FleetSolveParams (the proven
  // production configuration: interior point with the structured "fleet" Newton
  // factory, keep-all screening). Fleet HONORS exactly Engine::Ipm (its
  // default), Engine::Bshe94b, and Engine::Ssn; ANY OTHER engine (Chain, Auto,
  // SmoothingNewton, Fbs, ...) throws std::invalid_argument from solve.
  struct FleetParams {
    ProblemBase::Engine engine = ProblemBase::Engine::Ipm;   // Default => Ipm
    std::string ipmNewton      = "fleet";                    // "fleet" | "dense"
    double magTol              = 1.0e-12;   // squared-norm convergence tolerance
    int    iterMax             = 500000;

    // Pre-solve per-asset screening (round-trip mileage order); 0 / 0.0 = keep
    // all. solveFleetPlan applies these internally; keep-all is exact under the
    // "fleet" factory.
    Index  maxSourcesPerSink    = 0;
    double gapFraction          = 0.0;
    int    maxCertificateRounds = 10;
    double certificateSlack     = 1.0e-6;

    double epsilon              = -1.0;   // scaled-instance tie-break; <0 = auto
  };

  // The decoded fleet answer. It mirrors the fields the fleet GUI reads from a
  // FleetSolveResult, so a viewer can store a FleetResult in place of a
  // FleetSolveResult with near-identical accessors. Raw solver telemetry beyond
  // 'converged' is the VIResult half of the solve tuple.
  struct FleetResult {
    Network::FleetPlan plan;            // real units, checkFleetPlan-clean
    double   shortfall = 0.0;           // theta at the plan
    VectorXd milesUsed;                 // per vehicle type, real vehicle-miles
    VectorXd budgetShadowPrice;         // lambda_k >= 0 per vehicle type
    bool     certifiedP = false;        // solved AND nothing screened stays attractive
    bool     converged  = false;        // = vi.converged
    Index    keptPairs = 0;             // y variables in the final LCP
    Index    totalPairs = 0;            // keep-all (pair, capable type) count
    int      certificateRounds = 0;
  };

  class Fleet : public Problem<FleetParams, FleetResult> {
  public:
    // Build the problem from its instance data. Validation is deferred to the
    // library calls (validateFleetInstance inside solveFleetPlan etc.).
    explicit Fleet(Network::FleetInstance instance);

    // Data source: a random instance from a generation profile and seed (wraps
    // makeRandomFleetInstance). Throws std::invalid_argument on a bad profile.
    static Network::FleetInstance generate(const Network::FleetProfile& profile,
                                           std::uint64_t seed);

    // The engines Fleet honors, in preference order (the first is its default;
    // Engine::Default resolves to it). Single source of truth for solve's guard.
    static const std::vector<ProblemBase::Engine>& honoredEngines();

    // The large conservative QP (the ProblemBase contract): builds
    // FleetSolveParams from params, calls solveFleetPlan, and decodes. Throws
    // std::invalid_argument on a bad instance (via solveFleetPlan) or an engine
    // Fleet does not honor.
    Solution solve(const Params& params) const override;

    // Extra methods (NOT in the Problem template) -- thin delegates to the
    // vincpnet functions the fleet_viewer uses. greedyPlan is the fast
    // heuristic; swapToLocalOptimum drives each asset's flows to a 2-exchange
    // local optimum in place; sparsify (purifyFleetPlan) consolidates arcs in
    // place without changing deliveries.
    Network::FleetGreedyResult greedyPlan(
        const Network::FleetGreedyParams& params = {}) const;
    Network::FleetSwapSummary swapToLocalOptimum(
        Network::FleetPlan& plan, int maxSwapsPerAsset = 100000) const;

    // The Problem-framework hook: sparsify a whole FleetResult. Non-trivial for
    // Fleet -- the interior-point QP spreads flow over the optimal face. It wraps
    // the in-place plan-level sparsify below: deliveries (hence shortfall) are
    // invariant, vehicle-miles drop, and the solve metadata is carried over.
    FleetResult sparsify(const FleetResult& result) const override;

    // The in-place, plan-level purification the fleet GUI uses (it needs the
    // FleetPurifySummary for its report). Kept unchanged so fleet_app behavior is
    // byte-identical; the result-level hook above delegates to it.
    Network::FleetPurifySummary sparsify(
        Network::FleetPlan& plan, int maxSwapsPerAsset = 100000) const;

    const Network::FleetInstance& instance() const { return data; }

  protected:
  private:
    Network::FleetInstance data;
  };

} // namespace VINCP::App

#endif // VINCP_APPS_FLEETPROBLEM_HPP
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------

// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
// Production fleet optimizer: nondimensionalize, reduce + screen per asset,
// solve the fleet KKT-LCP, certify, unpack (fleet-formulation.md section 9).
// ----------------------------------------------
#ifndef VIMCP_NETWORK_FLEETSOLVE_HPP
#define VIMCP_NETWORK_FLEETSOLVE_HPP

#include "fleetlcp.hpp"

namespace VIMCP::Network {

  // Observability hooks for solveFleetPlan (2026-07-08 performance plan,
  // stage FP0). Both are optional; empty means silent, and the solve is
  // identical either way. roundStartLogger fires after a certificate round's
  // LCP assembly and BEFORE its engine solve, so an aborted run still
  // reports the dimension it was attempting; roundEndLogger fires after the
  // solve with the engine result and the round's solve wall time.
  using FleetRoundStartLogger =
      function<void(int round, Index keptPairs, Index dim)>;
  using FleetRoundEndLogger =
      function<void(int round, const VIResult& vi, double milliseconds)>;

  // Mirrors FlowPlanParams where the fleet pipeline has the same knob. The
  // "chain" engine is not offered (a warm-start refinement never needed
  // here yet).
  struct FleetSolveParams {
    string engine = "ipm";          // "ipm" | "bshe94b" | "ssn"
    double magTol = 1.0e-12;        // squared-norm convergence tolerance
    int iterMax = 500000;
    int iterFreq = 0;

    // Newton linear algebra inside the "ipm" engine: "fleet" =
    // makeFleetNewtonFactory (fleetnewton.hpp, stage FN2 / ledger G5h):
    // per-cell Sherman-Morrison + dual Schur complement, O(numVars)-ish per
    // iteration instead of dim^3. "dense" = the generic dense-LU factory,
    // kept for cross-checks. Default "fleet" since FN3 (2026-07-08,
    // performance.md P10-P13).
    string ipmNewton = "fleet";
    // Dev-mode drift check (mehrotraipm.hpp): positive => the engine
    // verifies every structured Newton solve against its own M and throws
    // on violation; 0 = off.
    double newtonCheckTol = 0.0;

    // Observability (empty = silent): logger is handed to the inner engine
    // and produces heartbeats at iterFreq; the round hooks bracket each
    // certificate round's solve.
    IterationLogger logger;
    FleetRoundStartLogger roundStartLogger;
    FleetRoundEndLogger roundEndLogger;

    // Per-asset screen (round-trip mileage order); 0 / 0.0 = keep all.
    // Default keep-all: under the "fleet" factory the full problem is exact
    // by construction (nothing excluded, no certificate loop) and solves in
    // seconds (FN3, 2026-07-08: 70-node banded 4x3, dim 14,579, 39
    // iterations, 6.1 s, certified, objective better than any screened
    // run). Set a positive count when using the dense factory or a
    // projection engine, where the dimension matters.
    Index maxSourcesPerSink = 0;
    double gapFraction = 0.0;

    int maxCertificateRounds = 10;
    double certificateSlack = 1.0e-6;

    // Tie-break; negative = defaultFleetTieBreakEpsilon on the SCALED
    // instance.
    double epsilon = -1.0;
  };

  struct FleetSolveResult {
    FleetPlan plan;                 // real units, checkFleetPlan-clean
    double shortfall = 0.0;         // theta at the plan (vs original demand)
    VectorXd milesUsed;             // per type, real vehicle-miles
    VectorXd budgetShadowPrice;     // lambda_k >= 0, real units: shortfall
                                    // reduction per extra vehicle-mile of k
    VIResult vi;                    // final inner solve (scaled, squared res)
    Index keptPairs = 0;            // y variables in the final LCP
    Index totalPairs = 0;           // keep-all (pair, capable type) count
    int certificateRounds = 0;
    bool certifiedP = false;        // solved AND nothing screened out
                                    // remains attractive (R3 analog)
  };

  // Solve the reduced conservative fleet QP to certified optimality.
  // Throws std::invalid_argument on bad inputs (validateFleetInstance,
  // parameter checks, unservable assets per makeFleetReducedProblem).
  FleetSolveResult solveFleetPlan(const FleetInstance& inst,
                                  const FleetSolveParams& params = {});

} // namespace VIMCP::Network

#endif // VIMCP_NETWORK_FLEETSOLVE_HPP
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------

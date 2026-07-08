// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
// Production fleet optimizer: nondimensionalize, reduce + screen per asset,
// solve the fleet KKT-LCP, certify, unpack (fleet-formulation.md section 9).
// ----------------------------------------------
#ifndef VINCP_NETWORK_FLEETSOLVE_HPP
#define VINCP_NETWORK_FLEETSOLVE_HPP

#include "fleetlcp.hpp"

namespace VINCP::Network {

  // Mirrors FlowPlanParams where the fleet pipeline has the same knob. The
  // "chain" engine and the structured "flow" Newton factory are not offered:
  // the first is a warm-start refinement never needed here yet, the second
  // is single-commodity algebra (its fleet analog is ledger G5h).
  struct FleetSolveParams {
    string engine = "ipm";          // "ipm" | "bshe94b" | "ssn"
    double magTol = 1.0e-12;        // squared-norm convergence tolerance
    int iterMax = 500000;
    int iterFreq = 0;

    // Per-asset screen (round-trip mileage order); 0 / 0.0 = keep all.
    // Default 6: the 70-node two-asset profile lands near LCP dimension
    // 1,000, which the dense engines handle in seconds.
    Index maxSourcesPerSink = 6;
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

} // namespace VINCP::Network

#endif // VINCP_NETWORK_FLEETSOLVE_HPP
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------

// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
// The production flow-planning solver: nondimensionalize -> reduce ->
// bsHe94b -> R3 certificate loop -> unpack. One call from a calibrated
// Instance to an optimal Plan (doc/reduction.md).
// ----------------------------------------------
#ifndef VINCP_NETWORK_FLOWPLAN_HPP
#define VINCP_NETWORK_FLOWPLAN_HPP

#include "flowlcp.hpp"

#include <string>

using std::string;

namespace VINCP::Network {

  struct FlowPlanParams {
    // Which inner engine solves the affine complementarity system (both in
    // the NONDIMENSIONALIZED units):
    //   "bshe94b"  He's factor-once projection-contraction (the default)
    //   "chain"    chainedSolodovHe: Solodov-Svaiter to (roughMagTol,
    //              roughIterMax), then bsHe94b warm-started from its iterate
    //              (task E3a; built for cold-start-hostile cases like the
    //              200-node banded instances)
    //   "ipm"      mehrotraIpm: Mehrotra predictor-corrector interior point
    //              on the same complementarity system. Iteration counts are
    //              ~10-40 INDEPENDENT of the degenerate near-tied faces that
    //              stall the projection engines, but every iteration factors
    //              the Newton matrix -- under this engine iterMax counts LU
    //              factorizations, so set it in the low hundreds, not the
    //              projection engines' hundreds of thousands.
    //   "ssn"      semismoothNewtonSolve on the same system wrapped as a
    //              pure-NCP VIModel with its exact (constant) Jacobian: the
    //              semismooth engine's large-affine exercise (gate MC3).
    //              Like "ipm", every iteration is one factorization, so
    //              iterMax counts LU factorizations -- low hundreds.
    string engine = "bshe94b";
    double roughMagTol = 1.0e-4;   // chain phase-1 squared-residual target
    int roughIterMax = 20000;      // chain phase-1 iteration cap

    // Newton linear algebra for the "ipm" engine (ignored by the others):
    //   "dense"  one dense LU of the Newton matrix per iteration (default)
    //   "flow"   makeFlowNewtonFactory (flownewton.hpp): per-sink
    //            Sherman-Morrison + dual Schur complement, O(pairs) plus an
    //            LLT of ~(numSources + 1)^2 per iteration instead of dense
    //            dim^3 -- the NS2 structured factory, built to make the
    //            keep-all / no-screen solve feasible.
    string ipmNewton = "dense";
    double newtonCheckTol = 0.0;   // > 0: the engine verifies every Newton
                                   //   solve against M to this SQUARED
                                   //   residual and throws on drift (dev-mode
                                   //   guard for the structured factory)

    double magTol = 1.0e-12;   // squared-residual tolerance for the tight
                               // solve, in the NONDIMENSIONALIZED units
    int iterMax = 500000;
    int iterFreq = 0;          // <= 0: no iteration logging

    // Pair screen (Proposition R3; see ScreenParams in reduction.hpp for the
    // rule semantics). Both zero solves over every source-sink pair; either
    // rule active starts smaller and lets the certificate loop pull in any
    // pair wrongly excluded, so the final answer is exact either way.
    Index maxSourcesPerSink = 0;   // count rule
    double gapFraction = 0.0;      // gap rule (near-tie geometries)
    int maxCertificateRounds = 10;
    double certificateSlack = 1.0e-6;   // dual-feasibility slack (scaled units)

    double epsilon = -1.0;     // R4 tie-break for the SCALED system; negative
                               // means defaultTieBreakEpsilon(scaled instance)
  };

  struct FlowPlanResult {
    Plan plan;                       // real tons
    double shortfall = 0.0;          // theta at the plan
    double tonMilesUsed = 0.0;       // real ton-miles moved
    double budgetShadowPrice = 0.0;  // lambda in REAL units: shortfall
                                     // reduction per extra ton-mile of budget
    VIResult vi;                     // final bsHe94b result (SCALED units;
                                     // residual is SQUARED)
    Index keptPairs = 0;             // t variables in the final solve
    Index totalPairs = 0;            // sources x sinks
    int certificateRounds = 0;       // R3 re-solves triggered
    bool certifiedP = false;         // true when the solve converged AND every
                                     // excluded pair passed the R3 check (or
                                     // nothing was excluded); trust the plan
                                     // as optimal only when true
  };

  // Solve the flow-planning problem for a calibrated instance
  // (tonMileLimit > 0; run greedyPlan first to calibrate). Internally works
  // on the nondimensionalized system (tons / largest demand, miles / largest
  // cost -- exact unit change, mandatory for projection-method convergence)
  // and returns everything in real units. Honest-return on solver
  // non-convergence (check certifiedP / vi.converged); throws
  // std::invalid_argument on bad inputs.
  FlowPlanResult solveFlowPlan(const Instance& inst,
                               const FlowPlanParams& params = FlowPlanParams{});

} // namespace VINCP::Network

#endif // VINCP_NETWORK_FLOWPLAN_HPP
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------

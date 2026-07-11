// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
// Reference oracle: the FULL (unreduced) flow-planning formulation solved as
// a mixed complementarity problem with dHan06. Independent of the production
// path (no shortest paths, no reduction, different inner solver), so
// agreement with it validates Lemma R1 end to end. Tiny instances only.
// ----------------------------------------------
#ifndef VIMCP_NETWORK_ORACLE_HPP
#define VIMCP_NETWORK_ORACLE_HPP

#include "plan.hpp"

namespace VIMCP::Network {

  // ---------------------------------------------------------------------------
  // The full-formulation KKT system
  // ---------------------------------------------------------------------------

  // KKT of the ORIGINAL formulation (formulation.md sections 2-4), variables
  // (f, S, R) with balance equalities, delivery rows (F1), caps, and the
  // budget. Mixed VIMCP layout z = (x, y):
  //   x = nu            balance multipliers, FREE, one per node
  //   y = [ f | S | R | delta | mu | lambda ]  all >= 0:
  //       f      arc flows, row-major (f_ab at fBase + a*numNodes + b)
  //       S      supplies, one per node
  //       R      resupplies, one per DEMAND node (demandNodes order; R = 0
  //              elsewhere by convention)
  //       delta  delivery multipliers, one per node
  //       mu     capacity multipliers, one per node
  //       lambda the scalar budget multiplier
  // Rows (writing Q_a = 2 P_a / D_a^2):
  //   H_nu(j)    = sum_i f_ij + S_j - R_j - sum_k f_jk               (= 0)
  //   G_f(a,b)   = nu_a - nu_b - delta_b + lambda c_ab
  //   G_S(a)     = mu_a - nu_a
  //   G_R(a)     = Q_a R_a - 2 P_a / D_a + delta_a + nu_a
  //   G_delta(j) = sum_i f_ij - R_j
  //   G_mu(i)    = C_i - S_i
  //   G_lambda   = L - sum_ab c_ab f_ab
  // The symmetric part of M is diag(0, ..., Q, ..., 0): monotone. There is
  // deliberately NO tie-break here -- the oracle pins theta and R*, not f.
  struct OracleKkt {
    MatrixXd M;
    VectorXd q;
    Index numNodes = 0;
    vector<Index> demandNodes;    // R-block order (node ids)
    Index fBase = 0, sBase = 0, rBase = 0;
    Index deltaBase = 0, muBase = 0, lambdaIndex = 0;
  };

  // Assemble the system. Throws std::invalid_argument on an uncalibrated
  // budget (tonMileLimit <= 0).
  OracleKkt buildOracleKkt(const Instance& inst);

  // Read a solver iterate back into a Plan (nonnegative blocks clamped at 0;
  // R filled as 0 at non-demand nodes). Throws on a size mismatch.
  Plan unpackOracle(const Instance& inst, const OracleKkt& kkt,
                    const VectorXd& z);

  // ---------------------------------------------------------------------------
  // The oracle solve
  // ---------------------------------------------------------------------------

  struct OracleParams {
    double magTol = 1.0e-12;   // squared-residual tolerance (dHan06), applied
                               // to the NONDIMENSIONALIZED system (see below)
    int iterMax = 300000;
    int iterFreq = 0;          // <= 0: no iteration logging
    Index maxNodes = 8;        // the full dense formulation is O(numNodes^2)
                               // variables; refuse anything bigger
  };

  struct OracleResult {
    Plan plan;                 // in REAL units (tons)
    double shortfall = 0.0;    // theta at the returned plan
    VIResult vi;               // raw solver result over the NONDIMENSIONAL
                               // system (z and residual in scaled units;
                               // residual is SQUARED); check vi.converged
                               // before trusting the plan
  };

  // Solve the full formulation with dHan06 over the mixed projector from the
  // all-zero start. Internally the system is NONDIMENSIONALIZED (tons in
  // units of the largest demand, miles in units of the largest cost) -- an
  // exact unit change under which the optimum is invariant, but which the
  // projection method needs to converge at a sane rate -- and the returned
  // plan is scaled back to real tons. Throws std::invalid_argument if the
  // instance exceeds maxNodes or the budget is uncalibrated; solver-level
  // failures follow the library convention (honest vi.converged, throws only
  // on NaN/divergence).
  OracleResult solveOracle(const Instance& inst,
                           const OracleParams& params = OracleParams{});

} // namespace VIMCP::Network

#endif // VIMCP_NETWORK_ORACLE_HPP
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------

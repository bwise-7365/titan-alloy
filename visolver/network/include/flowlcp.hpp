// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
// Matrix generator and unpacker: the reduced flow-planning QP's KKT system as
// a monotone affine complementarity problem (M, q), and the expansion of its
// solution back to a full network Plan (doc/reduction.md sections 2, 5, 7).
// ----------------------------------------------
#ifndef VIMCP_NETWORK_FLOWLCP_HPP
#define VIMCP_NETWORK_FLOWLCP_HPP

#include "plan.hpp"
#include "reduction.hpp"

namespace VIMCP::Network {

  // ---------------------------------------------------------------------------
  // The KKT complementarity system
  // ---------------------------------------------------------------------------

  // KKT of the reduced problem P_eps in the library's mixed-VIMCP convention
  // with an EMPTY free block: z = y = [ t | mu | lambda ], all >= 0, and
  //   0 <= G(z) = M z + q  _|_  z >= 0 ,
  // where t are the kept source->sink shipments (sink-major, following
  // reduced.kept), mu are the source capacity multipliers, and lambda is the
  // scalar ton-mile budget multiplier. Blocks (writing Q_n = 2 P_n / D_n^2,
  // d_p = shipCost of pair p, s(p)/n(p) its source/sink):
  //   G_t(p)  = Q_{n(p)} (sum_{r: n(r)=n(p)} t_r - D_{n(p)})
  //             + (lambda + eps) d_p + mu_{s(p)}
  //   G_mu(s) = C_s - sum_{p: s(p)=s} t_p
  //   G_la    = L - sum_p d_p t_p
  // M's symmetric part is block-diag(Q-blocks, 0, 0) >= 0 (the incidence and
  // budget blocks are skew), so the problem is MONOTONE and bsHe94b's
  // convergence theory applies (literature.md claims 2-3). eps > 0 is the R4
  // min-ton-mile tie-break, baked into q.
  struct FlowLcp {
    MatrixXd M;                    // (numPairs + numSources + 1) square
    VectorXd q;
    vector<Index> pairSourcePos;   // per t variable: index into reduced.sources
    vector<Index> pairSinkPos;     // per t variable: index into reduced.sinks
    VectorXd pairCost;             // d_p, aligned with the t block
    Index numPairs = 0;
    Index numSources = 0;          // capacity rows == reduced.sources.size()
    double epsilon = 0.0;          // tie-break actually used
  };

  // The R4 default tie-break: epsilon = tieBreakRelative * (sum of P over
  // demand nodes) / L, so the tie-break perturbs the optimal shortfall by at
  // most tieBreakRelative of full scale (Proposition R4).
  inline constexpr double tieBreakRelative = 1.0e-8;
  double defaultTieBreakEpsilon(const Instance& inst);

  // Assemble (M, q) for the instance's calibrated budget. Throws
  // std::invalid_argument if tonMileLimit is not positive (calibrate first:
  // greedyPlan), if epsilon is negative, or if the reduced problem is empty.
  FlowLcp buildFlowLcp(const Instance& inst, const ReducedProblem& reduced,
                       double epsilon);

  // ---------------------------------------------------------------------------
  // Unpacker
  // ---------------------------------------------------------------------------

  // Expand a solution z of the complementarity system into a full Plan:
  // each shipment t_p > 0 travels its canonical shortest route (routeNodes),
  // accumulating arc flows; S and R are the row/column totals per node. By
  // Lemma R1's constructive direction the result is feasible and uses exactly
  // sum_p d_p t_p ton-miles. Negative entries in the t block (never produced
  // by a projection solver, but tolerated) are clamped to 0. Throws
  // std::invalid_argument on a size mismatch or non-finite z.
  Plan unpackFlowLcp(const Instance& inst, const ShortestRoutes& routes,
                     const ReducedProblem& reduced, const FlowLcp& lcp,
                     const VectorXd& z);

} // namespace VIMCP::Network

#endif // VIMCP_NETWORK_FLOWLCP_HPP
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------

// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
// Structured Newton factory for the flow LCP: per-sink Sherman-Morrison +
// dual Schur complement, replacing the interior-point engine's dense LU.
// ----------------------------------------------
#ifndef VIMCP_NETWORK_FLOWNEWTON_HPP
#define VIMCP_NETWORK_FLOWNEWTON_HPP

#include "flowlcp.hpp"
#include "mehrotraipm.hpp"

namespace VIMCP::Network {

  // Build a NewtonSolverFactory (mehrotraipm.hpp seam) that solves the flow
  // LCP's per-iteration Newton system
  //     K = M + diag(sOverY)
  // by structure instead of a dense LU (gate NS2; every formula below is
  // machine-verified by network/doc/ns2-newton-check.mac, all 10 checks).
  // With z = [ t | mu | lambda ] (pure NCP, numFree = 0) and the
  // complementarity diagonal split (Dt | Dmu | Dla):
  //
  //   1. The t-block Qblk + diag(Dt) is block-diagonal BY SINK, each block a
  //      RANK-ONE Q_n * ones plus a positive diagonal: inverted per sink in
  //      O(k_n) by Sherman-Morrison,
  //          W|_n = diag(1/Dt) - (Q_n / (1 + Q_n sigma_n)) u_n u_n^T,
  //          u_n = 1/Dt on the slice, sigma_n = sum of 1/Dt over the slice.
  //   2. The borders are thin (one capacity column per source + the budget
  //      column) and SKEW, so eliminating dt cancels the signs and leaves an
  //      SPD dual Schur complement of size (numSources + 1),
  //          S = diag(Dmu | Dla) + B^T W B,   B = [E  d],
  //      assembled in O(pairs + sum k_n^2) and factored by LLT.
  //
  // Per-iteration cost is O(pairs * k_avg) + one tiny LLT versus the dense
  // LU's O(dim^3) -- the lever that makes the keep-all / no-screen solve
  // feasible (NS3). The engine itself still holds the dense M for residual
  // matvecs; a matrix-free M is out of scope here.
  //
  // The factory is bound to THIS lcp's structure and data (sink-major slice
  // layout, source positions, costs, Q_n from M's diagonal); rebuild it
  // whenever the lcp is rebuilt (e.g. each R3 certificate round). Because
  // the flow LCP has no free block, the factory rejects a nonzero
  // freeRegularization. During development, mehrotraIpm's newtonCheckTol
  // verifies every solve this factory returns against the engine's own M.
  //
  // Throws std::invalid_argument on an lcp whose fields are inconsistent
  // (empty, sizes disagreeing, pairs not sink-major-contiguous, or a
  // non-positive Q diagonal); the returned factory throws
  // std::invalid_argument on a nonzero freeRegularization, a wrong-sized or
  // non-positive sOverY, and std::runtime_error if the Schur complement
  // fails its LLT (numerically not positive definite).
  NewtonSolverFactory makeFlowNewtonFactory(const FlowLcp& lcp);

} // namespace VIMCP::Network

#endif // VIMCP_NETWORK_FLOWNEWTON_HPP
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------

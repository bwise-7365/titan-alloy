// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
// Structured Newton factory for the fleet LCP: per-cell Sherman-Morrison +
// dual Schur complement, replacing the interior-point engine's dense LU.
// ----------------------------------------------
#ifndef VIMCP_NETWORK_FLEETNEWTON_HPP
#define VIMCP_NETWORK_FLEETNEWTON_HPP

#include "fleetlcp.hpp"
#include "mehrotraipm.hpp"

namespace VIMCP::Network {

  // Build a NewtonSolverFactory (mehrotraipm.hpp extension point) that
  // solves the fleet LCP's per-iteration Newton system
  //     K = M + diag(sOverY)
  // by structure instead of a dense LU (stage FN2 of the 2026-07-08 fleet
  // performance plan, ledger G5h; every formula below is machine-verified
  // by network/doc/fleet-newton-check.mac, all 9 checks). With
  // z = [ y | mu | lambda ] (pure NCP, numFree = 0) and the complementarity
  // diagonal split (Dy | Dmu | Dla):
  //
  //   1. The y-block Qblk + diag(Dy) is block-diagonal BY DEMAND CELL
  //      (fleetlcp.hpp packs y cell-major), each block a RANK-ONE
  //      Q_cell * ones plus a positive diagonal: inverted per cell in
  //      O(k_cell) by Sherman-Morrison,
  //          W|_cell = diag(1/Dy) - (Q_c / (1 + Q_c sigma_c)) u_c u_c^T,
  //          u_c = 1/Dy on the cell, sigma_c = sum of 1/Dy over the cell.
  //   2. The borders are thin and SKEW: one supply column per (source,
  //      asset) cell (a single 1 per variable row) and one budget column
  //      per vehicle type (rho_p per variable row). Eliminating dy cancels
  //      the signs and leaves an SPD dual Schur complement of size
  //      (numSupplyCells + numTypes),
  //          S = diag(Dmu | Dla) + B^T W B,   B = [E  R],
  //      assembled in two parts -- a per-variable 2x2 accumulation for
  //      B^T diag(1/Dy) B plus a per-cell rank-one downdate on the cell's
  //      support -- and factored by LLT.
  //
  // The fleet differences from the flow factory (flownewton.hpp): the
  // budget border is numTypes columns instead of one dense column, and a
  // cell mixes several sources and several vehicle types, so the downdate
  // support is (distinct sources + distinct types) of the cell.
  //
  // Per-iteration cost is O(numVars + sum of cell support^2) + one small
  // LLT versus the dense LU's O(dim^3) -- the lever that makes the
  // keep-all / no-screen fleet solve feasible (FN3). The engine itself
  // still holds the dense M for residual matvecs; the matrix-free M is
  // stage MF1.
  //
  // The factory is bound to THIS lcp's structure and data (cell-major
  // layout, incidence lists, rho weights, Q_cell from M's diagonal);
  // rebuild it whenever the lcp is rebuilt (e.g. each certificate round).
  // Because the fleet LCP has no free block, the factory rejects a nonzero
  // freeRegularization. During development, mehrotraIpm's newtonCheckTol
  // verifies every solve this factory returns against the engine's own M.
  //
  // Throws std::invalid_argument on an lcp whose fields are inconsistent
  // (empty, sizes disagreeing, variables not cell-major-contiguous, an
  // out-of-range incidence, a non-positive rho, or a non-positive Q
  // diagonal); the returned factory throws std::invalid_argument on a
  // nonzero freeRegularization or a wrong-sized or non-positive sOverY,
  // and std::runtime_error if the Schur complement fails its LLT
  // (numerically not positive definite).
  NewtonSolverFactory makeFleetNewtonFactory(const FleetLcp& lcp);

} // namespace VIMCP::Network

#endif // VIMCP_NETWORK_FLEETNEWTON_HPP
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------

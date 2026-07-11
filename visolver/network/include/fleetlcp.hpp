// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
// Matrix generator and unpacker for the FLEET optimizer: the reduced
// conservative fleet QP's KKT system as a monotone affine complementarity
// problem (M, q), and the expansion of its solution back to a FleetPlan
// (fleet-formulation.md section 9; derivation machine-verified in
// doc/fleet-mcp-check.mac, whose checks are cited below by number).
// ----------------------------------------------
#ifndef VIMCP_NETWORK_FLEETLCP_HPP
#define VIMCP_NETWORK_FLEETLCP_HPP

#include "fleetplan.hpp"
#include "fleetreduction.hpp"

namespace VIMCP::Network {

  // KKT of the reduced conservative fleet QP, empty free block:
  // z = [ y | mu | lambda ], all >= 0, 0 <= G(z) = M z + q _|_ z >= 0.
  //   y      one variable per (kept pair, CAPABLE vehicle type), CELL-MAJOR:
  //          all variables of one demand cell (sink, asset) contiguous --
  //          the analog of the base LCP's sink-major packing.
  //   mu     one supply multiplier per (source, asset) cell.
  //   lambda one budget multiplier per vehicle type (K rows -- G-F8).
  // Blocks (Q_cell = 2 P_ta / D_ta^2, rho = round-trip miles per unit):
  //   G_y      = Q_cell (R_cell - D_cell) + mu_cell(src) + (la_k + eps) rho
  //   G_mu     = C_cell - sum of the cell's y
  //   G_la(k)  = B_k - sum_p rho_pk y_pk
  // jacobian(G) = M entry-for-entry and the monotonicity of M (symmetric
  // part = per-cell rank-one Q blocks; borders skew) are Maxima checks 1-3.
  struct FleetLcp {
    MatrixXd M;              // (numVars + numSupplyCells + numTypes) square;
                             // EMPTY (0 x 0) when built without the dense
                             // matrix (see buildFleetLcp) -- the index lists
                             // below carry the full structure either way
    VectorXd q;
    // Per y variable, aligned with the y block:
    vector<Index> varAsset;      // asset index a
    vector<Index> varSourcePos;  // position into perAsset[a].sources
    vector<Index> varSinkPos;    // position into perAsset[a].sinks
    vector<Index> varType;       // vehicle type k (capable: kappa > 0)
    VectorXd varRho;             // rho = shipCost(s, t) / kappa(a, k)
    vector<Index> varMuIndex;    // 0-based index into the mu block
    vector<Index> varCell;       // 0-based demand-cell index (certificates)
    VectorXd varQuad;            // Q_cell = 2 P / D^2 of the variable's
                                 // demand cell (constant across a cell)
    // Block offsets:
    vector<Index> muOffsetPerAsset;    // mu index of asset a's source 0
    vector<Index> cellOffsetPerAsset;  // cell index of asset a's sink 0
    Index numVars = 0;
    Index numSupplyCells = 0;
    Index numCells = 0;          // demand cells across assets
    Index numTypes = 0;
    double epsilon = 0.0;        // tie-break actually used
  };

  // The R4-analog default tie-break:
  // epsilon = tieBreakRelative * (sum of P over demand cells) / (sum_k B_k),
  // dimensionally consistent (gain rows scale like P, price rows like rho;
  // Maxima check 7 verifies the tie-break's type selection).
  double defaultFleetTieBreakEpsilon(const FleetInstance& inst);

  // Assemble (M, q). Throws std::invalid_argument if epsilon is negative,
  // any budget B_k is not positive, the reduction yields no variables, or a
  // sink's kept list is empty. With assembleDenseMatrixP = false, M is left
  // EMPTY and only q, the index lists, and varQuad are built (O(numVars)
  // instead of O(numVars^2) time and memory) -- sufficient for the
  // matrix-free interior-point path (applyFleetLcpM + the fleet Newton
  // factory, stage MF1), and the only viable form at the production target
  // scale where the dense M does not fit.
  FleetLcp buildFleetLcp(const FleetInstance& inst,
                         const FleetReducedProblem& reduced, double epsilon,
                         bool assembleDenseMatrixP = true);

  // The matrix-vector product M v computed from the index lists in
  // O(numVars), without touching (or requiring) the dense M: per y row
  //   (M v)_p  = Q_cell(p) * (sum of v over p's demand cell)
  //              + v_mu(p) + rho_p v_la(type(p)),
  // per mu row the negated cell supply sum, per lambda row the negated
  // rho-weighted type sum. Agrees with lcp.M * v when the dense M is
  // present. Throws std::invalid_argument on a size mismatch.
  VectorXd applyFleetLcpM(const FleetLcp& lcp, const VectorXd& v);

  // Expand a solution z into a full FleetPlan: each y_pk > 0 walks its
  // OUTBOUND shortest route accumulating flow[a] and vehicles[k], and the
  // REVERSE shortest route accumulating the deadhead vehicles (Lemma FL4;
  // self pairs close their own loop). S and R are per-cell totals. Negative
  // y entries are clamped to 0. The result satisfies checkFleetPlan to
  // accumulation tolerance (circulation is exact only arc-by-arc on
  // single-arc routes; multi-hop loops cancel to rounding). Throws
  // std::invalid_argument on size mismatch or non-finite z.
  FleetPlan unpackFleetLcp(const FleetInstance& inst,
                           const FleetReducedProblem& reduced,
                           const FleetLcp& lcp, const VectorXd& z);

} // namespace VIMCP::Network

#endif // VIMCP_NETWORK_FLEETLCP_HPP
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------

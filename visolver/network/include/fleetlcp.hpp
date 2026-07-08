// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
// Matrix generator and unpacker for the FLEET optimizer: the reduced
// conservative fleet QP's KKT system as a monotone affine complementarity
// problem (M, q), and the expansion of its solution back to a FleetPlan
// (fleet-formulation.md section 9; derivation machine-verified in
// doc/fleet-mcp-check.mac, whose checks are cited below by number).
// ----------------------------------------------
#ifndef VINCP_NETWORK_FLEETLCP_HPP
#define VINCP_NETWORK_FLEETLCP_HPP

#include "fleetplan.hpp"
#include "fleetreduction.hpp"

namespace VINCP::Network {

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
    MatrixXd M;              // (numVars + numSupplyCells + numTypes) square
    VectorXd q;
    // Per y variable, aligned with the y block:
    vector<Index> varAsset;      // asset index a
    vector<Index> varSourcePos;  // position into perAsset[a].sources
    vector<Index> varSinkPos;    // position into perAsset[a].sinks
    vector<Index> varType;       // vehicle type k (capable: kappa > 0)
    VectorXd varRho;             // rho = shipCost(s, t) / kappa(a, k)
    vector<Index> varMuIndex;    // 0-based index into the mu block
    vector<Index> varCell;       // 0-based demand-cell index (certificates)
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
  // sink's kept list is empty.
  FleetLcp buildFleetLcp(const FleetInstance& inst,
                         const FleetReducedProblem& reduced, double epsilon);

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

} // namespace VINCP::Network

#endif // VINCP_NETWORK_FLEETLCP_HPP
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------

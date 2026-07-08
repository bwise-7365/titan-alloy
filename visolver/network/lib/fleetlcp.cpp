// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
// Fleet LCP assembly and unpacking. The algebra is machine-verified in
// doc/fleet-mcp-check.mac; checks are cited by number.
// ----------------------------------------------
#include "fleetlcp.hpp"

#include "flowlcp.hpp"   // tieBreakRelative: the shared R4 tie-break scale

#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace VINCP::Network {

  double
  defaultFleetTieBreakEpsilon(const FleetInstance& inst)
  {
    validateFleetInstance(inst);
    double prioritySum = 0.0;
    for (Index i = 0; i < inst.numNodes; ++i) {
      for (Index a = 0; a < numAssets(inst); ++a) {
        if (0.0 < inst.demand(i, a)) {
          prioritySum += inst.priority(i, a);
        }
      }
    }
    double budgetSum = 0.0;
    for (Index k = 0; k < numVehicleTypes(inst); ++k) {
      budgetSum += vehicleBudget(inst, k);
    }
    if (0.0 >= budgetSum) {
      throw std::invalid_argument(
          "defaultFleetTieBreakEpsilon: total budget must be positive.");
    }
    return tieBreakRelative * prioritySum / budgetSum;
  }

  FleetLcp
  buildFleetLcp(const FleetInstance& inst, const FleetReducedProblem& reduced,
                double epsilon, bool assembleDenseMatrixP)
  {
    validateFleetInstance(inst);
    if (0.0 > epsilon || !std::isfinite(epsilon)) {
      throw std::invalid_argument(
          "buildFleetLcp: epsilon must be finite and non-negative.");
    }
    const Index numA = numAssets(inst);
    const Index numK = numVehicleTypes(inst);
    if (static_cast<Index>(reduced.perAsset.size()) != numA) {
      throw std::invalid_argument(
          "buildFleetLcp: reduction does not match the instance.");
    }
    for (Index k = 0; k < numK; ++k) {
      if (0.0 >= vehicleBudget(inst, k)) {
        throw std::invalid_argument(
            "buildFleetLcp: every vehicle budget B_k must be positive.");
      }
    }

    FleetLcp lcp;
    lcp.epsilon = epsilon;
    lcp.numTypes = numK;
    lcp.muOffsetPerAsset.assign(static_cast<size_t>(numA), 0);
    lcp.cellOffsetPerAsset.assign(static_cast<size_t>(numA), 0);

    // First pass: block offsets and variable count. Cell-major: assets in
    // order, each asset's sinks in order, each sink's kept sources in order,
    // each pair's capable types ascending (Maxima fixture ordering).
    for (Index a = 0; a < numA; ++a) {
      const ReducedProblem& asset = reduced.perAsset[static_cast<size_t>(a)];
      lcp.muOffsetPerAsset[static_cast<size_t>(a)] = lcp.numSupplyCells;
      lcp.cellOffsetPerAsset[static_cast<size_t>(a)] = lcp.numCells;
      lcp.numSupplyCells += static_cast<Index>(asset.sources.size());
      lcp.numCells += static_cast<Index>(asset.sinks.size());
      for (size_t t = 0; t < asset.sinks.size(); ++t) {
        if (asset.kept[t].empty()) {
          throw std::invalid_argument(
              "buildFleetLcp: a sink's kept list is empty.");
        }
        for (const Index s : asset.kept[t]) {
          (void)s;
          for (Index k = 0; k < numK; ++k) {
            if (0.0 < reduced.kappa(a, k)) {
              ++lcp.numVars;
            }
          }
        }
      }
    }
    if (0 == lcp.numVars) {
      throw std::invalid_argument(
          "buildFleetLcp: the reduction yields no variables.");
    }

    // Second pass: per-variable bookkeeping.
    lcp.varAsset.reserve(static_cast<size_t>(lcp.numVars));
    lcp.varSourcePos.reserve(static_cast<size_t>(lcp.numVars));
    lcp.varSinkPos.reserve(static_cast<size_t>(lcp.numVars));
    lcp.varType.reserve(static_cast<size_t>(lcp.numVars));
    lcp.varMuIndex.reserve(static_cast<size_t>(lcp.numVars));
    lcp.varCell.reserve(static_cast<size_t>(lcp.numVars));
    lcp.varRho.resize(lcp.numVars);
    Index p = 0;
    for (Index a = 0; a < numA; ++a) {
      const ReducedProblem& asset = reduced.perAsset[static_cast<size_t>(a)];
      for (size_t t = 0; t < asset.sinks.size(); ++t) {
        for (const Index s : asset.kept[t]) {
          for (Index k = 0; k < numK; ++k) {
            if (0.0 >= reduced.kappa(a, k)) {
              continue;
            }
            lcp.varAsset.push_back(a);
            lcp.varSourcePos.push_back(s);
            lcp.varSinkPos.push_back(static_cast<Index>(t));
            lcp.varType.push_back(k);
            lcp.varMuIndex.push_back(
                lcp.muOffsetPerAsset[static_cast<size_t>(a)] + s);
            lcp.varCell.push_back(
                lcp.cellOffsetPerAsset[static_cast<size_t>(a)]
                + static_cast<Index>(t));
            lcp.varRho(p) = asset.shipCost(s, static_cast<Index>(t))
                            / reduced.kappa(a, k);
            ++p;
          }
        }
      }
    }

    // Assembly (Maxima checks 1-3): rank-one Q block per demand cell, skew
    // mu incidence, skew per-type budget borders. varQuad and q are always
    // built; the dense M only on request (the matrix-free path never needs
    // it, and its Q-block loop alone is O(numVars^2)).
    const Index dim = lcp.numVars + lcp.numSupplyCells + numK;
    const Index muBase = lcp.numVars;
    const Index laBase = lcp.numVars + lcp.numSupplyCells;
    lcp.M = assembleDenseMatrixP ? MatrixXd::Zero(dim, dim) : MatrixXd();
    lcp.q = VectorXd::Zero(dim);
    lcp.varQuad.resize(lcp.numVars);

    for (Index i = 0; i < lcp.numVars; ++i) {
      const Index a = lcp.varAsset[static_cast<size_t>(i)];
      const ReducedProblem& asset = reduced.perAsset[static_cast<size_t>(a)];
      const Index sinkNode =
          asset.sinks[static_cast<size_t>(lcp.varSinkPos[static_cast<size_t>(i)])];
      const double demand = inst.demand(sinkNode, a);
      const double quad = 2.0 * inst.priority(sinkNode, a) / (demand * demand);
      lcp.varQuad(i) = quad;

      if (assembleDenseMatrixP) {
        for (Index r = 0; r < lcp.numVars; ++r) {
          if (lcp.varCell[static_cast<size_t>(r)]
              == lcp.varCell[static_cast<size_t>(i)]) {
            lcp.M(i, r) = quad;
          }
        }
        const Index muRow = muBase + lcp.varMuIndex[static_cast<size_t>(i)];
        const Index laRow = laBase + lcp.varType[static_cast<size_t>(i)];
        lcp.M(i, muRow) = 1.0;
        lcp.M(muRow, i) = -1.0;
        lcp.M(i, laRow) = lcp.varRho(i);
        lcp.M(laRow, i) = -lcp.varRho(i);
      }

      lcp.q(i) = -2.0 * inst.priority(sinkNode, a) / demand
                 + epsilon * lcp.varRho(i);
    }
    for (Index a = 0; a < numA; ++a) {
      const ReducedProblem& asset = reduced.perAsset[static_cast<size_t>(a)];
      for (size_t s = 0; s < asset.sources.size(); ++s) {
        const Index sourceNode = asset.sources[s];
        lcp.q(muBase + lcp.muOffsetPerAsset[static_cast<size_t>(a)]
              + static_cast<Index>(s)) = inst.supplyCap(sourceNode, a);
      }
    }
    for (Index k = 0; k < numK; ++k) {
      lcp.q(laBase + k) = vehicleBudget(inst, k);
    }
    return lcp;
  }

  VectorXd
  applyFleetLcpM(const FleetLcp& lcp, const VectorXd& v)
  {
    const Index dim = lcp.numVars + lcp.numSupplyCells + lcp.numTypes;
    if (v.size() != dim || lcp.varQuad.size() != lcp.numVars) {
      throw std::invalid_argument(
          "applyFleetLcpM: v or the lcp index data has the wrong size.");
    }
    const Index muBase = lcp.numVars;
    const Index laBase = lcp.numVars + lcp.numSupplyCells;

    // Cell sums of the y block feed every same-cell Q entry at once.
    VectorXd cellSum = VectorXd::Zero(lcp.numCells);
    for (Index p = 0; p < lcp.numVars; ++p) {
      cellSum(lcp.varCell[static_cast<size_t>(p)]) += v(p);
    }

    VectorXd result = VectorXd::Zero(dim);
    for (Index p = 0; p < lcp.numVars; ++p) {
      const Index mu = lcp.varMuIndex[static_cast<size_t>(p)];
      const Index la = lcp.varType[static_cast<size_t>(p)];
      result(p) = lcp.varQuad(p) * cellSum(lcp.varCell[static_cast<size_t>(p)])
                  + v(muBase + mu) + lcp.varRho(p) * v(laBase + la);
      result(muBase + mu) -= v(p);
      result(laBase + la) -= lcp.varRho(p) * v(p);
    }
    return result;
  }

  FleetPlan
  unpackFleetLcp(const FleetInstance& inst, const FleetReducedProblem& reduced,
                 const FleetLcp& lcp, const VectorXd& z)
  {
    const Index dim = lcp.numVars + lcp.numSupplyCells + lcp.numTypes;
    if (z.size() != dim || !z.allFinite()) {
      throw std::invalid_argument(
          "unpackFleetLcp: z has the wrong size or non-finite entries.");
    }

    FleetPlan plan = makeZeroFleetPlan(inst);
    for (Index i = 0; i < lcp.numVars; ++i) {
      const double units = std::max(z(i), 0.0);
      if (0.0 >= units) {
        continue;
      }
      const Index a = lcp.varAsset[static_cast<size_t>(i)];
      const Index k = lcp.varType[static_cast<size_t>(i)];
      const ReducedProblem& asset = reduced.perAsset[static_cast<size_t>(a)];
      const Index fromNode = asset.sources[static_cast<size_t>(
          lcp.varSourcePos[static_cast<size_t>(i)])];
      const Index toNode = asset.sinks[static_cast<size_t>(
          lcp.varSinkPos[static_cast<size_t>(i)])];
      const double vehicleCount = units / reduced.kappa(a, k);

      // Outbound route: cargo and loaded vehicles (Lemma FL4).
      const vector<Index> out = routeNodes(reduced.routes, fromNode, toNode);
      for (size_t step = 0; step + 1 < out.size(); ++step) {
        plan.flow[static_cast<size_t>(a)](out[step], out[step + 1]) += units;
        plan.vehicles[static_cast<size_t>(k)](out[step], out[step + 1]) +=
            vehicleCount;
      }
      // Deadhead return along the reverse shortest route; a self pair's
      // route above is already a closed loop and needs no return.
      if (fromNode != toNode) {
        const vector<Index> back =
            routeNodes(reduced.routes, toNode, fromNode);
        for (size_t step = 0; step + 1 < back.size(); ++step) {
          plan.vehicles[static_cast<size_t>(k)](back[step], back[step + 1]) +=
              vehicleCount;
        }
      }
      plan.supplied(fromNode, a) += units;
      plan.resupply(toNode, a) += units;
    }
    return plan;
  }

} // namespace VINCP::Network
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------

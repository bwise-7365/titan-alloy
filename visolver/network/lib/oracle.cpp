// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
// Full-formulation KKT assembly and the dHan06-based oracle solve.
// ----------------------------------------------
#include "oracle.hpp"

#include "dhan06.hpp"

#include <algorithm>
#include <stdexcept>

namespace VINCP::Network {

  OracleKkt
  buildOracleKkt(const Instance& inst)
  {
    validateInstance(inst);
    if (0.0 >= inst.tonMileLimit) {
      throw std::invalid_argument(
          "buildOracleKkt: tonMileLimit must be calibrated (> 0).");
    }
    const Index m = inst.numNodes;

    OracleKkt kkt;
    kkt.numNodes = m;
    kkt.demandNodes = sinkNodes(inst);
    const Index numDemand = static_cast<Index>(kkt.demandNodes.size());
    kkt.fBase = m;                        // after the free nu block
    kkt.sBase = kkt.fBase + m * m;
    kkt.rBase = kkt.sBase + m;
    kkt.deltaBase = kkt.rBase + numDemand;
    kkt.muBase = kkt.deltaBase + m;
    kkt.lambdaIndex = kkt.muBase + m;
    const Index dim = kkt.lambdaIndex + 1;
    kkt.M = MatrixXd::Zero(dim, dim);
    kkt.q = VectorXd::Zero(dim);

    const auto fIndex = [&kkt, m](Index a, Index b) {
      return kkt.fBase + a * m + b;
    };

    // Arc columns/rows. The += / -= pairs make the diagonal arcs (a == b)
    // cancel out of the balance and nu terms exactly as the model says.
    for (Index a = 0; a < m; ++a) {
      for (Index b = 0; b < m; ++b) {
        const Index arc = fIndex(a, b);
        // balance rows H_nu: inflow to b, outflow from a
        kkt.M(b, arc) += 1.0;
        kkt.M(a, arc) -= 1.0;
        // arc stationarity G_f = nu_a - nu_b - delta_b + lambda c_ab
        kkt.M(arc, a) += 1.0;
        kkt.M(arc, b) -= 1.0;
        kkt.M(arc, kkt.deltaBase + b) = -1.0;
        kkt.M(arc, kkt.lambdaIndex) = inst.cost(a, b);
        // delivery row G_delta(b): + f_ab; budget row: - c_ab f_ab
        kkt.M(kkt.deltaBase + b, arc) = 1.0;
        kkt.M(kkt.lambdaIndex, arc) = -inst.cost(a, b);
      }
    }

    // Supply columns/rows.
    for (Index i = 0; i < m; ++i) {
      kkt.M(i, kkt.sBase + i) = 1.0;                 // balance: + S_i
      kkt.M(kkt.sBase + i, i) = -1.0;                // G_S: - nu_i
      kkt.M(kkt.sBase + i, kkt.muBase + i) = 1.0;    // G_S: + mu_i
      kkt.M(kkt.muBase + i, kkt.sBase + i) = -1.0;   // G_mu: - S_i
      kkt.q(kkt.muBase + i) = inst.supplyCap(i);     // G_mu: + C_i
    }

    // Resupply columns/rows (demand nodes only).
    for (Index pos = 0; pos < numDemand; ++pos) {
      const Index node = kkt.demandNodes[static_cast<size_t>(pos)];
      const Index row = kkt.rBase + pos;
      const double demand = inst.demand(node);
      kkt.M(row, row) = 2.0 * inst.priority(node) / (demand * demand);
      kkt.M(row, kkt.deltaBase + node) = 1.0;        // G_R: + delta
      kkt.M(row, node) = 1.0;                        // G_R: + nu
      kkt.q(row) = -2.0 * inst.priority(node) / demand;
      kkt.M(node, row) = -1.0;                       // balance: - R
      kkt.M(kkt.deltaBase + node, row) = -1.0;       // delivery: - R
    }

    kkt.q(kkt.lambdaIndex) = inst.tonMileLimit;      // budget: + L
    return kkt;
  }

  Plan
  unpackOracle(const Instance& inst, const OracleKkt& kkt, const VectorXd& z)
  {
    if (z.size() != kkt.q.size()) {
      throw std::invalid_argument("unpackOracle: z has the wrong size.");
    }
    const Index m = inst.numNodes;
    Plan plan = makeZeroPlan(inst);
    for (Index a = 0; a < m; ++a) {
      for (Index b = 0; b < m; ++b) {
        plan.flow(a, b) = std::max(z(kkt.fBase + a * m + b), 0.0);
      }
      plan.supplied(a) = std::max(z(kkt.sBase + a), 0.0);
    }
    for (Index pos = 0;
         pos < static_cast<Index>(kkt.demandNodes.size()); ++pos) {
      const Index node = kkt.demandNodes[static_cast<size_t>(pos)];
      plan.resupply(node) = std::max(z(kkt.rBase + pos), 0.0);
    }
    return plan;
  }

  OracleResult
  solveOracle(const Instance& inst, const OracleParams& params)
  {
    if (!(0.0 < params.magTol) || 0 >= params.iterMax || 1 > params.maxNodes) {
      throw std::invalid_argument("solveOracle: invalid params.");
    }
    if (params.maxNodes < inst.numNodes) {
      throw std::invalid_argument(
          "solveOracle: instance too large for the dense full-formulation "
          "oracle; it exists to check tiny cases (raise maxNodes knowingly).");
    }

    // Nondimensionalize: solve in units of the largest demand (tons) and the
    // largest arc cost (miles). theta depends only on the ratios R/D and on
    // P, and every constraint is homogeneous under the unit change, so the
    // optimum maps 1:1. The projection method needs this: the raw KKT mixes
    // ~1e3-scale budget rows with ~1e-4-scale multipliers and stalls, while
    // the O(1)-scaled system contracts quickly.
    double tonScale = inst.demand.maxCoeff();
    if (0.0 >= tonScale) {
      tonScale = 1.0;                    // no demand: nothing to scale for
    }
    const double mileScale = inst.cost.maxCoeff();
    Instance scaled = inst;
    scaled.supplyCap /= tonScale;
    scaled.demand /= tonScale;
    scaled.cost /= mileScale;
    scaled.tonMileLimit /= tonScale * mileScale;

    const OracleKkt kkt = buildOracleKkt(scaled);

    OracleResult result;
    const VectorXd z0 = VectorXd::Zero(kkt.q.size());
    result.vi = dHan06(z0, kkt.M, kkt.q, makeMixedProjector(inst.numNodes),
                       params.magTol, params.iterMax, params.iterFreq);
    result.plan = unpackOracle(scaled, kkt, result.vi.z);
    result.plan.flow *= tonScale;        // back to real tons
    result.plan.supplied *= tonScale;
    result.plan.resupply *= tonScale;
    result.shortfall = shortfallObjective(inst, result.plan);
    return result;
  }

} // namespace VINCP::Network
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------

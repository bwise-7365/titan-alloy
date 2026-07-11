// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
// Assembly of the reduced-QP KKT complementarity system and the route-based
// unpacker.
// ----------------------------------------------
#include "flowlcp.hpp"

#include <algorithm>
#include <stdexcept>

namespace VIMCP::Network {

  double
  defaultTieBreakEpsilon(const Instance& inst)
  {
    validateInstance(inst);
    if (0.0 >= inst.tonMileLimit) {
      throw std::invalid_argument(
          "defaultTieBreakEpsilon: tonMileLimit must be calibrated (> 0).");
    }
    double prioritySum = 0.0;
    for (Index i = 0; i < inst.numNodes; ++i) {
      if (0.0 < inst.demand(i)) {
        prioritySum += inst.priority(i);
      }
    }
    return tieBreakRelative * prioritySum / inst.tonMileLimit;
  }

  FlowLcp
  buildFlowLcp(const Instance& inst, const ReducedProblem& reduced,
               double epsilon)
  {
    validateInstance(inst);
    if (0.0 >= inst.tonMileLimit) {
      throw std::invalid_argument(
          "buildFlowLcp: tonMileLimit must be calibrated (> 0).");
    }
    if (0.0 > epsilon) {
      throw std::invalid_argument("buildFlowLcp: epsilon must be >= 0.");
    }
    const Index numSources = static_cast<Index>(reduced.sources.size());
    const Index numSinks = static_cast<Index>(reduced.sinks.size());
    if (0 == numSources || 0 == numSinks) {
      throw std::invalid_argument("buildFlowLcp: reduced problem is empty.");
    }

    FlowLcp lcp;
    lcp.epsilon = epsilon;
    lcp.numSources = numSources;

    // Enumerate the t variables sink-major, following the kept lists, so each
    // sink's pairs are contiguous (its Q-block is a contiguous square).
    for (Index t = 0; t < numSinks; ++t) {
      const vector<Index>& kept = reduced.kept[static_cast<size_t>(t)];
      if (kept.empty()) {
        throw std::invalid_argument(
            "buildFlowLcp: a sink has no admitted sources.");
      }
      for (const Index s : kept) {
        lcp.pairSourcePos.push_back(s);
        lcp.pairSinkPos.push_back(t);
      }
    }
    lcp.numPairs = static_cast<Index>(lcp.pairSourcePos.size());
    lcp.pairCost.resize(lcp.numPairs);
    for (Index p = 0; p < lcp.numPairs; ++p) {
      lcp.pairCost(p) = reduced.shipCost(lcp.pairSourcePos[static_cast<size_t>(p)],
                                         lcp.pairSinkPos[static_cast<size_t>(p)]);
    }

    const Index dim = lcp.numPairs + numSources + 1;
    const Index muBase = lcp.numPairs;
    const Index laRow = lcp.numPairs + numSources;
    lcp.M = MatrixXd::Zero(dim, dim);
    lcp.q = VectorXd::Zero(dim);

    for (Index p = 0; p < lcp.numPairs; ++p) {
      const Index sinkPos = lcp.pairSinkPos[static_cast<size_t>(p)];
      const Index sinkNode = reduced.sinks[static_cast<size_t>(sinkPos)];
      const double demand = inst.demand(sinkNode);
      const double quad = 2.0 * inst.priority(sinkNode) / (demand * demand);

      // Q-block: pairs delivering to the same sink couple through Q_n.
      for (Index r = 0; r < lcp.numPairs; ++r) {
        if (lcp.pairSinkPos[static_cast<size_t>(r)] == sinkPos) {
          lcp.M(p, r) = quad;
        }
      }
      // Capacity incidence (skew pair) and budget row/column (skew pair).
      const Index muRow = muBase + lcp.pairSourcePos[static_cast<size_t>(p)];
      lcp.M(p, muRow) = 1.0;
      lcp.M(muRow, p) = -1.0;
      lcp.M(p, laRow) = lcp.pairCost(p);
      lcp.M(laRow, p) = -lcp.pairCost(p);

      lcp.q(p) = -2.0 * inst.priority(sinkNode) / demand
                 + epsilon * lcp.pairCost(p);
    }
    for (Index s = 0; s < numSources; ++s) {
      lcp.q(muBase + s) =
          inst.supplyCap(reduced.sources[static_cast<size_t>(s)]);
    }
    lcp.q(laRow) = inst.tonMileLimit;
    return lcp;
  }

  Plan
  unpackFlowLcp(const Instance& inst, const ShortestRoutes& routes,
                const ReducedProblem& reduced, const FlowLcp& lcp,
                const VectorXd& z)
  {
    const Index dim = lcp.numPairs + lcp.numSources + 1;
    if (z.size() != dim) {
      throw std::invalid_argument("unpackFlowLcp: z has the wrong size.");
    }
    if (!z.allFinite()) {
      throw std::invalid_argument("unpackFlowLcp: z has non-finite entries.");
    }

    Plan plan = makeZeroPlan(inst);
    for (Index p = 0; p < lcp.numPairs; ++p) {
      const double tons = std::max(z(p), 0.0);
      if (0.0 < tons) {
        const Index fromNode =
            reduced.sources[static_cast<size_t>(lcp.pairSourcePos[static_cast<size_t>(p)])];
        const Index toNode =
            reduced.sinks[static_cast<size_t>(lcp.pairSinkPos[static_cast<size_t>(p)])];
        const vector<Index> nodes = routeNodes(routes, fromNode, toNode);
        for (size_t step = 0; step + 1 < nodes.size(); ++step) {
          plan.flow(nodes[step], nodes[step + 1]) += tons;
        }
        plan.supplied(fromNode) += tons;
        plan.resupply(toNode) += tons;
      }
    }
    return plan;
  }

} // namespace VIMCP::Network
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------

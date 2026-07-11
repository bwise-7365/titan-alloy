// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
// Production solver implementation: scaling, the bsHe94b solve, and the R3
// certificate loop.
// ----------------------------------------------
#include "flowplan.hpp"

#include "flownewton.hpp"

#include "bshe94b.hpp"
#include "chainedsolver.hpp"
#include "mehrotraipm.hpp"
#include "semismoothnewton.hpp"

#include <algorithm>
#include <stdexcept>

namespace VIMCP::Network {

  namespace {

    void
    validateFlowPlanParams(const FlowPlanParams& params)
    {
      if (!(0.0 < params.magTol) || 0 >= params.iterMax) {
        throw std::invalid_argument(
            "solveFlowPlan: magTol must be positive and iterMax > 0.");
      }
      if ("bshe94b" != params.engine && "chain" != params.engine
          && "ipm" != params.engine && "ssn" != params.engine) {
        throw std::invalid_argument(
            "solveFlowPlan: engine must be 'bshe94b', 'chain', 'ipm', or 'ssn', not '"
            + params.engine + "'.");
      }
      if (!(0.0 < params.roughMagTol) || 0 >= params.roughIterMax) {
        throw std::invalid_argument(
            "solveFlowPlan: chain rough params must be positive.");
      }
      if ("dense" != params.ipmNewton && "flow" != params.ipmNewton) {
        throw std::invalid_argument(
            "solveFlowPlan: ipmNewton must be 'dense' or 'flow', not '"
            + params.ipmNewton + "'.");
      }
      if (0.0 > params.newtonCheckTol) {
        throw std::invalid_argument(
            "solveFlowPlan: newtonCheckTol must be non-negative.");
      }
      if (0 > params.maxSourcesPerSink || 0.0 > params.gapFraction
          || 0 > params.maxCertificateRounds
          || 0.0 > params.certificateSlack) {
        throw std::invalid_argument(
            "solveFlowPlan: screen/certificate params must be non-negative.");
      }
      return;
    }

    // Proposition R3, dual-feasibility check of every EXCLUDED pair at the
    // current solution (all in scaled units). Violating sources are appended
    // to their sink's kept list; returns whether any were.
    bool
    addCertificateViolationsP(const Instance& scaled, const FlowLcp& lcp,
                              const VectorXd& z, double epsilon, double slack,
                              ReducedProblem& reduced)
    {
      const Index numSources = static_cast<Index>(reduced.sources.size());
      const Index numSinks = static_cast<Index>(reduced.sinks.size());

      // Delivered tons per sink position, and the kept-pair mask.
      VectorXd delivered = VectorXd::Zero(numSinks);
      vector<vector<bool>> keptP(static_cast<size_t>(numSinks),
                                 vector<bool>(static_cast<size_t>(numSources),
                                              false));
      for (Index p = 0; p < lcp.numPairs; ++p) {
        const Index sinkPos = lcp.pairSinkPos[static_cast<size_t>(p)];
        const Index sourcePos = lcp.pairSourcePos[static_cast<size_t>(p)];
        delivered(sinkPos) += std::max(z(p), 0.0);
        keptP[static_cast<size_t>(sinkPos)][static_cast<size_t>(sourcePos)] =
            true;
      }
      const double lambda = std::max(z(lcp.numPairs + numSources), 0.0);

      bool anyViolationP = false;
      for (Index t = 0; t < numSinks; ++t) {
        const Index sinkNode = reduced.sinks[static_cast<size_t>(t)];
        const double demand = scaled.demand(sinkNode);
        const double gain = 2.0 * scaled.priority(sinkNode)
                            * (demand - delivered(t)) / (demand * demand);
        for (Index s = 0; s < numSources; ++s) {
          if (!keptP[static_cast<size_t>(t)][static_cast<size_t>(s)]) {
            const double mu = std::max(z(lcp.numPairs + s), 0.0);
            const double price =
                (lambda + epsilon) * reduced.shipCost(s, t) + mu;
            if (gain > price + slack) {
              reduced.kept[static_cast<size_t>(t)].push_back(s);
              anyViolationP = true;
            }
          }
        }
      }
      return anyViolationP;
    }

  } // namespace

  FlowPlanResult
  solveFlowPlan(const Instance& inst, const FlowPlanParams& params)
  {
    validateInstance(inst);
    validateFlowPlanParams(params);
    if (0.0 >= inst.tonMileLimit) {
      throw std::invalid_argument(
          "solveFlowPlan: tonMileLimit must be calibrated (> 0); run "
          "greedyPlan first.");
    }

    // Nondimensionalize (see oracle.hpp for the argument: an exact unit
    // change the projection method needs -- the raw system mixes ~1e8 budget
    // rows with ~1e-8 multipliers at full scale and stalls).
    double tonScale = inst.demand.maxCoeff();
    if (0.0 >= tonScale) {
      tonScale = 1.0;
    }
    const double mileScale = inst.cost.maxCoeff();
    Instance scaled = inst;
    scaled.supplyCap /= tonScale;
    scaled.demand /= tonScale;
    scaled.cost /= mileScale;
    scaled.tonMileLimit /= tonScale * mileScale;

    const ShortestRoutes routes = computeShortestRoutes(scaled);
    ScreenParams screen;
    screen.maxSourcesPerSink = params.maxSourcesPerSink;
    screen.gapFraction = params.gapFraction;
    ReducedProblem reduced = makeReducedProblem(scaled, routes, screen);
    const double epsilon = (0.0 > params.epsilon)
                               ? defaultTieBreakEpsilon(scaled)
                               : params.epsilon;
    const Index numSources = static_cast<Index>(reduced.sources.size());
    const bool screenedP =
        (0 < params.maxSourcesPerSink
         && params.maxSourcesPerSink < numSources)
        || 0.0 < params.gapFraction;

    FlowPlanResult result;
    result.totalPairs =
        numSources * static_cast<Index>(reduced.sinks.size());

    // Solve; with an active screen, re-solve until the R3 certificate passes
    // (each round permanently adds columns, so this terminates).
    FlowLcp lcp;
    for (int round = 0; round <= params.maxCertificateRounds; ++round) {
      lcp = buildFlowLcp(scaled, reduced, epsilon);
      const VectorXd z0 =
          VectorXd::Zero(lcp.numPairs + lcp.numSources + 1);
      if ("chain" == params.engine) {
        ChainedSolverParams chainParams;
        chainParams.roughMagTol = params.roughMagTol;
        chainParams.roughIterMax = params.roughIterMax;
        result.vi = chainedSolodovHe(z0, lcp.M, lcp.q, projectNonnegative,
                                     params.magTol, params.iterMax,
                                     params.iterFreq, chainParams);
      }
      else if ("ipm" == params.engine) {
        // The flow LCP is a pure NCP (t, mu, lambda all complementary; no
        // free block), so numFree = 0; the engine ignores z0 by design.
        // ipmNewton picks the linear algebra: an empty factory is the dense
        // LU; "flow" is the structured per-sink factory, rebuilt each round
        // because it is bound to this round's lcp.
        MehrotraIpmParams ipmParams;
        ipmParams.newtonCheckTol = params.newtonCheckTol;
        const NewtonSolverFactory factory =
            ("flow" == params.ipmNewton) ? makeFlowNewtonFactory(lcp)
                                         : NewtonSolverFactory{};
        result.vi = mehrotraIpm(lcp.M, lcp.q, 0, params.magTol,
                                params.iterMax, params.iterFreq, ipmParams,
                                IterationLogger{}, factory);
      }
      else if ("ssn" == params.engine) {
        // Same system as a pure-NCP VIModel with its exact constant
        // Jacobian; like "ipm", each iteration is one factorization.
        const VIModel lcpModel = makeVIModel(
            0, lcp.M.rows(),
            [&lcp](const VectorXd& v) -> VectorXd { return lcp.M * v + lcp.q; });
        SemismoothNewtonParams ssnParams;
        ssnParams.magTol = params.magTol;
        ssnParams.iterMax = params.iterMax;
        ssnParams.iterFreq = params.iterFreq;
        ssnParams.jacobian = [&lcp](const VectorXd&) -> MatrixXd { return lcp.M; };
        result.vi = semismoothNewtonSolve(lcpModel, z0, ssnParams);
      }
      else {
        result.vi = bsHe94b(z0, lcp.M, lcp.q, projectNonnegative,
                            params.magTol, params.iterMax, params.iterFreq);
      }
      result.certificateRounds = round;
      if (!result.vi.converged) {
        break;                       // honest return; certifiedP stays false
      }
      if (!screenedP) {
        result.certifiedP = true;    // every pair was in the problem
        break;
      }
      if (!addCertificateViolationsP(scaled, lcp, result.vi.z, epsilon,
                                     params.certificateSlack, reduced)) {
        result.certifiedP = true;    // all excluded pairs priced out
        break;
      }
      // else: violated pairs were appended; rebuild and re-solve.
    }
    result.keptPairs = lcp.numPairs;

    result.plan = unpackFlowLcp(scaled, routes, reduced, lcp, result.vi.z);
    result.plan.flow *= tonScale;
    result.plan.supplied *= tonScale;
    result.plan.resupply *= tonScale;
    result.shortfall = shortfallObjective(inst, result.plan);
    result.tonMilesUsed = tonMiles(inst, result.plan);
    const double lambdaScaled =
        std::max(result.vi.z(lcp.numPairs + lcp.numSources), 0.0);
    result.budgetShadowPrice = lambdaScaled / (tonScale * mileScale);
    return result;
  }

} // namespace VIMCP::Network
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------

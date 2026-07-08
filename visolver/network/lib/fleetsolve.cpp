// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
// Fleet optimizer implementation: scaling, engine dispatch, the per-asset
// certificate loop, and real-unit results.
// ----------------------------------------------
#include "fleetsolve.hpp"

#include "bshe94b.hpp"
#include "fleetnewton.hpp"
#include "mehrotraipm.hpp"
#include "semismoothnewton.hpp"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <limits>
#include <stdexcept>

namespace VINCP::Network {

  namespace {

    void
    validateFleetSolveParams(const FleetSolveParams& params)
    {
      if (!(0.0 < params.magTol) || 0 >= params.iterMax) {
        throw std::invalid_argument(
            "solveFleetPlan: magTol must be positive and iterMax > 0.");
      }
      if ("ipm" != params.engine && "bshe94b" != params.engine
          && "ssn" != params.engine) {
        throw std::invalid_argument(
            "solveFleetPlan: engine must be 'ipm', 'bshe94b', or 'ssn', not '"
            + params.engine + "'.");
      }
      if ("dense" != params.ipmNewton && "fleet" != params.ipmNewton) {
        throw std::invalid_argument(
            "solveFleetPlan: ipmNewton must be 'dense' or 'fleet', not '"
            + params.ipmNewton + "'.");
      }
      if (0 > params.maxSourcesPerSink || 0.0 > params.gapFraction
          || 0 > params.maxCertificateRounds
          || 0.0 > params.certificateSlack) {
        throw std::invalid_argument(
            "solveFleetPlan: screen/certificate params must be non-negative.");
      }
      return;
    }

    // The R3-analog dual-feasibility check of every EXCLUDED (source, sink)
    // pair of every asset, at the current solution (scaled units). A pair
    // enters with ALL capable types at once, so its price is the CHEAPEST
    // capable type's price; violating sources are appended to their sink's
    // kept list. Returns whether any were.
    bool
    addFleetCertificateViolationsP(const FleetInstance& scaled,
                                   const FleetLcp& lcp, const VectorXd& z,
                                   double epsilon, double slack,
                                   FleetReducedProblem& reduced)
    {
      const Index numA = numAssets(scaled);
      const Index numK = numVehicleTypes(scaled);
      const Index muBase = lcp.numVars;
      const Index laBase = lcp.numVars + lcp.numSupplyCells;

      // Delivered units per demand cell, and the kept mask per asset.
      VectorXd delivered = VectorXd::Zero(lcp.numCells);
      for (Index p = 0; p < lcp.numVars; ++p) {
        delivered(lcp.varCell[static_cast<size_t>(p)]) += std::max(z(p), 0.0);
      }

      bool anyViolationP = false;
      for (Index a = 0; a < numA; ++a) {
        ReducedProblem& asset = reduced.perAsset[static_cast<size_t>(a)];
        const Index numSources = static_cast<Index>(asset.sources.size());
        const Index numSinks = static_cast<Index>(asset.sinks.size());
        for (Index t = 0; t < numSinks; ++t) {
          vector<bool> keptP(static_cast<size_t>(numSources), false);
          for (const Index s : asset.kept[static_cast<size_t>(t)]) {
            keptP[static_cast<size_t>(s)] = true;
          }
          const Index sinkNode = asset.sinks[static_cast<size_t>(t)];
          const double demand = scaled.demand(sinkNode, a);
          const Index cell =
              lcp.cellOffsetPerAsset[static_cast<size_t>(a)] + t;
          const double gain = 2.0 * scaled.priority(sinkNode, a)
                              * (demand - delivered(cell))
                              / (demand * demand);
          for (Index s = 0; s < numSources; ++s) {
            if (keptP[static_cast<size_t>(s)]) {
              continue;
            }
            const double mu = std::max(
                z(muBase + lcp.muOffsetPerAsset[static_cast<size_t>(a)] + s),
                0.0);
            double price = std::numeric_limits<double>::infinity();
            for (Index k = 0; k < numK; ++k) {
              if (0.0 < reduced.kappa(a, k)) {
                const double lambda = std::max(z(laBase + k), 0.0);
                const double rho =
                    asset.shipCost(s, t) / reduced.kappa(a, k);
                price = std::min(price, (lambda + epsilon) * rho + mu);
              }
            }
            if (gain > price + slack) {
              asset.kept[static_cast<size_t>(t)].push_back(s);
              anyViolationP = true;
            }
          }
        }
      }
      return anyViolationP;
    }

  } // namespace

  FleetSolveResult
  solveFleetPlan(const FleetInstance& inst, const FleetSolveParams& params)
  {
    validateFleetInstance(inst);
    validateFleetSolveParams(params);
    const Index numK = numVehicleTypes(inst);

    // Nondimensionalize: units by the largest demand, miles by the largest
    // distance. Budgets B_k = N_k v_k H carry units x miles (one vehicle-mile
    // moves kappa unit-miles), so the COUNTS absorb both scales; kappa is a
    // pure ratio of type data and is untouched. An exact change of variables,
    // as in solveFlowPlan.
    double unitScale = inst.demand.maxCoeff();
    if (0.0 >= unitScale) {
      unitScale = 1.0;
    }
    const double mileScale = inst.distance.maxCoeff();
    FleetInstance scaled = inst;
    scaled.supplyCap /= unitScale;
    scaled.demand /= unitScale;
    scaled.distance /= mileScale;
    for (VehicleType& vehicle : scaled.vehicles) {
      vehicle.count /= unitScale * mileScale;
    }

    ScreenParams screen;
    screen.maxSourcesPerSink = params.maxSourcesPerSink;
    screen.gapFraction = params.gapFraction;
    FleetReducedProblem reduced = makeFleetReducedProblem(scaled, screen);
    const double epsilon = (0.0 > params.epsilon)
                               ? defaultFleetTieBreakEpsilon(scaled)
                               : params.epsilon;

    FleetSolveResult result;
    bool screenedP = 0.0 < params.gapFraction;
    result.totalPairs = 0;
    for (Index a = 0; a < numAssets(inst); ++a) {
      const ReducedProblem& asset = reduced.perAsset[static_cast<size_t>(a)];
      Index capable = 0;
      for (Index k = 0; k < numK; ++k) {
        if (0.0 < reduced.kappa(a, k)) {
          ++capable;
        }
      }
      const Index numSources = static_cast<Index>(asset.sources.size());
      result.totalPairs +=
          numSources * static_cast<Index>(asset.sinks.size()) * capable;
      screenedP = screenedP
                  || (0 < params.maxSourcesPerSink
                      && params.maxSourcesPerSink < numSources);
    }

    // Solve; with an active screen, re-solve until the certificate passes
    // (columns are only ever added, so this terminates). The structured ipm
    // path is fully matrix-free (MF1): the dense M is neither assembled nor
    // held -- at the production target scale it would not fit.
    const bool matrixFreeP =
        ("ipm" == params.engine && "fleet" == params.ipmNewton);
    FleetLcp lcp;
    for (int round = 0; round <= params.maxCertificateRounds; ++round) {
      lcp = buildFleetLcp(scaled, reduced, epsilon, !matrixFreeP);
      const Index dim = lcp.numVars + lcp.numSupplyCells + lcp.numTypes;
      const VectorXd z0 = VectorXd::Zero(dim);
      if (params.roundStartLogger) {
        params.roundStartLogger(round, lcp.numVars, dim);
      }
      const auto solveStart = std::chrono::steady_clock::now();
      if ("ipm" == params.engine) {
        // Pure NCP: numFree = 0. ipmNewton selects the Newton linear
        // algebra: the structured fleet factory (FN2) with the matrix-free
        // field (MF1), both rebuilt each certificate round with the lcp, or
        // the generic dense-LU factory on the explicit M.
        MehrotraIpmParams ipmParams;
        ipmParams.newtonCheckTol = params.newtonCheckTol;
        if (matrixFreeP) {
          const MatrixApply applyM = [&lcp](const VectorXd& v) -> VectorXd {
            return applyFleetLcpM(lcp, v);
          };
          result.vi = mehrotraIpm(applyM, lcp.q, 0, params.magTol,
                                  params.iterMax, params.iterFreq,
                                  ipmParams, params.logger,
                                  makeFleetNewtonFactory(lcp));
        }
        else {
          result.vi = mehrotraIpm(lcp.M, lcp.q, 0, params.magTol,
                                  params.iterMax, params.iterFreq,
                                  ipmParams, params.logger,
                                  NewtonSolverFactory{});
        }
      }
      else if ("ssn" == params.engine) {
        const VIModel lcpModel = makeVIModel(
            0, lcp.M.rows(),
            [&lcp](const VectorXd& v) -> VectorXd { return lcp.M * v + lcp.q; });
        SemismoothNewtonParams ssnParams;
        ssnParams.magTol = params.magTol;
        ssnParams.iterMax = params.iterMax;
        ssnParams.iterFreq = params.iterFreq;
        ssnParams.jacobian =
            [&lcp](const VectorXd&) -> MatrixXd { return lcp.M; };
        result.vi = semismoothNewtonSolve(lcpModel, z0, ssnParams,
                                          params.logger);
      }
      else {
        result.vi = bsHe94b(z0, lcp.M, lcp.q, projectNonnegative,
                            params.magTol, params.iterMax, params.iterFreq,
                            BsHe94bParams{}, params.logger);
      }
      if (params.roundEndLogger) {
        const auto solveStop = std::chrono::steady_clock::now();
        const double milliseconds =
            std::chrono::duration<double, std::milli>(solveStop - solveStart)
                .count();
        params.roundEndLogger(round, result.vi, milliseconds);
      }
      result.certificateRounds = round;
      if (!result.vi.converged) {
        break;                       // honest return; certifiedP stays false
      }
      if (!screenedP) {
        result.certifiedP = true;
        break;
      }
      if (!addFleetCertificateViolationsP(scaled, lcp, result.vi.z, epsilon,
                                          params.certificateSlack, reduced)) {
        result.certifiedP = true;
        break;
      }
    }
    result.keptPairs = lcp.numVars;

    // Unpack in scaled units, then rescale to real units.
    result.plan = unpackFleetLcp(scaled, reduced, lcp, result.vi.z);
    result.plan.supplied *= unitScale;
    result.plan.resupply *= unitScale;
    for (MatrixXd& x : result.plan.flow) {
      x *= unitScale;
    }
    for (MatrixXd& u : result.plan.vehicles) {
      u *= unitScale;
    }
    result.shortfall = fleetShortfallObjective(inst, result.plan);
    result.milesUsed = VectorXd(numK);
    result.budgetShadowPrice = VectorXd(numK);
    const Index laBase = lcp.numVars + lcp.numSupplyCells;
    for (Index k = 0; k < numK; ++k) {
      result.milesUsed(k) = vehicleMiles(inst, result.plan, k);
      result.budgetShadowPrice(k) =
          std::max(result.vi.z(laBase + k), 0.0) / (unitScale * mileScale);
    }
    return result;
  }

} // namespace VINCP::Network
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------

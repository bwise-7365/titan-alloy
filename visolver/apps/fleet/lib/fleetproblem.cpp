// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
// Fleet problem class implementation: map the app params onto FleetSolveParams,
// run the library QP, and delegate greedy / swap / sparsify to vimcpnet.
// ----------------------------------------------
#include "fleetproblem.hpp"

#include <stdexcept>
#include <utility>

namespace VIMCP::App {

  namespace {

    // Map the shared engine enum onto the FleetSolveParams engine string. Fleet
    // honors only the orthant-structural engines; Chain / Auto belong to SAOE.
    std::string
    fleetEngineString(ProblemBase::Engine engine)
    {
      switch (engine) {
        case ProblemBase::Engine::Default:
        case ProblemBase::Engine::Ipm:
          return "ipm";
        case ProblemBase::Engine::Bshe94b:
          return "bshe94b";
        case ProblemBase::Engine::Ssn:
          return "ssn";
        default:
          throw std::invalid_argument(
              std::string("Fleet::solve: engine ") + engineName(engine)
              + " is not honored; use one of: "
              + engineTokenList(Fleet::honoredEngines()) + ".");
      }
    }

  } // namespace

  Fleet::Fleet(Network::FleetInstance instance)
    : data(std::move(instance))
  {
    return;
  }

  const std::vector<ProblemBase::Engine>&
  Fleet::honoredEngines()
  {
    static const std::vector<ProblemBase::Engine> engines = {
        ProblemBase::Engine::Ipm,
        ProblemBase::Engine::Bshe94b,
        ProblemBase::Engine::Ssn,
    };
    return engines;
  }

  Network::FleetInstance
  Fleet::generate(const Network::FleetProfile& profile, std::uint64_t seed)
  {
    return Network::makeRandomFleetInstance(profile, seed);
  }

  Fleet::Solution
  Fleet::solve(const Params& params) const
  {
    Network::FleetSolveParams sp;
    sp.engine               = fleetEngineString(params.engine);
    sp.ipmNewton            = params.ipmNewton;
    sp.magTol               = params.magTol;
    sp.iterMax              = params.iterMax;
    sp.maxSourcesPerSink    = params.maxSourcesPerSink;
    sp.gapFraction          = params.gapFraction;
    sp.maxCertificateRounds = params.maxCertificateRounds;
    sp.certificateSlack     = params.certificateSlack;
    sp.epsilon              = params.epsilon;

    const Network::FleetSolveResult r = Network::solveFleetPlan(data, sp);

    FleetResult result;
    result.plan              = r.plan;
    result.shortfall         = r.shortfall;
    result.milesUsed         = r.milesUsed;
    result.budgetShadowPrice = r.budgetShadowPrice;
    result.certifiedP        = r.certifiedP;
    result.converged         = r.vi.converged;
    result.keptPairs         = r.keptPairs;
    result.totalPairs        = r.totalPairs;
    result.certificateRounds = r.certificateRounds;

    return Solution{ r.vi, result };
  }

  Network::FleetGreedyResult
  Fleet::greedyPlan(const Network::FleetGreedyParams& params) const
  {
    return Network::greedyFleetPlan(data, params);
  }

  Network::FleetSwapSummary
  Fleet::swapToLocalOptimum(Network::FleetPlan& plan, int maxSwapsPerAsset) const
  {
    return Network::swapFleetToLocalOptimum(data, plan, maxSwapsPerAsset);
  }

  Network::FleetPurifySummary
  Fleet::sparsify(Network::FleetPlan& plan, int maxSwapsPerAsset) const
  {
    return Network::purifyFleetPlan(data, plan, maxSwapsPerAsset);
  }

  FleetResult
  Fleet::sparsify(const FleetResult& result) const
  {
    FleetResult out = result;              // carries shortfall + solve metadata
    Network::FleetPlan plan = result.plan;
    sparsify(plan);                        // the plan-level overload (purify)
    out.plan = plan;
    // Deliveries are invariant under purification, so shortfall is unchanged;
    // only the vehicle-miles fall, so recompute them. The dual/solve metadata
    // (budgetShadowPrice, certifiedP, converged, kept/totalPairs,
    // certificateRounds) describe the QP solve and carry over as-is.
    const Index numTypes = Network::numVehicleTypes(data);
    out.milesUsed.resize(numTypes);
    for (Index k = 0; k < numTypes; ++k) {
      out.milesUsed(k) = Network::vehicleMiles(data, plan, k);
    }
    return out;
  }

} // namespace VIMCP::App
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------

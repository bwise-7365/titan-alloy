// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
// fleet_benchmark: instrumented probe of the fleet optimizer (stage FP0 in
// 2026-07-08-fleet-performance-plan.md). Reproduces the fleet_viewer default
// profile, prints the instance shape, and runs solveFleetPlan with flushed
// per-iteration heartbeats and per-round rows, so a run killed mid-round
// still yields the dimension and per-iteration cost it was attempting. Run
// in a RELEASE build for meaningful timings.
// ----------------------------------------------
#include "fleetinstance.hpp"
#include "fleetsolve.hpp"

#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <exception>
#include <iomanip>
#include <iostream>

using namespace VIMCP;
using namespace VIMCP::Network;
using std::cout;

namespace {

  // The fleet_viewer defaults (gui/fleetmainwindow.cpp): 20 supply-only /
  // 20 both / 30 demand-only / 0 transit, catalogs 4 assets x 3 vehicle
  // types, seed 20260704, limited fleet. Laydown and the solver knobs are
  // command-line arguments so one binary covers the probe matrix.
  const int kDefaultLaydown = 1;              // banded: the reported case
  const int kDefaultCertRounds = 1;           // bounded probe (IP4 protocol)
  const int kDefaultScreen = 6;               // FleetSolveParams default
  const int kDefaultAssets = 4;
  const int kDefaultVehicles = 3;
  const std::uint64_t kDefaultSeed = 20260704;

  int
  argOr(int argc, char** argv, int position, int fallback)
  {
    if (position < argc) {
      return std::atoi(argv[position]);
    }
    return fallback;
  }

} // namespace

int
main(int argc, char** argv)
{
  cout << "fleet_benchmark (FP0 probe). Timings are meaningful in a RELEASE "
          "build only.\n"
          "usage: fleet_benchmark [laydown] [maxCertificateRounds] "
          "[maxSourcesPerSink] [assets] [vehicles] [seed] [ipmNewton]\n"
          "defaults:              "
       << kDefaultLaydown << " " << kDefaultCertRounds << " "
       << kDefaultScreen << " " << kDefaultAssets << " " << kDefaultVehicles
       << " " << kDefaultSeed << " dense"
       << "\nmaxSourcesPerSink 0 = keep-all (no screen, no certificate "
          "loop); ipmNewton 'fleet' = the structured factory (FN2). All "
          "output is flushed: a run killed mid-round still reports the "
          "round's dimension and per-iteration cost.\n";

  const int laydown = argOr(argc, argv, 1, kDefaultLaydown);
  const int certRounds = argOr(argc, argv, 2, kDefaultCertRounds);
  const int screenK = argOr(argc, argv, 3, kDefaultScreen);
  const int numAssetTypes = argOr(argc, argv, 4, kDefaultAssets);
  const int numVehicleKinds = argOr(argc, argv, 5, kDefaultVehicles);
  const std::uint64_t seed =
      (6 < argc) ? static_cast<std::uint64_t>(std::atoll(argv[6]))
                 : kDefaultSeed;
  const string ipmNewton = (7 < argc) ? argv[7] : "dense";

  try {
    FleetProfile profile;
    profile.geometry.numSupplyOnly = 20;
    profile.geometry.numBoth = 20;
    profile.geometry.numDemandOnly = 30;
    profile.geometry.numNeither = 0;
    profile.geometry.laydownType = laydown;
    profile.assets = assetCatalog(numAssetTypes);
    profile.vehicles = vehicleCatalog(numVehicleKinds);
    validateFleetProfile(profile);
    const FleetInstance inst = makeRandomFleetInstance(profile, seed);

    // Instance shape and the keep-all count the screen is up against.
    const Index numA = numAssets(inst);
    const Index numK = numVehicleTypes(inst);
    cout << "\ninstance: " << inst.numNodes << " nodes, " << numA
         << " assets x " << numK << " vehicle types, laydown " << laydown
         << ", seed " << seed << "\n";
    Index keepAllVars = 0;
    Index supplyCells = 0;
    for (Index a = 0; a < numA; ++a) {
      const Index sources =
          static_cast<Index>(fleetSourceNodes(inst, a).size());
      const Index sinks = static_cast<Index>(fleetSinkNodes(inst, a).size());
      Index capable = 0;
      for (Index k = 0; k < numK; ++k) {
        if (0.0 < unitCapacity(inst.assets[static_cast<size_t>(a)],
                               inst.vehicles[static_cast<size_t>(k)])) {
          ++capable;
        }
      }
      keepAllVars += sources * sinks * capable;
      supplyCells += sources;
      cout << "  asset " << a << " ("
           << inst.assets[static_cast<size_t>(a)].name << "): " << sources
           << " sources, " << sinks << " sinks, " << capable
           << " capable types\n";
    }
    cout << "  keep-all: " << keepAllVars << " y-variables, LCP dim "
         << (keepAllVars + supplyCells + numK) << "\n\n";

    FleetSolveParams params;
    params.maxCertificateRounds = certRounds;
    params.maxSourcesPerSink = screenK;
    params.ipmNewton = ipmNewton;
    params.iterFreq = 1;

    const auto runStart = std::chrono::steady_clock::now();
    const auto elapsedSeconds = [runStart]() -> double {
      return std::chrono::duration<double>(std::chrono::steady_clock::now()
                                           - runStart)
          .count();
    };
    params.logger = [&elapsedSeconds](int iter, int iterMax, double mag,
                                      double magTol) {
      cout << "    it " << std::setw(4) << iter << "/" << iterMax
           << "  mag " << std::scientific << std::setprecision(3) << mag
           << " (tol " << std::setprecision(1) << magTol << ")"
           << std::fixed << "  t " << std::setprecision(1)
           << elapsedSeconds() << " s" << std::defaultfloat << std::endl;
    };
    params.roundStartLogger = [&elapsedSeconds](int round, Index kept,
                                                Index dim) {
      cout << "round " << round << ": solving at dim " << dim << " (" << kept
           << " y-variables)  t " << std::fixed << std::setprecision(1)
           << elapsedSeconds() << " s" << std::defaultfloat << std::endl;
    };
    params.roundEndLogger = [](int round, const VIResult& vi,
                               double milliseconds) {
      cout << "round " << round << " done: iter " << vi.iter << ", "
           << (vi.converged ? "converged" : "NOT converged") << ", residual "
           << std::scientific << std::setprecision(3) << vi.residual
           << std::defaultfloat << ", " << std::fixed << std::setprecision(1)
           << milliseconds / 1000.0 << " s" << std::defaultfloat << std::endl;
    };

    const FleetSolveResult result = solveFleetPlan(inst, params);
    const double totalSeconds = elapsedSeconds();

    cout << "\nsummary: kept " << result.keptPairs << "/" << result.totalPairs
         << " (pair, type) variables, rounds " << result.certificateRounds
         << ", certified " << (result.certifiedP ? "yes" : "NO")
         << ", converged " << (result.vi.converged ? "yes" : "NO") << "\n"
         << std::fixed << "  shortfall " << std::setprecision(5)
         << result.shortfall << ", wall " << std::setprecision(1)
         << totalSeconds << " s\n"
         << std::defaultfloat;
    for (Index k = 0; k < numVehicleTypes(inst); ++k) {
      cout << "  type " << k << " ("
           << inst.vehicles[static_cast<size_t>(k)].name << "): miles "
           << std::fixed << std::setprecision(1) << result.milesUsed(k)
           << " / budget " << vehicleBudget(inst, k) << ", lambda "
           << std::scientific << std::setprecision(2)
           << result.budgetShadowPrice(k) << std::defaultfloat << "\n";
    }
  }
  catch (const std::exception& problem) {
    cout << "ERROR: " << problem.what() << "\n";
    return 1;
  }
  return 0;
}
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------

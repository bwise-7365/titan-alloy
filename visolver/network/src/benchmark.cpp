// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
// network_benchmark: scale/performance sweep of the flow-planning solver
// (task C5). Prints one table per configuration; run in a RELEASE build for
// meaningful timings. Results are recorded in doc/performance.md.
// ----------------------------------------------
#include "config.hpp"
#include "flowplan.hpp"
#include "greedy.hpp"

#include <chrono>
#include <cstdint>
#include <exception>
#include <iomanip>
#include <iostream>

using namespace VINCP;
using namespace VINCP::Network;
using std::cout;

namespace {

  const std::uint64_t kSeedBase = 20260704;
  const int kInstancesPerConfig = 5;

  struct ConfigTotals {
    double milliseconds = 0.0;
    double shortfall = 0.0;
    double ration = 0.0;
    double deliveredFraction = 0.0;
    double tonMileFraction = 0.0;
    int certified = 0;
  };

  double
  sumSinkPriorities(const Instance& inst)
  {
    double total = 0.0;
    for (Index i = 0; i < inst.numNodes; ++i) {
      if (0.0 < inst.demand(i)) {
        total += inst.priority(i);
      }
    }
    return total;
  }

  // One configuration: kInstancesPerConfig random instances, greedy
  // calibration at 80%, then the production solve. Columns:
  //   kept/total  pairs in the final solve vs all source-sink pairs
  //   rnds        R3 certificate re-solve rounds
  //   iter        bsHe94b iterations of the final solve
  //   ms          solveFlowPlan wall time (includes FW, assembly, unpack)
  //   th_ration   budget-unconstrained lower bound
  //   th_star     achieved shortfall
  //   dlv         delivered / rationed-target tons
  //   tm/tmG      optimizer ton-miles / greedy ton-miles (<= 0.80 by budget;
  //               below 0.80 means the budget was not even scarce)
  //   lambda      budget shadow price (real units; 0 = slack budget)
  void
  runConfig(const string& name, const InstanceProfile& profile,
            const FlowPlanParams& params,
            int instanceCount = kInstancesPerConfig,
            std::uint64_t seedBase = kSeedBase)
  {
    cout << "\n== " << name << " ==\n";
    cout << "  seed        kept/total rnds   iter       ms  th_ration"
            "    th_star    dlv   tm/tmG      lambda  cert\n";

    ConfigTotals totals;
    for (int k = 0; k < instanceCount; ++k) {
      const std::uint64_t seed = seedBase + static_cast<std::uint64_t>(k);
      Instance inst = makeRandomInstance(profile, seed);
      const GreedyResult greedy = greedyPlan(inst);
      inst.tonMileLimit = greedy.suggestedLimit;
      const VectorXd targets = greedy.targets;

      const auto start = std::chrono::steady_clock::now();
      const FlowPlanResult result = solveFlowPlan(inst, params);
      const auto stop = std::chrono::steady_clock::now();
      const double ms =
          std::chrono::duration<double, std::milli>(stop - start).count();

      const double thetaRation = shortfallOfResupply(inst, targets);
      const double delivered =
          result.plan.resupply.sum() / targets.sum();
      const double tonMileFraction =
          result.tonMilesUsed / greedy.tonMilesUsed;

      cout << std::setw(6) << seed << "  "
           << std::setw(6) << result.keptPairs << "/"
           << std::setw(5) << result.totalPairs
           << std::setw(5) << result.certificateRounds
           << std::setw(7) << result.vi.iter
           << std::fixed
           << std::setw(9) << std::setprecision(1) << ms
           << std::setw(11) << std::setprecision(5) << thetaRation
           << std::setw(11) << std::setprecision(5) << result.shortfall
           << std::setw(7) << std::setprecision(3) << delivered
           << std::setw(9) << std::setprecision(3) << tonMileFraction
           << std::scientific
           << std::setw(12) << std::setprecision(2)
           << result.budgetShadowPrice
           << std::defaultfloat
           << (result.certifiedP ? "   yes" : "    NO") << "\n";

      totals.milliseconds += ms;
      totals.shortfall += result.shortfall;
      totals.ration += thetaRation;
      totals.deliveredFraction += delivered;
      totals.tonMileFraction += tonMileFraction;
      totals.certified += result.certifiedP ? 1 : 0;
    }

    const double n = static_cast<double>(instanceCount);
    cout << std::fixed
         << "  mean:            ms " << std::setprecision(1)
         << totals.milliseconds / n
         << "   th_ration " << std::setprecision(5) << totals.ration / n
         << "   th_star " << std::setprecision(5) << totals.shortfall / n
         << "   dlv " << std::setprecision(3)
         << totals.deliveredFraction / n
         << "   tm/tmG " << std::setprecision(3)
         << totals.tonMileFraction / n
         << "   certified " << totals.certified << "/"
         << instanceCount << "\n"
         << std::defaultfloat;
    return;
  }

} // namespace

// With a config-file argument (task E1), run exactly ONE configuration
// described by the file: profile.* keys shape the instances, solver.* and
// screen.* keys tune the solve, and benchmark.name / benchmark.instances /
// benchmark.seedBase control the sweep. No recompilation. Without an
// argument, run the built-in C5 sweep.
int
main(int argc, char** argv)
{
  cout << "network_benchmark (tasks C5/E1). Timings are meaningful in a "
          "RELEASE build only.\n";

  if (1 < argc) {
    try {
      ConfigEntries entries = parseConfigFile(argv[1]);
      InstanceProfile profile;
      FlowPlanParams params;
      string name = argv[1];
      int instances = kInstancesPerConfig;
      std::uint64_t seedBase = kSeedBase;
      applyProfileConfig(entries, profile);
      applyFlowPlanConfig(entries, params);
      consumeString(entries, "benchmark.name", name);
      consumeInt(entries, "benchmark.instances", instances);
      consumeUint64(entries, "benchmark.seedBase", seedBase);
      requireAllConsumed(entries);
      runConfig(name, profile, params, instances, seedBase);
    }
    catch (const std::exception& problem) {
      cout << "ERROR: " << problem.what() << "\n";
      return 1;
    }
    return 0;
  }

  // Iteration cap sized so a non-converging configuration finishes and
  // prints an honest cert=NO row instead of grinding for half an hour.
  // (Finding, 2026-07-04: the KEEP-ALL 70-node system -- 2041 unknowns, ~95%
  // of them worthless long-haul pairs -- makes projection-contraction crawl;
  // the R3 screen is an iteration-count necessity, not just memory relief.
  // Keep-all is therefore benchmarked only at 26 nodes, as the exact
  // reference beside its screened twin.)
  FlowPlanParams keepAll;                    // exact, no screen
  keepAll.iterMax = 150000;
  FlowPlanParams screen3 = keepAll;
  screen3.maxSourcesPerSink = 3;
  FlowPlanParams screen10 = keepAll;
  screen10.maxSourcesPerSink = 10;

  InstanceProfile mid26;                     // 26 nodes: keep-all affordable
  mid26.numSupplyOnly = 8;
  mid26.numBoth = 6;
  mid26.numDemandOnly = 10;
  mid26.numNeither = 2;
  runConfig("26 nodes, laydown 0, keep-all (exact reference)", mid26,
            keepAll);
  runConfig("26 nodes, laydown 0, screen k=3 + certificate (cross-check)",
            mid26, screen3);

  InstanceProfile spec70;                    // the spec's 70-node example
  spec70.numNeither = 2;
  runConfig("70 nodes, laydown 0, screen k=10 + certificate", spec70,
            screen10);

  InstanceProfile alt70 = spec70;
  alt70.laydownType = 1;
  runConfig("70 nodes, laydown 1 (banded), screen k=10", alt70, screen10);

  InstanceProfile big200;                    // the 200-node stress case
  big200.numSupplyOnly = 50;
  big200.numBoth = 50;
  big200.numDemandOnly = 95;
  big200.numNeither = 5;
  runConfig("200 nodes, laydown 0, screen k=10 + certificate", big200,
            screen10);

  // The realistic stress combination: 200 nodes AND the banded laydown.
  // Near-tied banded costs defeat a count screen with an exact-grade slack
  // (performance.md P4: the certificate buys near-worthless pairs in
  // expensive installments), so this config runs with certificateSlack
  // 1e-4 (scaled units): certified BOUNDED suboptimality — forgone
  // improvement <= slack x total deliverable tons, negligible against the
  // theta ~ 40+ rationing floor at this size. Three seeds: instances may
  // take minutes each.
  InstanceProfile big200Alt = big200;
  big200Alt.laydownType = 1;
  FlowPlanParams screen10Loose = screen10;
  screen10Loose.certificateSlack = 1.0e-4;
  const int kBig200AltSeeds = 3;
  runConfig("200 nodes, laydown 1 (banded), screen k=10, slack 1e-4",
            big200Alt, screen10Loose, kBig200AltSeeds);

  cout << "\nDone. Paste this output into the C5 gate for "
          "doc/performance.md.\n";
  return 0;
}
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------

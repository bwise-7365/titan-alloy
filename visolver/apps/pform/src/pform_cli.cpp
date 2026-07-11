// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
// pform_cli: a CLI app for the PFORM parliament-formation model. Reads an instance
// from a limited-subset GMS file (or generates one randomly), runs the PForm /
// SAOE solve, and prints the parliament supports and probabilities. All report
// text comes from the shared pformtext renderers, so pform_gui shows the same.
// ----------------------------------------------
#include "pformgms.hpp"
#include "pformproblem.hpp"
#include "pformtext.hpp"

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <exception>
#include <stdexcept>
#include <string>
#include <vector>

using namespace VINCP;
using namespace VINCP::App;

namespace {

  const char* const kAppTag = "pform_cli";

  void
  printUsage()
  {
    const std::string engines = engineTokenList(SAOE::honoredEngines());
    std::fprintf(stderr,
                 "usage: pform_cli [--engine %s] <file.gms>\n"
                 "       pform_cli [--engine %s] --random [seed] [parties] [issues]\n",
                 engines.c_str(), engines.c_str());
    return;
  }

} // namespace

int
main(int argc, char** argv)
{
  std::vector<std::string> args(argv + 1, argv + argc);
  ProblemBase::Engine engine = ProblemBase::Engine::Default;

  try {
    // Pull an optional "--engine NAME" from anywhere in the argument list; the
    // rest are positional (the source and, for --random, its parameters).
    for (std::size_t i = 0; i < args.size();) {
      if ("--engine" == args[i]) {
        if (i + 1 >= args.size()) {
          printUsage();
          return 2;
        }
        engine = parseEngineToken(args[i + 1]);
        if (!engineIsHonored(SAOE::honoredEngines(), engine)) {
          throw std::invalid_argument(
              "engine '" + std::string(engineToken(engine))
              + "' is not honored by pform/SAOE; use one of: "
              + engineTokenList(SAOE::honoredEngines()) + ".");
        }
        args.erase(args.begin() + static_cast<std::ptrdiff_t>(i),
                   args.begin() + static_cast<std::ptrdiff_t>(i + 2));
      }
      else {
        ++i;
      }
    }
    if (args.empty()) {
      printUsage();
      return 2;
    }

    PformInstance in;
    const std::string& first = args[0];
    if ("--random" == first) {
      PformRandomSpec spec;
      std::uint64_t seed = 0;
      if (args.size() > 1) {
        seed = std::stoull(args[1]);
      }
      if (args.size() > 2) {
        spec.numParties = std::stoi(args[2]);
      }
      if (args.size() > 3) {
        spec.numIssues = std::stoi(args[3]);
      }
      // A seed of 0 means "pick one for me": draw a non-zero seed and report it
      // so the run is reproducible (as the fleet/network viewers do).
      seed = pformResolveSeed(seed);
      spec.seed = seed;
      in.data = PForm::generate(spec);
      in.unselectedProb = 0.05;
      in.partyLabels = pformDefaultLabels("P", spec.numParties);
      in.issueLabels = pformDefaultLabels("I", spec.numIssues);
      in.randomP = true;
      in.seed = seed;
    }
    else {
      const PformGmsInput gms = readPformGms(first);
      in.data = gms.data;
      in.unselectedProb = gms.unselectedProb;
      in.partyLabels = gms.partyLabels;
      in.issueLabels = gms.issueLabels;
    }

    std::fputs(renderPformInputs(in, kAppTag).c_str(), stdout);
    const PForm problem(in.data);
    PformParams params;
    params.unselectedProb = in.unselectedProb;
    params.engine = engine;   // --engine, or Default (the SAOE chain)

    // with interior solvers, this will likely have lots of small efforts
    // because it will be in the center of a face.
    const auto [vi, res] = problem.solve(params);
    std::fputs(renderPformResult(in, params, vi, res, kAppTag).c_str(), stdout);

    // Extract and print the coalition structure from the RAW result (the even
    // split over each coalition is the signal; sparsify would erase it).
    const Index nParties = in.data.weight.size();
    const Index nIssues  = in.data.position.rows();
    std::fputs(
        renderPformCoalitions(in, pformCoalitions(res, nParties, nIssues)).c_str(),
        stdout);

    // This will not change the result of an edge-following solver,
    // but it will sometimes simplify that of an interior-path solver.
    // Not often for PForm.
    //printf("Sparsified result ... \n");
    //const PformResult sparse = problem.sparsify(res);
    //std::fputs(renderPformResult(in, params, vi, sparse, kAppTag).c_str(), stdout);

    return vi.converged ? 0 : 1;
  }
  catch (const std::exception& e) {
    std::fprintf(stderr, "pform_cli: %s\n", e.what());
    return 2;
  }
}
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------

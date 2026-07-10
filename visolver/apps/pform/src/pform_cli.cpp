// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
// pform_cli: a CLI app for the PFORM parliament-formation model. Reads an instance
// from a limited-subset GMS file (or generates one randomly), runs the PForm /
// SAOE solve, and prints the parliament supports and probabilities.
// ----------------------------------------------
#include "pformgms.hpp"
#include "pformproblem.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <exception>
#include <random>
#include <stdexcept>
#include <string>
#include <vector>

using namespace VINCP;
using namespace VINCP::App;

namespace {

  struct CliInstance {
    PformData data;
    double    unselectedProb = 0.05;
    std::vector<std::string> partyLabels;
    std::vector<std::string> issueLabels;
    bool          randomP = false;   // generated (vs read from a GMS file)
    std::uint64_t seed = 0;          // the PRNG seed actually used (random mode)
  };

  std::vector<std::string>
  defaultLabels(const char* prefix, Index n)
  {
    std::vector<std::string> labels;
    for (Index i = 0; i < n; ++i) {
      labels.push_back(std::string(prefix) + std::to_string(i));
    }
    return labels;
  }

  // Render a parliament's matching as its controlling-party labels per issue.
  std::string
  matchingText(Index k, Index m, Index d, const std::vector<std::string>& partyLabels)
  {
    const std::vector<Index> f = pformMatching(k, m, d);
    std::string out = "[";
    for (std::size_t i = 0; i < f.size(); ++i) {
      if (0 < i) {
        out += " ";
      }
      out += partyLabels[static_cast<std::size_t>(f[i])];
    }
    out += "]";
    return out;
  }

  // Render a coalition pattern: a pinned issue shows its controlling party, a
  // free issue shows '*'. Every cell is right-justified to the widest party
  // label so the columns line up under matchingText's, e.g. [P0  * P1 P0 P1].
  std::string
  patternText(const std::vector<Index>& pattern,
              const std::vector<std::string>& partyLabels)
  {
    std::size_t width = 1;
    for (const std::string& label : partyLabels) {
      width = std::max(width, label.size());
    }
    std::string out = "[";
    for (std::size_t i = 0; i < pattern.size(); ++i) {
      if (0 < i) {
        out += " ";
      }
      const std::string cell =
          (kFreeIssue == pattern[i])
              ? std::string("*")
              : partyLabels[static_cast<std::size_t>(pattern[i])];
      out += std::string(width - cell.size(), ' ') + cell;
    }
    out += "]";
    return out;
  }

  // Spreadsheet-style coalition label: A..Z, then AA, AB, ... for the (rare)
  // case of more than 26 coalitions.
  std::string
  coalitionLabel(std::size_t index)
  {
    std::string out;
    std::size_t n = index;
    for (;;) {
      out.insert(out.begin(), static_cast<char>('A' + static_cast<int>(n % 26)));
      if (n < 26) {
        break;
      }
      n = n / 26 - 1;
    }
    return out;
  }

  // Echo the instance that was read/generated: the weight vector and the
  // position and salience matrices (issues x parties), all with their labels.
  void
  printInputs(const CliInstance& in)
  {
    const Index M = in.data.weight.size();
    const Index D = in.data.position.rows();

    std::printf("=== PFORM instance (pform_cli) ===\n");
    if (in.randomP) {
      std::printf("Random instance, PRNG seed = %llu\n",
                  static_cast<unsigned long long>(in.seed));
    }

    std::printf("Party weights:\n");
    for (Index m = 0; m < M; ++m) {
      std::printf("  %-8s %8.2f\n",
                  in.partyLabels[static_cast<std::size_t>(m)].c_str(), in.data.weight(m));
    }

    const auto printMatrix = [&](const char* title, const MatrixXd& matrix) {
      std::printf("\n%s (issues x parties):\n", title);
      std::printf("  %-8s", "issue");
      for (Index m = 0; m < M; ++m) {
        std::printf(" %8s", in.partyLabels[static_cast<std::size_t>(m)].c_str());
      }
      std::printf("\n");
      for (Index d = 0; d < D; ++d) {
        std::printf("  %-8s", in.issueLabels[static_cast<std::size_t>(d)].c_str());
        for (Index m = 0; m < M; ++m) {
          std::printf(" %8.3f", matrix(d, m));
        }
        std::printf("\n");
      }
    };
    printMatrix("Preferred positions", in.data.position);
    printMatrix("Saliences", in.data.salience);
    std::printf("\n");
    return;
  }

  void
  printResult(const CliInstance& in, const PformParams& params,
              const VIResult& vi, const PformResult& res)
  {
    const Index M = in.data.weight.size();
    const Index D = in.data.position.rows();
    const Index K = res.probabilities.size();

    std::printf("=== PFORM result (pform_cli) ===\n");
    std::printf("Instance: %lld parties, %lld issues, K = %lld parliaments\n",
                static_cast<long long>(M), static_cast<long long>(D),
                static_cast<long long>(K));
    std::printf("unselectedProb q = %.4f  (derived effort floor eps = %.4e)\n",
                params.unselectedProb, res.epsilon);
    // Resolve Default to SAOE's actual default engine for the display.
    const ProblemBase::Engine shownEngine =
        (ProblemBase::Engine::Default == params.engine)
            ? SAOE::honoredEngines().front()
            : params.engine;
    std::printf("Engine: %s%s\n", engineName(shownEngine),
                (ProblemBase::Engine::Default == params.engine) ? " [default]" : "");
    std::printf("Solver: converged = %s, residual^2 = %.3e, "
                "outer iters = %d, inner iters = %d\n\n",
                vi.converged ? "true" : "false", vi.residual, vi.iter,
                vi.innerIters);

    const Index kStar = res.deterministic;
    std::printf("Deterministic (Central Position) parliament: k = %lld  %s\n",
                static_cast<long long>(kStar),
                matchingText(kStar, M, D, in.partyLabels).c_str());
    std::printf("  eta = %.4f, phi = %.4f\n\n", res.eta(kStar), res.phi(kStar));

    // Active parliaments: those carrying effort from at least one party.
    const double maxEffort = (0 < res.effort.size()) ? res.effort.maxCoeff() : 0.0;
    const double threshold = std::max(1.0e-9, 1.0e-6 * maxEffort);
    std::vector<Index> active;
    for (Index k = 0; k < K; ++k) {
      if (threshold < res.effort.col(k).sum()) {
        active.push_back(k);
      }
    }
    std::sort(active.begin(), active.end(),
              [&res](Index a, Index b) { return res.probabilities(a) > res.probabilities(b); });

    std::printf("Supported parliaments (non-zero total effort, most likely first;\n"
                "'*' marks the deterministic parliament):\n");
    std::printf("  %-5s %-8s %-22s", "k", "prob", "matching");
    for (Index m = 0; m < M; ++m) {
      std::printf(" %8s", in.partyLabels[static_cast<std::size_t>(m)].c_str());
    }
    std::printf(" %10s\n", "total");
    for (const Index k : active) {
      const char marker = (k == kStar) ? '*' : ' ';
      std::printf("%c %-4lld %-8.4f %-22s", marker, static_cast<long long>(k),
                  res.probabilities(k), matchingText(k, M, D, in.partyLabels).c_str());
      for (Index m = 0; m < M; ++m) {
        std::printf(" %8.2f", res.effort(m, k));
      }
      std::printf(" %10.2f\n", res.effort.col(k).sum());
    }

    std::printf("\nParty expected utilities:\n");
    for (Index m = 0; m < M; ++m) {
      std::printf("  %-8s %8.4f\n",
                  in.partyLabels[static_cast<std::size_t>(m)].c_str(), res.utilities(m));
    }

    if (!vi.converged) {
      std::printf("\nWARNING: the SAOE solve did not converge to tolerance; the "
                  "reported point is the best visited.\n");
    }
    return;
  }

  // Print the coalition structure extracted from the (raw) result, then a
  // per-party breakdown of how each party split its effort across coalitions.
  void
  printCoalitions(const CliInstance& in, const std::vector<PformCoalition>& coalitions)
  {
    std::printf(
        "\nCoalition structure (parliaments grouped by identical party support;\n"
        "'*' marks an issue whose controlling party is free within the coalition):\n");
    if (coalitions.empty()) {
      std::printf("  (no supported parliaments)\n");
      return;
    }

    // Pre-render the variable-width text columns so they can be sized to fit.
    std::vector<std::string> ids, parties, patterns, contribs;
    std::size_t idW = 2, partyW = 7, patW = 7, conW = 13;
    for (std::size_t g = 0; g < coalitions.size(); ++g) {
      const PformCoalition& c = coalitions[g];
      ids.push_back(coalitionLabel(g));
      std::string ps;
      std::string cs;
      for (std::size_t i = 0; i < c.members.size(); ++i) {
        const std::string& label = in.partyLabels[static_cast<std::size_t>(c.members[i])];
        if (0 < i) {
          ps += ",";
          cs += " ";
        }
        ps += label;
        char buf[64];
        std::snprintf(buf, sizeof buf, "%s:%.2f", label.c_str(),
                      c.effortPer(static_cast<Index>(i)));
        cs += buf;
      }
      parties.push_back(ps);
      patterns.push_back(patternText(c.pattern, in.partyLabels));
      contribs.push_back(cs);
      idW    = std::max(idW, ids.back().size());
      partyW = std::max(partyW, parties.back().size());
      patW   = std::max(patW, patterns.back().size());
      conW   = std::max(conW, contribs.back().size());
    }

    std::printf("  %-*s %-*s %-*s %-*s %10s %6s %11s\n", static_cast<int>(idW), "id",
                static_cast<int>(partyW), "parties", static_cast<int>(patW), "pattern",
                static_cast<int>(conW), "contributions", "prob(each)", "seats",
                "prob(total)");
    for (std::size_t g = 0; g < coalitions.size(); ++g) {
      const PformCoalition& c = coalitions[g];
      std::printf("  %-*s %-*s %-*s %-*s %10.4f %6lld %11.4f%s\n",
                  static_cast<int>(idW), ids[g].c_str(), static_cast<int>(partyW),
                  parties[g].c_str(), static_cast<int>(patW), patterns[g].c_str(),
                  static_cast<int>(conW), contribs[g].c_str(), c.probEach,
                  static_cast<long long>(c.parliaments.size()), c.probTotal,
                  c.regularP ? "" : "  (irregular)");
    }

    // Party vote split: how each party divided its budget across coalitions.
    std::printf(
        "\nParty vote split (effort committed to each coalition; xN = N seats):\n");
    const Index M = in.data.weight.size();
    for (Index m = 0; m < M; ++m) {
      std::string line;
      for (std::size_t g = 0; g < coalitions.size(); ++g) {
        const PformCoalition& c = coalitions[g];
        for (std::size_t i = 0; i < c.members.size(); ++i) {
          if (c.members[i] == m) {
            char buf[64];
            std::snprintf(buf, sizeof buf, "  %s:%.2fx%lld", ids[g].c_str(),
                          c.effortPer(static_cast<Index>(i)),
                          static_cast<long long>(c.parliaments.size()));
            line += buf;
            break;
          }
        }
      }
      if (line.empty()) {
        line = "  (no coalition)";
      }
      std::printf("  %-8s ->%s   (weight %.2f)\n",
                  in.partyLabels[static_cast<std::size_t>(m)].c_str(), line.c_str(),
                  in.data.weight(m));
    }
    return;
  }

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

    CliInstance in;
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
      if (0 == seed) {
        std::random_device rd;
        seed = (static_cast<std::uint64_t>(rd()) << 32)
               ^ static_cast<std::uint64_t>(rd());
        if (0 == seed) {
          seed = 1;
        }
      }
      spec.seed = seed;
      in.data = PForm::generate(spec);
      in.unselectedProb = 0.05;
      in.partyLabels = defaultLabels("P", spec.numParties);
      in.issueLabels = defaultLabels("I", spec.numIssues);
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

    printInputs(in);
    const PForm problem(in.data);
    PformParams params;
    params.unselectedProb = in.unselectedProb;
    params.engine = engine;   // --engine, or Default (the SAOE chain)

    // with interior solvers, this will likely have lots of small efforts
    // because it will be in the center of a face.
    const auto [vi, res] = problem.solve(params);
    printResult(in, params, vi, res);

    // Extract and print the coalition structure from the RAW result (the even
    // split over each coalition is the signal; sparsify would erase it).
    const Index nParties = in.data.weight.size();
    const Index nIssues  = in.data.position.rows();
    printCoalitions(in, pformCoalitions(res, nParties, nIssues));

    // This will not change the result of an edge-following solver,
    // but it will sometimes simplify that of an interior-path solver.
    // Not often for PForm.
    //printf("Sparsified result ... \n");
    //const PformResult sparse = problem.sparsify(res);
    //printResult(in, params, vi, sparse);

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

// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
// pmatrix: a CLI app that reads an SAOE instance from a limited-subset GMS
// file, solves the Nash equilibrium via the SAOE class, and prints the answer.
// ----------------------------------------------
#include "pmatrixgms.hpp"
#include "saoeproblem.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdio>
#include <exception>
#include <vector>

using namespace VINCP;
using namespace VINCP::App;

namespace {

  // Echo the instance that was read: the full reward matrix and the actor
  // weights, both with their GMS labels.
  void
  printInputs(const PmatrixInput& in)
  {
    const Index M = in.R.rows();
    const Index N = in.R.cols();

    std::printf("=== SAOE instance (pmatrix) ===\n");
    std::printf("Reward matrix (actors x options):\n");
    std::printf("  %-8s", "actor");
    for (Index j = 0; j < N; ++j) {
      std::printf(" %10s", in.optionLabels[static_cast<std::size_t>(j)].c_str());
    }
    std::printf("\n");
    for (Index i = 0; i < M; ++i) {
      std::printf("  %-8s", in.actorLabels[static_cast<std::size_t>(i)].c_str());
      for (Index j = 0; j < N; ++j) {
        std::printf(" %10.2f", in.R(i, j));
      }
      std::printf("\n");
    }

    std::printf("\nActor weights (strengths):\n");
    for (Index i = 0; i < M; ++i) {
      std::printf("  %-8s %10.2f\n",
                  in.actorLabels[static_cast<std::size_t>(i)].c_str(), in.S(i));
    }
    std::printf("\n");
    return;
  }

  // Print the decoded SAOE answer: the effort matrix (columns with zero effort
  // across every actor omitted), the option probabilities, per-actor expected
  // reward, and the solver telemetry.
  void
  printResult(const PmatrixInput& in, const SaoeParams& params,
              const VIResult& vi, const SaoeResult& res)
  {
    const Index M = res.e.rows();
    const Index N = res.e.cols();

    // Active options = columns carrying effort from at least one actor.
    const double maxEffort = (0 < res.e.size()) ? res.e.maxCoeff() : 0.0;
    const double activeThreshold = std::max(1.0e-9, 1.0e-6 * maxEffort);
    std::vector<Index> active;
    for (Index j = 0; j < N; ++j) {
      if (activeThreshold < res.e.col(j).sum()) {
        active.push_back(j);
      }
    }

    std::printf("=== SAOE result (pmatrix) ===\n");
    std::printf("%s\n\n", SAOE::answerContractNote());
    std::printf("Instance: %lld actors, %lld options, risk aversion a = %.3f\n",
                static_cast<long long>(M), static_cast<long long>(N),
                params.riskAversion);
    std::printf("Solver:   converged = %s, residual^2 = %.3e, "
                "outer iters = %d, inner iters = %d\n\n",
                vi.converged ? "true" : "false", vi.residual, vi.iter,
                vi.innerIters);

    // Efforts table.
    std::printf("Equilibrium efforts (zero-effort options omitted):\n");
    std::printf("  %-8s %10s", "actor", "strength");
    for (const Index j : active) {
      std::printf(" %10s", in.optionLabels[static_cast<std::size_t>(j)].c_str());
    }
    std::printf("\n");
    for (Index i = 0; i < M; ++i) {
      std::printf("  %-8s %10.2f",
                  in.actorLabels[static_cast<std::size_t>(i)].c_str(), in.S(i));
      for (const Index j : active) {
        std::printf(" %10.2f", res.e(i, j));
      }
      std::printf("\n");
    }

    // Option probabilities under the same active columns (pinned aggregate).
    std::printf("\nOption probabilities (active options; pinned at equilibrium):\n");
    std::printf("  %-19s", "probability");
    for (const Index j : active) {
      std::printf(" %10.4f", res.probabilities(j));
    }
    std::printf("\n");

    // Per-actor expected reward (pinned aggregate).
    std::printf("\nExpected reward per actor (pinned at equilibrium):\n");
    for (Index i = 0; i < M; ++i) {
      std::printf("  %-8s %12.4f\n",
                  in.actorLabels[static_cast<std::size_t>(i)].c_str(),
                  res.utilities(i));
    }

    if (!vi.converged) {
      std::printf("\nWARNING: the solver did not converge to tolerance; the "
                  "reported point is the best visited.\n");
    }
    return;
  }

} // namespace

int
main(int argc, char** argv)
{
  if (2 != argc) {
    std::fprintf(stderr, "usage: pmatrix <file.gms>\n");
    return 2;
  }

  try {
    const PmatrixInput in = readPmatrixGms(argv[1]);
    printInputs(in);
    const SAOE problem(SaoeData{ in.R, in.S });
    SaoeParams params;
    params.riskAversion = in.raFrac;   // engine defaults to the robust chain
    const auto [vi, res] = problem.solve(params);
    printResult(in, params, vi, res);
    return vi.converged ? 0 : 1;
  }
  catch (const std::exception& e) {
    std::fprintf(stderr, "pmatrix: %s\n", e.what());
    return 2;
  }
}
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------

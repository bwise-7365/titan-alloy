// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
// Shared text rendering for the PFORM front ends (see pformtext.hpp).
// ----------------------------------------------
#include "pformtext.hpp"

#include <algorithm>
#include <cstdarg>
#include <cstddef>
#include <cstdio>
#include <random>
#include <stdexcept>

namespace VINCP::App {

  namespace {

    // printf-into-a-string: keeps every format string from the original CLI
    // code verbatim, so the rendered text is byte-identical to what the CLI
    // printed before the extraction.
    void
    appendf(std::string& out, const char* format, ...)
    {
      char buffer[512];
      va_list args;
      va_start(args, format);
      const int n = std::vsnprintf(buffer, sizeof buffer, format, args);
      va_end(args);
      if (n < 0) {
        throw std::runtime_error("pformtext: vsnprintf failed.");
      }
      if (static_cast<std::size_t>(n) < sizeof buffer) {
        out += buffer;
        return;
      }
      // A line longer than the stack buffer: retry with an exact-size buffer.
      std::vector<char> big(static_cast<std::size_t>(n) + 1);
      va_start(args, format);
      std::vsnprintf(big.data(), big.size(), format, args);
      va_end(args);
      out += big.data();
      return;
    }

  } // namespace

  std::vector<std::string>
  pformDefaultLabels(const char* prefix, Index n)
  {
    std::vector<std::string> labels;
    for (Index i = 0; i < n; ++i) {
      labels.push_back(std::string(prefix) + std::to_string(i));
    }
    return labels;
  }

  std::uint64_t
  pformResolveSeed(std::uint64_t requested)
  {
    if (0 != requested) {
      return requested;
    }
    std::random_device rd;
    std::uint64_t seed = (static_cast<std::uint64_t>(rd()) << 32)
                         ^ static_cast<std::uint64_t>(rd());
    if (0 == seed) {
      seed = 1;
    }
    return seed;
  }

  std::string
  pformMatchingText(Index k, Index m, Index d,
                    const std::vector<std::string>& partyLabels)
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

  std::string
  pformPatternText(const std::vector<Index>& pattern,
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

  std::string
  pformCoalitionLabel(std::size_t index)
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

  std::string
  renderPformInputs(const PformInstance& in, const std::string& appTag)
  {
    const Index M = in.data.weight.size();
    const Index D = in.data.position.rows();
    std::string out;

    appendf(out, "=== PFORM instance (%s) ===\n", appTag.c_str());
    if (in.randomP) {
      appendf(out, "Random instance, PRNG seed = %llu\n",
              static_cast<unsigned long long>(in.seed));
    }

    appendf(out, "Party weights:\n");
    for (Index m = 0; m < M; ++m) {
      appendf(out, "  %-8s %8.2f\n",
              in.partyLabels[static_cast<std::size_t>(m)].c_str(),
              in.data.weight(m));
    }

    const auto appendMatrix = [&](const char* title, const MatrixXd& matrix) {
      appendf(out, "\n%s (issues x parties):\n", title);
      appendf(out, "  %-8s", "issue");
      for (Index m = 0; m < M; ++m) {
        appendf(out, " %8s", in.partyLabels[static_cast<std::size_t>(m)].c_str());
      }
      appendf(out, "\n");
      for (Index d = 0; d < D; ++d) {
        appendf(out, "  %-8s", in.issueLabels[static_cast<std::size_t>(d)].c_str());
        for (Index m = 0; m < M; ++m) {
          appendf(out, " %8.3f", matrix(d, m));
        }
        appendf(out, "\n");
      }
    };
    appendMatrix("Preferred positions", in.data.position);
    appendMatrix("Saliences", in.data.salience);
    appendf(out, "\n");
    return out;
  }

  std::string
  renderPformResult(const PformInstance& in, const PformParams& params,
                    const VIResult& vi, const PformResult& res,
                    const std::string& appTag)
  {
    const Index M = in.data.weight.size();
    const Index D = in.data.position.rows();
    const Index K = res.probabilities.size();
    std::string out;

    appendf(out, "=== PFORM result (%s) ===\n", appTag.c_str());
    appendf(out, "Instance: %lld parties, %lld issues, K = %lld parliaments\n",
            static_cast<long long>(M), static_cast<long long>(D),
            static_cast<long long>(K));
    appendf(out, "unselectedProb q = %.4f  (derived effort floor eps = %.4e)\n",
            params.unselectedProb, res.epsilon);
    // Resolve Default to SAOE's actual default engine for the display.
    const ProblemBase::Engine shownEngine =
        (ProblemBase::Engine::Default == params.engine)
            ? SAOE::honoredEngines().front()
            : params.engine;
    appendf(out, "Engine: %s%s\n", engineName(shownEngine),
            (ProblemBase::Engine::Default == params.engine) ? " [default]" : "");
    appendf(out, "Solver: converged = %s, residual^2 = %.3e, "
            "outer iters = %d, inner iters = %d\n\n",
            vi.converged ? "true" : "false", vi.residual, vi.iter,
            vi.innerIters);

    const Index kStar = res.deterministic;
    appendf(out, "Deterministic (Central Position) parliament: k = %lld  %s\n",
            static_cast<long long>(kStar),
            pformMatchingText(kStar, M, D, in.partyLabels).c_str());
    appendf(out, "  eta = %.4f, phi = %.4f\n\n", res.eta(kStar), res.phi(kStar));

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

    appendf(out, "Supported parliaments (non-zero total effort, most likely first;\n"
            "'*' marks the deterministic parliament):\n");
    appendf(out, "  %-5s %-8s %-22s", "k", "prob", "matching");
    for (Index m = 0; m < M; ++m) {
      appendf(out, " %8s", in.partyLabels[static_cast<std::size_t>(m)].c_str());
    }
    appendf(out, " %10s\n", "total");
    for (const Index k : active) {
      const char marker = (k == kStar) ? '*' : ' ';
      appendf(out, "%c %-4lld %-8.4f %-22s", marker, static_cast<long long>(k),
              res.probabilities(k),
              pformMatchingText(k, M, D, in.partyLabels).c_str());
      for (Index m = 0; m < M; ++m) {
        appendf(out, " %8.2f", res.effort(m, k));
      }
      appendf(out, " %10.2f\n", res.effort.col(k).sum());
    }

    appendf(out, "\nParty expected utilities:\n");
    for (Index m = 0; m < M; ++m) {
      appendf(out, "  %-8s %8.4f\n",
              in.partyLabels[static_cast<std::size_t>(m)].c_str(),
              res.utilities(m));
    }

    if (!vi.converged) {
      appendf(out, "\nWARNING: the SAOE solve did not converge to tolerance; the "
              "reported point is the best visited.\n");
    }
    return out;
  }

  std::string
  renderPformCoalitions(const PformInstance& in,
                        const std::vector<PformCoalition>& coalitions)
  {
    std::string out;
    appendf(out,
            "\nCoalition structure (parliaments grouped by identical party support;\n"
            "'*' marks an issue whose controlling party is free within the coalition):\n");
    if (coalitions.empty()) {
      appendf(out, "  (no supported parliaments)\n");
      return out;
    }

    // Pre-render the variable-width text columns so they can be sized to fit.
    std::vector<std::string> ids, parties, patterns, contribs;
    std::size_t idW = 2, partyW = 7, patW = 7, conW = 13;
    for (std::size_t g = 0; g < coalitions.size(); ++g) {
      const PformCoalition& c = coalitions[g];
      ids.push_back(pformCoalitionLabel(g));
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
      patterns.push_back(pformPatternText(c.pattern, in.partyLabels));
      contribs.push_back(cs);
      idW    = std::max(idW, ids.back().size());
      partyW = std::max(partyW, parties.back().size());
      patW   = std::max(patW, patterns.back().size());
      conW   = std::max(conW, contribs.back().size());
    }

    appendf(out, "  %-*s %-*s %-*s %-*s %10s %6s %11s\n", static_cast<int>(idW), "id",
            static_cast<int>(partyW), "parties", static_cast<int>(patW), "pattern",
            static_cast<int>(conW), "contributions", "prob(each)", "seats",
            "prob(total)");
    for (std::size_t g = 0; g < coalitions.size(); ++g) {
      const PformCoalition& c = coalitions[g];
      appendf(out, "  %-*s %-*s %-*s %-*s %10.4f %6lld %11.4f%s\n",
              static_cast<int>(idW), ids[g].c_str(), static_cast<int>(partyW),
              parties[g].c_str(), static_cast<int>(patW), patterns[g].c_str(),
              static_cast<int>(conW), contribs[g].c_str(), c.probEach,
              static_cast<long long>(c.parliaments.size()), c.probTotal,
              c.regularP ? "" : "  (irregular)");
    }

    // Party vote split: how each party divided its budget across coalitions.
    appendf(out,
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
      appendf(out, "  %-8s ->%s   (weight %.2f)\n",
              in.partyLabels[static_cast<std::size_t>(m)].c_str(), line.c_str(),
              in.data.weight(m));
    }
    return out;
  }

} // namespace VINCP::App
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------

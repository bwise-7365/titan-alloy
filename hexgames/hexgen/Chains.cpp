// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
#include "hexgen/Chains.h"

#include <cstddef>
#include <optional>

namespace HexGen {

  std::vector<std::vector<int>>
  chainsOf(const std::set<std::pair<int, int>>& steps, int hexCount)
  {
    const std::vector<std::pair<int, int>> edges(steps.begin(), steps.end());
    std::vector<std::vector<std::size_t>> incident(static_cast<std::size_t>(hexCount));
    for (std::size_t e = 0; e < edges.size(); ++e) {
      incident[static_cast<std::size_t>(edges[e].first)].push_back(e);
      incident[static_cast<std::size_t>(edges[e].second)].push_back(e);
    }
    const auto passThroughP = [&](int hex) { return 2 == incident[static_cast<std::size_t>(hex)].size(); };
    std::vector<bool> used(edges.size(), false);
    std::vector<std::vector<int>> chains;
    for (int pass = 0; pass < 2; ++pass) {
      for (int start = 0; start < hexCount; ++start) {
        if (0 == pass && passThroughP(start)) {
          continue;
        }
        for (const std::size_t first : incident[static_cast<std::size_t>(start)]) {
          if (used[first]) {
            continue;
          }
          std::vector<int> chain{start};
          int here = start;
          std::optional<std::size_t> next = first;
          while (next) {
            used[*next] = true;
            here = edges[*next].first == here ? edges[*next].second : edges[*next].first;
            chain.push_back(here);
            next.reset();
            if (passThroughP(here)) {
              for (const std::size_t e : incident[static_cast<std::size_t>(here)]) {
                if (!used[e]) {
                  next = e;
                }
              }
            }
          }
          chains.push_back(chain);
        }
      }
    }
    return chains;
  }

}  // namespace HexGen
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------

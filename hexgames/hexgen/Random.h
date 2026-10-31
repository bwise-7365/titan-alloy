// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
// The generator's random streams: one std::mt19937_64 per step, seeded mixSeed(seed, tag), so that a
// change to one step never shifts the draws of another. Draws are raw generator outputs turned into
// numbers here, never through <random> distributions, whose results differ between standard
// libraries (the rule of hexview/Buildings.h).
// ----------------------------------------------
#pragma once
#include "hexmodel/Seed.h"

#include <cstdint>
#include <random>
#include <utility>
#include <vector>

namespace HexGen {

  enum class StreamTag : std::uint8_t { Height, Cover, City, EdgeTop, EdgeBottom, EdgeLeft, EdgeRight };

  inline std::mt19937_64
  streamOf(std::uint64_t seed, StreamTag tag)
  {
    return std::mt19937_64(HexModel::mixSeed(seed, static_cast<std::uint64_t>(tag)));
  }

  // Uniform in [0, 1), from the top 53 bits of one draw.
  inline double
  uniform01(std::mt19937_64& gen)
  {
    return static_cast<double>(gen() >> 11) * (1.0 / 9007199254740992.0);
  }

  // Uniform in [0, n), n > 0, by rejection so that no value is favoured.
  inline std::uint64_t
  uniformBelow(std::mt19937_64& gen, std::uint64_t n)
  {
    const std::uint64_t limit = UINT64_MAX - UINT64_MAX % n;
    std::uint64_t x = gen();
    while (limit <= x) {
      x = gen();
    }
    return x % n;
  }

  // Fisher-Yates from the back, as java.util.Collections.shuffle.
  template <class T>
  void
  shuffle(std::vector<T>& v, std::mt19937_64& gen)
  {
    for (std::size_t i = v.size(); 1 < i; --i) {
      std::swap(v[i - 1], v[uniformBelow(gen, i)]);
    }
    return;
  }

}  // namespace HexGen
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------

// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
// Seed mixing shared by every module that derives random streams from one seed: the engine's named
// streams (HexEngine::PrngStreams) and the map generator's (HexGen).
// ----------------------------------------------
#pragma once
#include <cstdint>

namespace HexModel {

  // SplitMix64 finaliser over seed and stream number: distinct streams give decorrelated generators.
  constexpr std::uint64_t
  mixSeed(std::uint64_t seed, std::uint64_t stream)
  {
    std::uint64_t z = seed ^ ((stream + 1ull) * 0x9E3779B97F4A7C15ull);
    z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ull;
    z = (z ^ (z >> 27)) * 0x94D049BB133111EBull;
    return z ^ (z >> 31);
  }

}  // namespace HexModel
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------

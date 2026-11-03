// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
// Named random streams: one std::mt19937_64 per purpose, each seeded from the session seed mixed
// with the stream's tag, so adding a consumer to one purpose never shifts another.
// ----------------------------------------------
#pragma once
#include "hexmodel/Seed.h"

#include <array>
#include <cstdint>
#include <random>
#include <string_view>

namespace HexEngine {

  enum class StreamTag : std::uint8_t { Setup, Combat, Weather, Deck, SeaMove, Player0, Player1, Player2, Player3 };
  inline constexpr int kStreamCount = 9;

  std::string_view streamName(StreamTag);  // "combat", as written in hexsave stream/@tag
  StreamTag streamTag(std::string_view);   // throws std::invalid_argument for an unknown name

  // SplitMix64 finaliser over seed and tag (HexModel::mixSeed): distinct tags give decorrelated
  // generators.
  constexpr std::uint64_t
  mixSeed(std::uint64_t seed, StreamTag tag)
  {
    return HexModel::mixSeed(seed, static_cast<std::uint64_t>(tag));
  }

  class PrngStreams {
  public:
    explicit PrngStreams(std::uint64_t seed);
    std::uint64_t seed() const { return seed_; }
    std::mt19937_64& stream(StreamTag);
    std::uint64_t draws(StreamTag) const;  // outputs consumed so far, for hexsave stream/@draws
    // Re-derive a stream and discard n outputs (loading a save).
    void restore(StreamTag, std::uint64_t draws);
    // Added in M4: the one counted draw. Every consumer goes through this (or rollDie, which does)
    // so that draws() and the event log can never disagree.
    std::uint64_t next(StreamTag);

  private:
    std::uint64_t seed_;
    std::array<std::mt19937_64, kStreamCount> streams_;
    std::array<std::uint64_t, kStreamCount> draws_{};
  };

  // Every roll goes through here so the draw count and the event log agree.
  int rollDie(PrngStreams&, StreamTag, int faces);

}  // namespace HexEngine
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------

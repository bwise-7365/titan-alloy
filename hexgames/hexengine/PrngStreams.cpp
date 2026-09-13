// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
#include "hexengine/PrngStreams.h"

#include <array>
#include <stdexcept>
#include <string>

namespace HexEngine {

  namespace {

    // Parallel to StreamTag, in its declaration order; these are the tokens hexsave stream/@tag holds.
    constexpr std::array<std::string_view, kStreamCount> kNames{
        "setup", "combat", "weather", "deck", "sea-move", "player0", "player1", "player2", "player3"};

    std::size_t
    slotOf(StreamTag tag)
    {
      const std::size_t slot = static_cast<std::size_t>(tag);
      if (kStreamCount <= slot) {
        throw std::invalid_argument("HexEngine: stream tag outside StreamTag");
      }
      return slot;
    }

  }  // namespace

  std::string_view
  streamName(StreamTag tag)
  {
    return kNames[slotOf(tag)];
  }

  StreamTag
  streamTag(std::string_view name)
  {
    for (std::size_t i = 0; i < kNames.size(); ++i) {
      if (kNames[i] == name) {
        return static_cast<StreamTag>(i);
      }
    }
    throw std::invalid_argument("HexEngine::streamTag: unknown stream name '" + std::string(name) + "'");
  }

  PrngStreams::PrngStreams(std::uint64_t seed) : seed_(seed)
  {
    for (std::size_t i = 0; i < kStreamCount; ++i) {
      streams_[i].seed(mixSeed(seed_, static_cast<StreamTag>(i)));
    }
  }

  std::mt19937_64&
  PrngStreams::stream(StreamTag tag)
  {
    return streams_[slotOf(tag)];
  }

  std::uint64_t
  PrngStreams::draws(StreamTag tag) const
  {
    return draws_[slotOf(tag)];
  }

  std::uint64_t
  PrngStreams::next(StreamTag tag)
  {
    const std::size_t slot = slotOf(tag);
    ++draws_[slot];
    return streams_[slot]();
  }

  void
  PrngStreams::restore(StreamTag tag, std::uint64_t draws)
  {
    const std::size_t slot = slotOf(tag);
    streams_[slot].seed(mixSeed(seed_, tag));
    streams_[slot].discard(draws);
    draws_[slot] = draws;
    return;
  }

  int
  rollDie(PrngStreams& streams, StreamTag tag, int faces)
  {
    if (1 > faces) {
      throw std::invalid_argument("HexEngine::rollDie: a die of " + std::to_string(faces) + " faces");
    }
    // One engine output per roll, so that stream/@draws in a save is exactly the number of rolls.
    // The residue's bias over a six-sided die is below one part in 2^61 and is documented here
    // rather than hidden behind a distribution whose consumption is implementation-defined.
    const std::uint64_t drawn = streams.next(tag);
    return 1 + static_cast<int>(drawn % static_cast<std::uint64_t>(faces));
  }

}  // namespace HexEngine
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------

// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
#include "hexengine/PrngStreams.h"

#include <gtest/gtest.h>

#include <set>
#include <stdexcept>

TEST(PrngStreamsTest, TagsAreDecorrelated)
{
  HexEngine::PrngStreams streams(20260913ull);
  std::set<std::uint64_t> firsts;
  for (int i = 0; i < HexEngine::kStreamCount; ++i) {
    firsts.insert(streams.next(static_cast<HexEngine::StreamTag>(i)));
  }
  EXPECT_EQ(static_cast<std::size_t>(HexEngine::kStreamCount), firsts.size());

  // mixSeed is a pure function: three values computed independently from the SplitMix64 finaliser.
  static_assert(16294208416658607535ull == HexEngine::mixSeed(0ull, HexEngine::StreamTag::Setup));
  static_assert(2724399783758602859ull == HexEngine::mixSeed(20260912ull, HexEngine::StreamTag::Combat));
  static_assert(14194966728679492740ull == HexEngine::mixSeed(1ull, HexEngine::StreamTag::Player3));
  EXPECT_EQ(16294208416658607535ull, HexEngine::mixSeed(0ull, HexEngine::StreamTag::Setup));
}

TEST(PrngStreamsTest, NamesRoundTrip)
{
  for (int i = 0; i < HexEngine::kStreamCount; ++i) {
    const HexEngine::StreamTag tag = static_cast<HexEngine::StreamTag>(i);
    EXPECT_EQ(tag, HexEngine::streamTag(HexEngine::streamName(tag)));
  }
  EXPECT_EQ("combat", HexEngine::streamName(HexEngine::StreamTag::Combat));
  EXPECT_THROW((void)HexEngine::streamTag("no-such-stream"), std::invalid_argument);
}

TEST(PrngStreamsTest, RestoreReplays)
{
  HexEngine::PrngStreams first(20260913ull);
  std::vector<std::uint64_t> outputs;
  for (int i = 0; i < 5; ++i) {
    outputs.push_back(first.next(HexEngine::StreamTag::Combat));
  }
  EXPECT_EQ(5u, first.draws(HexEngine::StreamTag::Combat));
  EXPECT_EQ(0u, first.draws(HexEngine::StreamTag::Weather));

  HexEngine::PrngStreams restored(20260913ull);
  restored.restore(HexEngine::StreamTag::Combat, 3);
  EXPECT_EQ(3u, restored.draws(HexEngine::StreamTag::Combat));
  EXPECT_EQ(outputs[3], restored.next(HexEngine::StreamTag::Combat));
  EXPECT_EQ(4u, restored.draws(HexEngine::StreamTag::Combat));
}

TEST(PrngStreamsTest, RollDieCountsOneDrawAndStaysInRange)
{
  HexEngine::PrngStreams streams(7ull);
  for (int i = 0; i < 100; ++i) {
    const int value = HexEngine::rollDie(streams, HexEngine::StreamTag::Combat, 6);
    EXPECT_LE(1, value);
    EXPECT_GE(6, value);
  }
  EXPECT_EQ(100u, streams.draws(HexEngine::StreamTag::Combat));
  EXPECT_THROW((void)HexEngine::rollDie(streams, HexEngine::StreamTag::Combat, 0), std::invalid_argument);
}
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------

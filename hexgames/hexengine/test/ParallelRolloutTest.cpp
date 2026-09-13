// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
#include "TrcFixture.h"

#include "hexengine/Defaults.h"
#include "hexengine/Player.h"

#include <gtest/gtest.h>

#include <cstdint>
#include <random>
#include <set>
#include <thread>
#include <vector>

namespace {

  constexpr int kThreads = 16;
  constexpr int kCommands = 200;

  std::uint64_t
  rollout(const HexEngine::Session& root, std::uint64_t seed)
  {
    HexEngine::Session session = root.fork();
    std::mt19937_64 stream(seed);
    HexEngine::RandomPlayer player(stream);
    std::vector<HexEngine::Player*> bySide{&player, &player};
    HexEngine::play(session, bySide, kCommands);
    return session.position().digest();
  }

}  // namespace

TEST(ParallelRolloutTest, ForksAgreeWithSerial)
{
  const std::shared_ptr<const HexRules::GameDefinition> definition = TrcFixture::definition();
  const HexEngine::DefaultPolicySet defaults(*definition);
  const HexEngine::Session root(definition, defaults.policies(), TrcFixture::scenario(*definition),
                                 20260912ull);

  std::vector<std::uint64_t> serial(kThreads, 0);
  for (int i = 0; i < kThreads; ++i) {
    serial[static_cast<std::size_t>(i)] = rollout(root, 1000ull + static_cast<std::uint64_t>(i));
  }

  std::vector<std::uint64_t> parallel(kThreads, 0);
  std::vector<std::thread> threads;
  threads.reserve(kThreads);
  for (int i = 0; i < kThreads; ++i) {
    threads.emplace_back([&root, &parallel, i]() {
      parallel[static_cast<std::size_t>(i)] = rollout(root, 1000ull + static_cast<std::uint64_t>(i));
    });
  }
  for (std::thread& thread : threads) {
    thread.join();
  }

  EXPECT_EQ(serial, parallel);
  // Different streams take different games: the digests are not all one value.
  EXPECT_LT(1u, std::set<std::uint64_t>(serial.begin(), serial.end()).size());
}
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------

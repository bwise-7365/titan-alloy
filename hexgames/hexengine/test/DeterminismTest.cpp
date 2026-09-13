// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
#include "TrcFixture.h"

#include "hexengine/Defaults.h"
#include "hexengine/Player.h"

#include <gtest/gtest.h>

#include <random>
#include <string>
#include <vector>

namespace {

  std::vector<std::string>
  linesOf(const HexEngine::Session& session, const HexEngine::GameNames& names)
  {
    std::vector<std::string> out;
    for (const HexEngine::Event& event : session.events().events()) {
      out.push_back(HexEngine::TextEventEncoder::line(event, names));
    }
    return out;
  }

}  // namespace

TEST(DeterminismTest, SameSeedSameScript)
{
  const std::shared_ptr<const HexRules::GameDefinition> definition = TrcFixture::definition();
  const HexEngine::DefaultPolicySet defaults(*definition);
  const HexEngine::GameNames& names = defaults.names();

  const HexModel::UnitId armour = TrcFixture::unitOf(*definition, "g-ge-41-armour");
  const HexModel::UnitId other = TrcFixture::unitOf(*definition, "g-ge-56-armour");

  const auto run = [&]() {
    HexEngine::Session session(definition, defaults.policies(), TrcFixture::scenario(*definition),
                                20260912ull);
    std::vector<std::uint64_t> digests;
    const HexEngine::Reachability first = session.reachable(std::span<const HexModel::UnitId>(&armour, 1),
                                                             HexModel::ModeId{0});
    session.apply(HexEngine::MoveUnit{{armour}, HexModel::ModeId{0}, first.paths.at(3)});
    digests.push_back(session.position().digest());

    const HexEngine::Reachability second = session.reachable(std::span<const HexModel::UnitId>(&other, 1),
                                                              HexModel::ModeId{0});
    session.apply(HexEngine::MoveUnit{{other}, HexModel::ModeId{0}, second.paths.at(2)});
    digests.push_back(session.position().digest());

    session.apply(HexEngine::EndPhase{});
    digests.push_back(session.position().digest());
    session.apply(HexEngine::EndPhase{});
    digests.push_back(session.position().digest());
    return std::pair<std::vector<std::uint64_t>, std::vector<std::string>>{digests, linesOf(session, names)};
  };

  const auto once = run();
  const auto twice = run();
  EXPECT_EQ(once.first, twice.first);
  EXPECT_EQ(once.second, twice.second);
  EXPECT_EQ(4u, once.first.size());
  EXPECT_NE(once.first[0], once.first[1]);
}

TEST(DeterminismTest, SameSeedSameRandomRollout)
{
  const std::shared_ptr<const HexRules::GameDefinition> definition = TrcFixture::definition();
  const HexEngine::DefaultPolicySet defaults(*definition);

  const auto run = [&]() {
    HexEngine::Session session(definition, defaults.policies(), TrcFixture::scenario(*definition),
                                20260912ull);
    std::mt19937_64 stream(4242ull);
    HexEngine::RandomPlayer player(stream);
    std::vector<HexEngine::Player*> bySide{&player, &player};
    HexEngine::play(session, bySide, 40);
    return session.position().digest();
  };

  EXPECT_EQ(run(), run());
}
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------

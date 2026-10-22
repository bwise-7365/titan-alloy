// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
// TRC's typed state: the codec reads and writes every flag, refuses a bad one by name, the state
// forks as a value and changes the digest, and it survives a hexsave write and read.
// ----------------------------------------------
#include "TrcTestFixture.h"

#include "hexrecord/Record.h"

#include <gtest/gtest.h>

#include <filesystem>
#include <string>

namespace {

  using HexModel::SideFlag;
  using HexModel::SideFlags;

  // Every flag the codec knows, each side sorted by name as the codec writes them.
  SideFlags
  everyFlag(const HexRules::RuleSet& rules)
  {
    SideFlags flags(rules.sides().size());
    flags[rules.side("axis").value] = {{"air-used", "1"},
                                       {"garrison-warsaw", "due"},
                                       {"helsinki-russian", "true"},
                                       {"leader-lost", "active"},
                                       {"rail-moves", "0"},
                                       {"rail-touched", "F27 G27"},
                                       {"replaced", "armour infantry-3-4"},
                                       {"replaced-armour", "2"},
                                       {"sea-used", "baltic black-sea"},
                                       {"sudden-death", "sudden-death-1942 axis"},
                                       {"surrendered", "finnish italian"},
                                       {"weather", "light-mud"},
                                       {"weather-drm", "-1"}};
    flags[rules.side("russian").value] = {{"replaced-guards", "1"}, {"replacement-points", "0"}, {"south-entry", "1"}};
    return flags;
  }

  std::string
  refusal(const Trc::TrcStateCodec& codec, const SideFlags& flags)
  {
    try {
      (void)codec.decode(flags);
    } catch (const std::invalid_argument& e) {
      return e.what();
    }
    return "accepted";
  }

  bool
  sameFlagsP(const SideFlags& a, const SideFlags& b)
  {
    if (a.size() != b.size()) {
      return false;
    }
    for (std::size_t s = 0; s < a.size(); ++s) {
      if (a[s].size() != b[s].size()) {
        return false;
      }
      for (std::size_t i = 0; i < a[s].size(); ++i) {
        if (a[s][i].name != b[s][i].name || a[s][i].value != b[s][i].value) {
          return false;
        }
      }
    }
    return true;
  }

}  // namespace

TEST(TrcStateTest, CodecReadsAndWritesEveryFlag)
{
  const auto definition = TrcTest::definition();
  const Trc::TrcStateCodec codec(*definition->rules);
  const SideFlags flags = everyFlag(*definition->rules);
  const HexModel::Polymorphic<HexModel::GameState> decoded = codec.decode(flags);
  const Trc::TrcState& state = decoded.as<Trc::TrcState>("decoded");

  const HexModel::SideId axis = definition->rules->side("axis");
  EXPECT_TRUE(Trc::Weather::LightMud == state.weather);
  EXPECT_EQ(-1, *state.weatherDrm);
  EXPECT_TRUE(state.surrendered.contains(Trc::Nation::Italian));
  EXPECT_TRUE(Trc::LeaderLost::Active == state.side(axis).leaderLost);
  EXPECT_EQ(0, *state.side(axis).railMoves);  // zero is not absence
  EXPECT_TRUE(state.side(axis).railTouched.contains(HexCoord::HexId{"G27"}));
  EXPECT_EQ(axis, state.suddenDeath->winner);
  EXPECT_TRUE(sameFlagsP(flags, codec.encode(state)));
}

TEST(TrcStateTest, CodecNamesTheFlagItRefuses)
{
  const auto definition = TrcTest::definition();
  const Trc::TrcStateCodec codec(*definition->rules);
  const std::size_t axis = definition->rules->side("axis").value;
  const std::size_t russian = definition->rules->side("russian").value;
  const auto one = [&](std::size_t side, const std::string& name, const std::string& value) {
    SideFlags flags(definition->rules->sides().size());
    flags[side].push_back(SideFlag{name, value});
    return refusal(codec, flags);
  };
  EXPECT_NE(std::string::npos, one(axis, "weather", "sunny").find("'weather'"));
  EXPECT_NE(std::string::npos, one(axis, "rail-moves", "two").find("'rail-moves'"));
  EXPECT_NE(std::string::npos, one(axis, "frost", "1").find("'frost'"));
  EXPECT_NE(std::string::npos, one(russian, "weather-drm", "0").find("'weather-drm'"));
  EXPECT_NE(std::string::npos, one(axis, "sea-used", "arctic").find("'sea-used'"));
  EXPECT_NE(std::string::npos, one(axis, "leader-lost", "gone").find("'leader-lost'"));

  SideFlags twice(definition->rules->sides().size());
  twice[axis] = {{"air-used", "1"}, {"air-used", "2"}};
  EXPECT_NE(std::string::npos, refusal(codec, twice).find("'air-used'"));
}

TEST(TrcStateTest, ForkAndDigest)
{
  const auto definition = TrcTest::definition();
  HexModel::Position position = TrcTest::blank(*definition, 3, "axis-i1-move", "axis");
  const HexModel::Position fork = position;
  EXPECT_EQ(position.digest(), fork.digest());

  Trc::stateOf(position).weather = Trc::Weather::Mud;
  EXPECT_NE(position.digest(), fork.digest());
  EXPECT_FALSE(Trc::stateOf(fork).weather.has_value());

  HexModel::Position counted = fork;
  Trc::stateOf(counted).side(definition->rules->side("russian")).replacementPoints = 0;
  EXPECT_NE(counted.digest(), fork.digest());
}

TEST(TrcStateTest, HexsaveRoundTrip)
{
  const auto definition = TrcTest::definition();
  const Trc::TrcPolicySet set(*definition);
  HexModel::Position position = TrcTest::blank(*definition, 3, "axis-i1-move", "axis");
  const Trc::TrcStateCodec codec(*definition->rules);
  position.setGameState(codec.decode(everyFlag(*definition->rules)));
  const HexEngine::Session session(definition, set.policies(), position, 20260914ull);

  const std::filesystem::path saved = std::filesystem::temp_directory_path() / "hexgames-trc-state.xml";
  HexRecord::writeRecord(saved, HexRecord::Kind::Save, session, {}, *set.policies().grammar);
  const HexRecord::Record reloaded = HexRecord::readRecord(saved, *definition, set.policies());
  std::filesystem::remove(saved);

  EXPECT_TRUE(sameFlagsP(everyFlag(*definition->rules), codec.encode(Trc::stateOf(reloaded.position))));
}
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------

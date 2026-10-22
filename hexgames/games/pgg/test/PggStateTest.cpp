// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
// PGG's typed state and battle obligation: the codecs read and write every field, refuse a bad one by
// name, the state forks as a value and changes the digest, and both survive a hexsave write and read.
// ----------------------------------------------
#include "PggTestFixture.h"

#include "hexrecord/Record.h"

#include <gtest/gtest.h>

#include <filesystem>
#include <string>

namespace {

  using HexModel::SideFlag;
  using HexModel::SideFlags;

  SideFlags
  everyFlag(const Pgg::PggPolicySet& set)
  {
    const Pgg::PggFacts& facts = set.facts();
    SideFlags flags(2);
    flags[facts.german().value] = {{"air-interdiction", "2117 0513"},
                                   {"continuing", "s2-25-7-arm-4-10"},
                                   {"disrupted", "s2-6-3-arm-4-10"},
                                   {"eliminated", "s2-6-inf-2-7 s2-6-inf-9-7"},  // roster order
                                   {"entered", "s2-6-7-inf-2-10"},
                                   {"german-held", "0513 0420"},  // board order
                                   {"halted", "s2-7-7-inf-2-10"},
                                   {"outcome", "50-79"},
                                   {"passed", "0714"},
                                   {"rail-cuts", "0714 0715"},
                                   {"rail-repaired", "1215:3"},
                                   {"retreated-onto", "s2-5-inf-9-7"},
                                   {"smolensk-taken", "5"},
                                   {"spent", "s2-25-7-arm-4-10:7"},
                                   {"unsupplied", "s2-26-inf-9-7"},
                                   {"zoc-entry", "s2-25-7-arm-4-10:2016"}};
    flags[facts.soviet().value] = {{"army-13", "s1-f-inf-2-4-6"},
                                   {"army-16", "s1-64-inf-2-4-6"},
                                   {"beyond-radius", "s1-275-inf-2-4-6"},
                                   {"frozen", "16"},
                                   {"interdiction", "1414"},
                                   {"interdiction-turns", "2"},
                                   {"owed", "W:3:1 X:4:0"},
                                   {"rail-units", "3"},
                                   {"recapture-vp", "4"},
                                   {"swf-this-turn", "1"},
                                   {"swf-used", "6"}};
    return flags;
  }

  std::string
  refusal(const Pgg::PggStateCodec& codec, const SideFlags& flags)
  {
    try {
      (void)codec.decode(flags);
    } catch (const std::invalid_argument& e) {
      return e.what();
    }
    return "accepted";
  }

  std::string
  dump(const SideFlags& flags)
  {
    std::string out;
    for (const std::vector<SideFlag>& side : flags) {
      for (const SideFlag& flag : side) {
        out += flag.name + "=" + flag.value + "; ";
      }
      out += "| ";
    }
    return out;
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

TEST(PggStateTest, CodecReadsAndWritesEveryFlag)
{
  const auto definition = PggTest::definition();
  const Pgg::PggPolicySet set(*definition);
  const SideFlags flags = everyFlag(set);
  const HexModel::Polymorphic<HexModel::GameState> decoded = set.stateCodec().decode(flags);
  const Pgg::PggState& state = decoded.as<Pgg::PggState>("decoded");
  EXPECT_EQ(2u, state.airInterdiction.size());
  EXPECT_EQ(Pgg::Army::Sixteenth, state.armies.at(PggTest::unit(set, "s1-64-inf-2-4-6")));
  EXPECT_TRUE(state.frozen.contains(Pgg::Army::Sixteenth));
  EXPECT_EQ(3, state.owed.at(Pgg::Area::W).rifles);
  EXPECT_EQ(7, state.side(set.facts().german()).spent.at(PggTest::unit(set, "s2-25-7-arm-4-10")));
  EXPECT_EQ(3u, *state.outcome);
  EXPECT_TRUE(sameFlagsP(flags, set.stateCodec().encode(state))) << dump(set.stateCodec().encode(state));
}

TEST(PggStateTest, CodecNamesTheFlagItRefuses)
{
  const auto definition = PggTest::definition();
  const Pgg::PggPolicySet set(*definition);
  const std::size_t german = set.facts().german().value;
  const std::size_t soviet = set.facts().soviet().value;
  const auto one = [&](std::size_t side, const std::string& name, const std::string& value) {
    SideFlags flags(2);
    flags[side].push_back(SideFlag{name, value});
    return refusal(set.stateCodec(), flags);
  };
  EXPECT_NE(std::string::npos, one(german, "air-interdiction", "9999").find("'air-interdiction'"));
  EXPECT_NE(std::string::npos, one(german, "interdiction", "1414").find("'interdiction'"));  // Soviet only
  EXPECT_NE(std::string::npos, one(soviet, "swf-used", "many").find("'swf-used'"));
  EXPECT_NE(std::string::npos, one(soviet, "owed", "X:4").find("'owed'"));
  EXPECT_NE(std::string::npos, one(soviet, "army-17", "s1-64-inf-2-4-6").find("'army-17'"));
  EXPECT_NE(std::string::npos, one(soviet, "disrupted", "s2-6-3-arm-4-10").find("'disrupted'"));  // wrong side
  EXPECT_NE(std::string::npos, one(german, "outcome", "7-9").find("'outcome'"));
}

TEST(PggStateTest, ForkAndDigest)
{
  const auto definition = PggTest::definition();
  const Pgg::PggPolicySet set(*definition);
  HexModel::Position position = PggTest::blank(set, 3, "german-move1", "german");
  const HexModel::Position fork = position;
  EXPECT_EQ(position.digest(), fork.digest());
  Pgg::stateOf(position).railCuts.insert(PggTest::hex(set, "0714"));
  EXPECT_NE(position.digest(), fork.digest());
  EXPECT_TRUE(Pgg::stateOf(fork).railCuts.empty());
}

TEST(PggStateTest, BattleCodecRoundTrip)
{
  const auto definition = PggTest::definition();
  const Pgg::PggPolicySet set(*definition);
  Pgg::PggBattle battle;
  battle.attackers = {PggTest::unit(set, "s2-25-7-arm-4-10")};
  battle.defenders = {PggTest::unit(set, "s1-64-inf-2-4-6")};
  battle.target = PggTest::hex(set, "2117");
  battle.attackerSide = set.facts().german();
  battle.defenderSide = set.facts().soviet();
  battle.overrunP = true;
  battle.code = "D2*";
  battle.stage = Pgg::BattleStage::Defender;
  battle.startedP = true;
  battle.choseP = true;
  battle.walk = Pgg::RetreatWalk{battle.defenders.front(), battle.target, 1, {PggTest::hex(set, "2217")}};
  battle.pathOfRetreat = {PggTest::hex(set, "2217")};
  battle.defenderHitP = true;
  const std::vector<HexModel::ObligationArg> args = set.battleCodec().encode(battle);
  const HexModel::Polymorphic<HexModel::GameObligation> decoded = set.battleCodec().decode("pgg-battle", args);
  std::string before;
  std::string after;
  battle.appendDigest(before);
  decoded.base().appendDigest(after);
  EXPECT_EQ(before, after);
  EXPECT_THROW((void)set.battleCodec().decode("battle", args), std::invalid_argument);
  std::vector<HexModel::ObligationArg> missing = args;
  missing.erase(missing.begin() + 2);  // target
  EXPECT_THROW((void)set.battleCodec().decode("pgg-battle", missing), std::invalid_argument);
}

TEST(PggStateTest, HexsaveRoundTrip)
{
  const auto definition = PggTest::definition();
  const Pgg::PggPolicySet set(*definition);
  HexModel::Position position = PggTest::blank(set, 3, "german-move1", "german");
  position.setGameState(set.stateCodec().decode(everyFlag(set)));
  const HexEngine::Session session(definition, set.policies(), position, 20260914ull);
  const std::filesystem::path saved = std::filesystem::temp_directory_path() / "hexgames-pgg-state.xml";
  HexRecord::writeRecord(saved, HexRecord::Kind::Save, session, {}, *set.policies().grammar);
  const HexRecord::Record reloaded = HexRecord::readRecord(saved, *definition, set.policies());
  std::filesystem::remove(saved);
  EXPECT_TRUE(sameFlagsP(everyFlag(set), set.stateCodec().encode(Pgg::stateOf(reloaded.position))));
}
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------

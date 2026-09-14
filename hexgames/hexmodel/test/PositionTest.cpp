// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
#include "hexmodel/PositionBuilder.h"

#include "hexmodel/BoardBuilder.h"
#include "hexmodel/RosterBuilder.h"
#include "hexrules/RuleSetBuilder.h"
#include "hexxml/PackageDoc.h"
#include "hexxml/RulesDoc.h"
#include "hexxml/SaveDoc.h"
#include "hexxml/SheetDoc.h"
#include "hexxml/XmlDocument.h"

#include <gtest/gtest.h>

#include <filesystem>
#include <stdexcept>

namespace {

  std::filesystem::path
  root()
  {
    return std::filesystem::path(HEXGAMES_SOURCE_DIR);
  }

  HexModel::Strengths
  trivialReader(std::string_view, HexModel::UnitKind)
  {
    return HexModel::Strengths{};
  }

  struct Fixture {
    HexRules::RuleSet rules;
    HexModel::Board board;
    HexModel::Roster roster;
  };

  Fixture
  buildFixture()
  {
    const HexXml::RulesDoc rulesDoc = HexXml::RulesDoc::parse(
        HexXml::XmlDocument::load(root() / "game_rules" / "xml" / "the-russian-campaign.xml"));
    HexRules::RuleSet rules = HexRules::RuleSetBuilder::build(rulesDoc);
    const HexXml::SheetDoc sheet = HexXml::SheetDoc::parse(
        HexXml::XmlDocument::load(root() / "map_graphics" / "xml" / "the-russian-campaign.xml"));
    const HexXml::PackageDoc package =
        HexXml::PackageDoc::parse(HexXml::XmlDocument::load(root() / "packages" / "xml" / "trc.package.xml"));
    HexModel::Board board = HexModel::BoardBuilder::build(sheet, rules, package);
    const HexXml::CounterSetDoc counters = HexXml::CounterSetDoc::parse(
        HexXml::XmlDocument::load(root() / "unit_graphics" / "xml" / "the-russian-campaign.xml"));
    HexModel::Roster roster = HexModel::RosterBuilder::build(counters, rules, package, &trivialReader);
    return Fixture{std::move(rules), std::move(board), std::move(roster)};
  }

  struct TestState : HexModel::Cloneable<TestState, HexModel::GameState> {
    void appendDigest(std::string& s) const override { s += "test;"; }
  };

  struct OtherState : HexModel::Cloneable<OtherState, HexModel::GameState> {
    void appendDigest(std::string& s) const override { s += "other;"; }
  };

  // A test game's codecs: a TestState from a document with no flags, and no game obligations.
  class FlaglessCodec : public HexModel::GameStateCodec {
  public:
    HexModel::Polymorphic<HexModel::GameState>
    decode(const HexModel::SideFlags& flags) const override
    {
      for (const std::vector<HexModel::SideFlag>& side : flags) {
        if (!side.empty()) {
          throw std::invalid_argument("PositionTest: flag '" + side.front().name + "'");
        }
      }
      return HexModel::makePolymorphic<HexModel::GameState, TestState>();
    }
    HexModel::SideFlags encode(const HexModel::GameState&) const override { return HexModel::SideFlags(2); }
  };

  class NoObligations : public HexModel::ObligationCodec {
  public:
    HexModel::Polymorphic<HexModel::GameObligation>
    decode(const std::string& name, const std::vector<HexModel::ObligationArg>&) const override
    {
      throw std::invalid_argument("PositionTest: obligation '" + name + "'");
    }
    std::vector<HexModel::ObligationArg>
    encode(const HexModel::GameObligation& owed) const override
    {
      throw std::invalid_argument("PositionTest: obligation '" + std::string(owed.kind()) + "'");
    }
  };

  HexModel::Position
  buildPosition(const Fixture& fx)
  {
    const HexXml::SaveDoc save =
        HexXml::SaveDoc::parse(HexXml::XmlDocument::load(root() / "game_records" / "xml" / "trc-test.xml"));
    const FlaglessCodec state;
    const NoObligations obligations;
    return HexModel::PositionBuilder::build(save, fx.board, fx.roster, fx.rules, state, obligations);
  }

}  // namespace

TEST(PositionTest, GameStateIsCheckedCopiedAndDigested)
{
  const Fixture fx = buildFixture();
  HexModel::Position pos = buildPosition(fx);

  EXPECT_NO_THROW((void)pos.gameState<TestState>());
  EXPECT_THROW((void)pos.gameState<OtherState>(), std::invalid_argument);
  EXPECT_THROW((void)HexModel::Position().gameState<TestState>(), std::invalid_argument);

  const HexModel::Position copy = pos;
  EXPECT_EQ(pos.digest(), copy.digest());
  pos.setGameState(HexModel::makePolymorphic<HexModel::GameState, OtherState>());
  EXPECT_NE(pos.digest(), copy.digest());
  EXPECT_NO_THROW((void)copy.gameState<TestState>());
}

TEST(PositionTest, PlaceKeepsStacksInStep)
{
  const Fixture fx = buildFixture();
  HexModel::Position pos = buildPosition(fx);

  const HexModel::UnitId armour41 = *fx.roster.find(HexModel::CounterId{"g-ge-41-armour"});
  const HexModel::UnitId armour56 = *fx.roster.find(HexModel::CounterId{"g-ge-56-armour"});
  const HexModel::HexIndex hexA = fx.board.indexOf(HexCoord::HexId{"F27"});  // 41st armour's setup hex
  const HexModel::HexIndex hexB = fx.board.indexOf(HexCoord::HexId{"G27"});  // 56th armour's setup hex

  EXPECT_EQ(std::vector<HexModel::UnitId>{armour41}, pos.unitsAt(hexA));

  pos.place(armour56, hexA);
  EXPECT_EQ((std::vector<HexModel::UnitId>{armour41, armour56}), pos.unitsAt(hexA));
  EXPECT_TRUE(pos.unitsAt(hexB).empty());

  // Placing the same unit on the same hex again must not duplicate it.
  pos.place(armour56, hexA);
  EXPECT_EQ((std::vector<HexModel::UnitId>{armour41, armour56}), pos.unitsAt(hexA));

  pos.place(armour56, hexB);
  EXPECT_EQ(std::vector<HexModel::UnitId>{armour41}, pos.unitsAt(hexA));
  EXPECT_EQ(std::vector<HexModel::UnitId>{armour56}, pos.unitsAt(hexB));

  pos.remove(armour56);
  EXPECT_TRUE(pos.unitsAt(hexB).empty());
  EXPECT_FALSE(pos.unit(armour56).where.has_value());
}

TEST(PositionTest, ControlIsStoredNotDerived)
{
  const Fixture fx = buildFixture();
  HexModel::Position pos = buildPosition(fx);

  const HexModel::HexIndex berlin = fx.board.indexOf(HexCoord::HexId{"E31"});
  const HexModel::SideId axis = fx.rules.side("axis");
  EXPECT_EQ(axis, pos.control(berlin));

  const HexModel::UnitId hitler = *fx.roster.find(HexModel::CounterId{"g-ge-hitler-hitler"});
  pos.remove(hitler);
  EXPECT_EQ(axis, pos.control(berlin));  // control persists after the occupant leaves
}

TEST(PositionTest, DigestIsCanonical)
{
  const Fixture fx = buildFixture();
  HexModel::Position a = buildPosition(fx);
  HexModel::Position b = buildPosition(fx);
  EXPECT_EQ(a.digest(), b.digest());

  // Reaching the same final placement through a different order of calls gives the same digest.
  const HexModel::UnitId armour41 = *fx.roster.find(HexModel::CounterId{"g-ge-41-armour"});
  const HexModel::HexIndex hexA = fx.board.indexOf(HexCoord::HexId{"F27"});
  const HexModel::HexIndex hexC = fx.board.indexOf(HexCoord::HexId{"E27"});
  a.place(armour41, hexC);
  a.place(armour41, hexA);  // ends back where it started
  EXPECT_EQ(a.digest(), b.digest());

  const HexModel::Position copy = b;
  EXPECT_EQ(b.digest(), copy.digest());

  // TRC's counters are single-step in this data set (no <steps> element prints more than one), so
  // reduced() has nothing to reduce from; a synthetic 1-of-2 Steps stands in for "one step lost".
  b.state(armour41).steps = HexModel::Steps(1, 2);
  EXPECT_NE(a.digest(), b.digest());
}

TEST(PositionTest, PendingDecisionRoundTrip)
{
  const Fixture fx = buildFixture();
  HexModel::Position pos = buildPosition(fx);
  const HexModel::UnitId armour41 = *fx.roster.find(HexModel::CounterId{"g-ge-41-armour"});
  const HexModel::HexIndex hexA = fx.board.indexOf(HexCoord::HexId{"F27"});

  // Nothing owed: nothing pending, and asking has nowhere to go.
  EXPECT_TRUE(std::holds_alternative<HexModel::NoDecision>(pos.pending()));
  EXPECT_THROW(pos.ask(HexModel::NoDecision{}), std::invalid_argument);
  EXPECT_THROW(pos.pop(), std::invalid_argument);

  const HexModel::Battle battle{std::nullopt, {armour41}};
  pos.push(HexModel::OwedLoss{battle, HexModel::SideId{0}, 1});
  EXPECT_TRUE(std::holds_alternative<HexModel::NoDecision>(pos.pending()));
  pos.ask(HexModel::ChooseLoss{HexModel::SideId{0}, {armour41}, 1});
  ASSERT_TRUE(std::holds_alternative<HexModel::ChooseLoss>(pos.pending()));
  EXPECT_EQ(1, std::get<HexModel::ChooseLoss>(pos.pending()).count);

  // A new top hides the decision below it until it is popped.
  pos.push(HexModel::UnitRetreat{battle, armour41, hexA, 2, 2, {}});
  EXPECT_TRUE(std::holds_alternative<HexModel::NoDecision>(pos.pending()));
  pos.ask(HexModel::ChooseRetreat{HexModel::SideId{0}, armour41, {hexA}, false});
  ASSERT_TRUE(std::holds_alternative<HexModel::ChooseRetreat>(pos.pending()));
  EXPECT_EQ(armour41, std::get<HexModel::ChooseRetreat>(pos.pending()).unit);

  const HexModel::Position copy = pos;
  EXPECT_EQ(pos.digest(), copy.digest());
  pos.pop();
  EXPECT_TRUE(std::holds_alternative<HexModel::ChooseLoss>(pos.pending()));
  EXPECT_NE(pos.digest(), copy.digest());
  pos.pop();
  EXPECT_TRUE(pos.resolution().empty());
}
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------

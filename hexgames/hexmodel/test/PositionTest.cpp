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

  HexModel::Position
  buildPosition(const Fixture& fx)
  {
    const HexXml::SaveDoc save =
        HexXml::SaveDoc::parse(HexXml::XmlDocument::load(root() / "game_records" / "xml" / "trc-test.xml"));
    return HexModel::PositionBuilder::build(save, fx.board, fx.roster, fx.rules);
  }

}  // namespace

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

  pos.setPending(HexModel::NoDecision{});
  EXPECT_TRUE(std::holds_alternative<HexModel::NoDecision>(pos.pending()));

  pos.setPending(HexModel::ChooseLoss{{armour41}, 1});
  ASSERT_TRUE(std::holds_alternative<HexModel::ChooseLoss>(pos.pending()));
  EXPECT_EQ(1, std::get<HexModel::ChooseLoss>(pos.pending()).count);

  pos.setPending(HexModel::ChooseRetreat{armour41, {hexA}});
  ASSERT_TRUE(std::holds_alternative<HexModel::ChooseRetreat>(pos.pending()));
  EXPECT_EQ(armour41, std::get<HexModel::ChooseRetreat>(pos.pending()).unit);

  pos.setPending(HexModel::ChooseCard{HexModel::RandomizerId{0}, {"card-a"}});
  ASSERT_TRUE(std::holds_alternative<HexModel::ChooseCard>(pos.pending()));

  pos.setPending(HexModel::GameChoice{"weather", {"clear", "mud"}});
  ASSERT_TRUE(std::holds_alternative<HexModel::GameChoice>(pos.pending()));
  EXPECT_EQ("weather", std::get<HexModel::GameChoice>(pos.pending()).verb);
}
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------

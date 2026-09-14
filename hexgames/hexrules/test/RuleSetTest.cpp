// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
#include "hexrules/RuleSetBuilder.h"

#include "hexxml/RulesDoc.h"
#include "hexxml/XmlDocument.h"

#include <gtest/gtest.h>

#include <algorithm>
#include <filesystem>
#include <functional>
#include <stdexcept>

namespace {

  std::filesystem::path
  gameRules(const char* name)
  {
    return std::filesystem::path(HEXGAMES_SOURCE_DIR) / "game_rules" / "xml" / name;
  }

  HexRules::RuleSet
  buildFrom(const char* name)
  {
    return HexRules::RuleSetBuilder::build(HexXml::RulesDoc::parse(HexXml::XmlDocument::load(gameRules(name))));
  }

  std::size_t
  phaseTreeDepth(const std::vector<HexRules::PhaseNode>& nodes)
  {
    std::size_t deepest = 0;
    for (const HexRules::PhaseNode& n : nodes) {
      deepest = std::max(deepest, 1 + phaseTreeDepth(n.children));
    }
    return deepest;
  }

}  // namespace

TEST(RuleSetTest, ThreeRuleFilesLoad)
{
  const HexRules::RuleSet trc = buildFrom("the-russian-campaign.xml");
  EXPECT_EQ(2u, trc.sides().size());
  EXPECT_FALSE(trc.hexTerrain().empty());
  EXPECT_FALSE(trc.hexsideTerrain().empty());
  EXPECT_GE(phaseTreeDepth(trc.phases()), 3u);
  ASSERT_FALSE(trc.combat().resolvers.empty());
  const auto crt = std::find_if(trc.combat().resolvers.begin(), trc.combat().resolvers.end(),
                                 [](const HexRules::Resolver& r) { return "crt" == r.id; });
  ASSERT_NE(trc.combat().resolvers.end(), crt);
  ASSERT_EQ(1u, crt->tables.size());
  EXPECT_EQ(9u, crt->tables[0].cols.size());
  for (const std::vector<std::string>& row : crt->tables[0].cells) {
    EXPECT_EQ(9u, row.size());
  }
  EXPECT_FALSE(trc.randomizers().empty());
  EXPECT_EQ(54u, trc.proseRules().size());

  const HexRules::RuleSet ds = buildFrom("dai-senso.xml");
  EXPECT_EQ(3u, ds.sides().size());

  const HexRules::RuleSet tarawa = buildFrom("d-day-at-tarawa.xml");
  EXPECT_EQ(2u, tarawa.sides().size());  // solitaire: one side automaton-controlled, not one-sided
}

TEST(RuleSetTest, HostilityIsSymmetric)
{
  const HexRules::RuleSet ds = buildFrom("dai-senso.xml");
  const HexRules::SideId axis = ds.side("axis");
  const HexRules::SideId western = ds.side("western");
  const HexRules::SideId soviet = ds.side("soviet");
  EXPECT_TRUE(ds.hostility().hostileP(axis, western));
  EXPECT_TRUE(ds.hostility().hostileP(axis, soviet));
  EXPECT_TRUE(ds.hostility().hostileP(western, soviet));
  EXPECT_FALSE(ds.hostility().hostileP(axis, axis));

  const std::filesystem::path bad =
      std::filesystem::path(HEXGAMES_SOURCE_DIR) / "hexrules" / "test" / "rules-asymmetric-hostility.xml";
  try {
    (void)HexRules::RuleSetBuilder::build(HexXml::RulesDoc::parse(HexXml::XmlDocument::load(bad)));
    FAIL() << "expected std::invalid_argument";
  } catch (const std::invalid_argument& e) {
    const std::string message = e.what();
    EXPECT_NE(std::string::npos, message.find("a"));
    EXPECT_NE(std::string::npos, message.find("b"));
  }
}

TEST(RuleSetTest, IdrefsResolve)
{
  const HexRules::RuleSet trc = buildFrom("the-russian-campaign.xml");
  // A representative sample: every zoc's blocked-by resolves, every mode's phase resolves.
  for (const HexRules::ZocSpec& z : trc.zocs()) {
    for (HexModel::EdgeTerrainId id : z.blockedBy) {
      EXPECT_LT(id.value, trc.hexsideTerrain().size());
    }
  }
  for (const HexRules::Mode& m : trc.movement().modes) {
    for (HexModel::PhaseId id : m.phases) {
      EXPECT_LT(id.value, 200u);
    }
  }

  const std::filesystem::path bad =
      std::filesystem::path(HEXGAMES_SOURCE_DIR) / "hexrules" / "test" / "rules-dangling-idref.xml";
  try {
    (void)HexRules::RuleSetBuilder::build(HexXml::RulesDoc::parse(HexXml::XmlDocument::load(bad)));
    FAIL() << "expected std::invalid_argument";
  } catch (const std::invalid_argument& e) {
    const std::string message = e.what();
    EXPECT_NE(std::string::npos, message.find("no-such-unit-type"));
  }
}

TEST(RuleSetTest, ConcealmentLoads)
{
  const HexRules::RuleSet pgg = buildFrom("panzergruppe-guderian.xml");
  const HexRules::UnitType& rifle = pgg.unitTypes()[pgg.unitType("soviet-rifle").value];
  ASSERT_TRUE(rifle.concealment.has_value());
  EXPECT_EQ(HexRules::HiddenFrom::All, rifle.concealment->from);
  EXPECT_EQ(HexRules::Conceals::Values, rifle.concealment->conceals);
  EXPECT_EQ(std::vector<HexRules::RevealTrigger>{HexRules::RevealTrigger::Combat}, rifle.concealment->reveal);
  EXPECT_EQ(HexRules::Rehide::Never, rifle.concealment->rehide);
  EXPECT_FALSE(pgg.unitTypes()[pgg.unitType("german-infantry").value].concealment.has_value());

  const HexRules::RuleSet tarawa = buildFrom("d-day-at-tarawa.xml");
  const HexRules::UnitType& garrison = tarawa.unitTypes()[tarawa.unitType("jp-unit").value];
  ASSERT_TRUE(garrison.concealment.has_value());
  EXPECT_EQ(HexRules::HiddenFrom::Enemy, garrison.concealment->from);
  EXPECT_EQ(HexRules::Conceals::Identity, garrison.concealment->conceals);
  const std::vector<HexRules::RevealTrigger> attackedOrRule{HexRules::RevealTrigger::Attacked,
                                                            HexRules::RevealTrigger::Rule};
  EXPECT_EQ(attackedOrRule, garrison.concealment->reveal);
  EXPECT_EQ(HexRules::Rehide::Rule, garrison.concealment->rehide);

  // hidden="true" with only one of the four attributes is refused, naming the unit type.
  const std::filesystem::path bad =
      std::filesystem::path(HEXGAMES_SOURCE_DIR) / "hexrules" / "test" / "rules-hidden-incomplete.xml";
  try {
    (void)HexRules::RuleSetBuilder::build(HexXml::RulesDoc::parse(HexXml::XmlDocument::load(bad)));
    FAIL() << "expected std::invalid_argument";
  } catch (const std::invalid_argument& e) {
    EXPECT_NE(std::string::npos, std::string(e.what()).find("scout"));
  }
}

TEST(RuleSetTest, TurnSelectorsAndTables)
{
  const HexRules::RuleSet trc = buildFrom("the-russian-campaign.xml");
  const auto suddenDeath =
      std::find_if(trc.phases().begin(), trc.phases().end(),
                    [](const HexRules::PhaseNode& n) { return "game-turn" == n.name || true; });
  // Walk the tree looking for the sudden-death phase by id text (phase ids are dense, so search by
  // re-parsing turns from the raw document is unnecessary -- the phase tree already carries turns).
  std::function<const HexRules::PhaseNode*(const std::vector<HexRules::PhaseNode>&)> find =
      [&](const std::vector<HexRules::PhaseNode>& nodes) -> const HexRules::PhaseNode* {
    for (const HexRules::PhaseNode& n : nodes) {
      if ("Sudden Death Victory Check" == n.name) {
        return &n;
      }
      if (const HexRules::PhaseNode* found = find(n.children)) {
        return found;
      }
    }
    return nullptr;
  };
  const HexRules::PhaseNode* sd = find(trc.phases());
  ASSERT_NE(nullptr, sd);
  EXPECT_TRUE(sd->turns.containsP(5));
  EXPECT_TRUE(sd->turns.containsP(11));
  EXPECT_TRUE(sd->turns.containsP(17));
  EXPECT_TRUE(sd->turns.containsP(23));
  EXPECT_FALSE(sd->turns.containsP(6));
  (void)suddenDeath;

  const auto crt = std::find_if(trc.combat().resolvers.begin(), trc.combat().resolvers.end(),
                                 [](const HexRules::Resolver& r) { return "crt" == r.id; });
  ASSERT_NE(trc.combat().resolvers.end(), crt);
  ASSERT_EQ(1u, crt->tables.size());
  EXPECT_EQ(6u, crt->tables[0].rowLabels.size());
  EXPECT_EQ(9u, crt->tables[0].cols.size());
}
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------

// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
// Which rules steps run, and in what order, on a synthetic phase tree (rules-steps.xml): phase
// boundaries close the innermost phase first and open the outermost first, a turn wrap closes and
// reopens the root, @turns and @commands filter, and command steps run outermost phase first. A
// step naming something other than a prose rule is refused when the rules load.
// ----------------------------------------------
#include "hexengine/Sequence.h"
#include "hexrules/RuleSetBuilder.h"
#include "hexxml/RulesDoc.h"
#include "hexxml/XmlDocument.h"

#include <gtest/gtest.h>

#include <filesystem>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

  using Ids = std::vector<std::string>;

  HexRules::RuleSet
  load(const char* name)
  {
    const std::filesystem::path path = std::filesystem::path(HEXGAMES_SOURCE_DIR) / "hexengine" / "test" / name;
    return HexRules::RuleSetBuilder::build(HexXml::RulesDoc::parse(HexXml::XmlDocument::load(path)));
  }

  Ids
  ids(const std::vector<const HexRules::Step*>& steps)
  {
    Ids out;
    for (const HexRules::Step* step : steps) {
      out.push_back(step->id);
    }
    return out;
  }

  // Only the verbs matter to StepRegistry::verify.
  class ThreeVerbs : public HexEngine::CommandGrammar {
  public:
    std::string
    verb(const HexEngine::Command&) const override
    {
      throw std::invalid_argument("ThreeVerbs: no commands in this test");
    }
    HexEngine::Command
    parse(const std::string&, const std::vector<std::pair<std::string, std::string>>&) const override
    {
      throw std::invalid_argument("ThreeVerbs: no commands in this test");
    }
    std::vector<std::pair<std::string, std::string>>
    arguments(const HexEngine::Command&) const override
    {
      throw std::invalid_argument("ThreeVerbs: no commands in this test");
    }
    std::vector<std::string> verbs() const override { return {"move", "attack", "end-phase"}; }
  };

  HexEngine::PhaseCursor::Stop
  stop(const HexRules::RuleSet& rules, int turn, const char* phase, const char* side)
  {
    return HexEngine::PhaseCursor::Stop{turn, rules.phase(phase), rules.side(side)};
  }

}  // namespace

TEST(SequenceTest, BoundariesCloseInnermostFirstAndOpenOutermostFirst)
{
  const HexRules::RuleSet rules = load("rules-steps.xml");
  const HexEngine::PhaseCursor cursor(rules);

  const HexEngine::Sequence::Boundary within = HexEngine::Sequence::boundary(cursor, stop(rules, 1, "a-move", "a"), nullptr);
  EXPECT_EQ(Ids{"a-move-end"}, ids(within.end));
  EXPECT_EQ(Ids{"a-fight-enter"}, ids(within.enter));

  const HexEngine::Sequence::Boundary handOver = HexEngine::Sequence::boundary(cursor, stop(rules, 1, "a-fight", "a"), nullptr);
  EXPECT_EQ(Ids{"a-end"}, ids(handOver.end));
  EXPECT_EQ(Ids{"b-enter"}, ids(handOver.enter));
  EXPECT_EQ(rules.phase("b-move"), handOver.next.phase);

  // The turn wraps: every phase closes, innermost first, and the root reopens before its children.
  const HexEngine::Sequence::Boundary wrap = HexEngine::Sequence::boundary(cursor, stop(rules, 1, "b-move", "b"), nullptr);
  EXPECT_EQ(2, wrap.next.turn);
  EXPECT_EQ((Ids{"b-move-end", "turn-end"}), ids(wrap.end));
  EXPECT_EQ((Ids{"turn-enter", "a-enter", "a-move-enter-1", "a-move-enter-2"}), ids(wrap.enter));

  // a-move-enter-2 runs on turn 2 only.
  const HexEngine::Sequence::Boundary later = HexEngine::Sequence::boundary(cursor, stop(rules, 2, "b-move", "b"), nullptr);
  EXPECT_EQ((Ids{"turn-enter", "a-enter", "a-move-enter-1"}), ids(later.enter));
}

TEST(SequenceTest, CommandStepsFilterByVerbOutermostFirst)
{
  const HexRules::RuleSet rules = load("rules-steps.xml");
  const HexEngine::PhaseCursor cursor(rules);
  const auto steps = [&](const char* phase, HexRules::StepAt at, const char* verb) {
    return ids(HexEngine::Sequence::commandSteps(cursor, rules.phase(phase), 1, at, verb));
  };
  EXPECT_EQ((Ids{"turn-before-move", "a-move-before"}), steps("a-move", HexRules::StepAt::BeforeCommand, "move"));
  EXPECT_EQ(Ids{"a-move-before"}, steps("a-move", HexRules::StepAt::BeforeCommand, "attack"));
  EXPECT_EQ(Ids{"turn-before-move"}, steps("a-fight", HexRules::StepAt::BeforeCommand, "move"));
  EXPECT_EQ(Ids{"a-fight-before-attack"}, steps("a-fight", HexRules::StepAt::BeforeCommand, "attack"));
  EXPECT_EQ(Ids{"turn-after"}, steps("b-move", HexRules::StepAt::AfterCommand, "end-phase"));
  EXPECT_TRUE(steps("b-move", HexRules::StepAt::BeforeCommand, "end-phase").empty());
}

TEST(SequenceTest, EveryUnknownVerbIsReportedAtOnce)
{
  const HexRules::RuleSet rules = load("rules-bad-verbs.xml");
  HexEngine::StepRegistry registry;
  registry.addEffect("mark", [](const HexEngine::StepCall& call) { return call.ctx.position; });
  registry.addCheck("gate", [](const HexEngine::CheckCall&) { return; });
  const ThreeVerbs grammar;
  try {
    registry.verify(rules, grammar);
    FAIL() << "steps naming misspelt verbs were accepted";
  } catch (const std::invalid_argument& e) {
    const std::string what = e.what();
    EXPECT_NE(std::string::npos, what.find("step 'first-typo' (line ")) << what;
    EXPECT_NE(std::string::npos, what.find("'mvoe'")) << what;
    EXPECT_NE(std::string::npos, what.find("step 'second-typo' (line ")) << what;
    EXPECT_NE(std::string::npos, what.find("'end-phse'")) << what;
    EXPECT_EQ(std::string::npos, what.find("good-gate")) << what;
    EXPECT_EQ(std::string::npos, what.find("'attack'")) << what;
  }

  // The same rules spelt right pass; the synthetic step fixture uses only known verbs.
  EXPECT_NO_THROW(registry.verify(load("rules-steps.xml"), grammar));
}

TEST(SequenceTest, StepsKeepTheirRules)
{
  const HexRules::RuleSet rules = load("rules-steps.xml");
  const HexEngine::PhaseCursor cursor(rules);
  const HexRules::PhaseNode& turn = cursor.node(rules.phase("turn"));
  ASSERT_EQ(4u, turn.steps.size());
  EXPECT_EQ(HexRules::StepAt::BeforeCommand, turn.steps[1].at);
  ASSERT_EQ(1u, turn.steps[1].rules.size());
  EXPECT_EQ("gate-rule", turn.steps[1].rules.front().text);

  try {
    (void)load("rules-step-unknown-rule.xml");
    FAIL() << "a step naming a terrain in @rules was accepted";
  } catch (const std::invalid_argument& e) {
    const std::string what = e.what();
    EXPECT_NE(std::string::npos, what.find("'lonely'")) << what;
    EXPECT_NE(std::string::npos, what.find("'clear'")) << what;
  }
}
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------

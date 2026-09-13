// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
#include "hexengine/PhaseCursor.h"

#include "hexrules/RuleSetBuilder.h"
#include "hexxml/RulesDoc.h"
#include "hexxml/XmlDocument.h"

#include <gtest/gtest.h>

#include <filesystem>
#include <string>
#include <vector>

namespace {

  HexRules::RuleSet
  load(const std::string& file)
  {
    const std::filesystem::path path =
        std::filesystem::path(HEXGAMES_SOURCE_DIR) / "game_rules" / "xml" / file;
    return HexRules::RuleSetBuilder::build(HexXml::RulesDoc::parse(HexXml::XmlDocument::load(path)));
  }

  // Every stop of a turn as "<phase id>/<side id>", which is how the sequence reads on the page.
  std::vector<std::string>
  spell(const HexRules::RuleSet& rules, int turn)
  {
    const HexEngine::PhaseCursor cursor(rules);
    std::vector<std::string> out;
    std::vector<std::string> phaseOf(rules.phaseIds().size());
    for (const auto& [text, id] : rules.phaseIds()) {
      phaseOf[id.value] = text;
    }
    for (const HexEngine::PhaseCursor::Stop& stop : cursor.turnStops(turn, nullptr)) {
      const std::string side = stop.side ? rules.sides()[stop.side->value].id : std::string("-");
      out.push_back(phaseOf[stop.phase.value] + "/" + side);
    }
    return out;
  }

}  // namespace

TEST(PhaseCursorTest, WalksTrcTree)
{
  const HexRules::RuleSet rules = load("the-russian-campaign.xml");

  const std::vector<std::string> expected{
      "weather-phase/axis",     "axis-i1-move/axis",        "axis-i1-combat/axis",
      "axis-i2-move/axis",      "axis-i2-combat/axis",      "axis-end/axis",
      "russian-i1-move/russian", "russian-i1-combat/russian", "russian-i2-move/russian",
      "russian-i2-combat/russian", "russian-end/russian"};
  EXPECT_EQ(expected, spell(rules, 1));

  // The sudden-death check is a phase of turns 5, 11, 17 and 23 only.
  std::vector<std::string> onFive = expected;
  onFive.push_back("sudden-death/-");
  EXPECT_EQ(onFive, spell(rules, 5));
  EXPECT_EQ(expected, spell(rules, 6));
  EXPECT_EQ(onFive, spell(rules, 23));
}

TEST(PhaseCursorTest, NextWalksIntoTheFollowingTurn)
{
  const HexRules::RuleSet rules = load("the-russian-campaign.xml");
  const HexEngine::PhaseCursor cursor(rules);

  const std::vector<HexEngine::PhaseCursor::Stop> stops = cursor.turnStops(1, nullptr);
  ASSERT_FALSE(stops.empty());
  const HexEngine::PhaseCursor::Stop second = cursor.next(stops.front(), nullptr);
  EXPECT_EQ(stops[1], second);

  const HexEngine::PhaseCursor::Stop wrapped = cursor.next(stops.back(), nullptr);
  EXPECT_EQ(2, wrapped.turn);
  EXPECT_EQ(cursor.turnStops(2, nullptr).front(), wrapped);

  const HexEngine::PhaseCursor::Stop nowhere{1, rules.phase("axis-turn"), std::nullopt};
  EXPECT_THROW((void)cursor.next(nowhere, nullptr), std::invalid_argument);
}

TEST(PhaseCursorTest, RepeatsPerSideForDs)
{
  const HexRules::RuleSet rules = load("dai-senso.xml");
  const std::vector<std::string> stops = spell(rules, 1);

  // The faction turn is played three times, Axis then Western then Soviet, each time whole.
  std::vector<std::string> order;
  for (const std::string& stop : stops) {
    const std::string side = stop.substr(stop.rfind('/') + 1);
    if ("-" == side) {
      continue;
    }
    if (order.empty() || order.back() != side) {
      order.push_back(side);
    }
  }
  const std::vector<std::string> expected{"axis", "western", "soviet"};
  EXPECT_EQ(expected, order);

  // and the first phase of the second repetition is the first phase of the first.
  const auto firstOf = [&](const std::string& side) {
    for (const std::string& stop : stops) {
      if (stop.substr(stop.rfind('/') + 1) == side) {
        return stop.substr(0, stop.rfind('/'));
      }
    }
    return std::string();
  };
  EXPECT_EQ(firstOf("axis"), firstOf("western"));
  EXPECT_EQ(firstOf("axis"), firstOf("soviet"));
}
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------

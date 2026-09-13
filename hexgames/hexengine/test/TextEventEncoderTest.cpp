// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
#include "TrcFixture.h"

#include "hexengine/Event.h"
#include "hexengine/GameNames.h"

#include <gtest/gtest.h>

#include <set>
#include <string>
#include <vector>

TEST(TextEventEncoderTest, OneLinePerEvent)
{
  const std::shared_ptr<const HexRules::GameDefinition> definition = TrcFixture::definition();
  const HexEngine::GameNames names(*definition->board, *definition->roster, *definition->rules);

  const HexModel::UnitId unit = TrcFixture::unitOf(*definition, "g-ge-41-armour");
  const HexModel::HexIndex from = TrcFixture::hexOf(*definition, "F27");
  const HexModel::HexIndex to = TrcFixture::hexOf(*definition, "F26");
  const HexModel::SideId axis = definition->rules->side("axis");
  const HexModel::SpaceId pool = definition->board->spaceId("axis-pool");

  const std::vector<HexEngine::Event> events{
      HexEngine::PhaseEntered{1, definition->rules->phase("axis-i1-move"), axis},
      HexEngine::UnitMoved{unit, {from, to}},
      HexEngine::UnitPlaced{unit, to},
      HexEngine::UnitReduced{unit, 1},
      HexEngine::UnitEliminated{unit, pool},
      HexEngine::UnitRevealed{unit},
      HexEngine::CombatDeclared{{unit}, to},
      HexEngine::CombatResolved{to, "3-1", "DR"},
      HexEngine::Retreated{unit, {to}},
      HexEngine::ControlChanged{to, axis},
      HexEngine::SupplyChecked{unit, true},
      HexEngine::DieRolled{HexEngine::StreamTag::Combat, 4},
      HexEngine::CardDrawn{HexModel::RandomizerId{0}, "card-17"},
      HexEngine::DecisionRequested{"loss"},
      HexEngine::DecisionAnswered{"loss", "g-ge-41-armour"},
      HexEngine::VictoryDeclared{axis, "axis-moscow-stalin"},
      HexEngine::GameEvent{"weather", "snow"}};
  ASSERT_EQ(std::variant_size_v<HexEngine::Event>, events.size());

  std::set<std::string> kinds;
  for (const HexEngine::Event& event : events) {
    const std::string line = HexEngine::TextEventEncoder::line(event, names);
    EXPECT_FALSE(line.empty());
    EXPECT_EQ(std::string::npos, line.find('\n'));
    const HexEngine::EventFields fields = HexEngine::TextEventEncoder::fields(event, names);
    EXPECT_EQ(0u, line.rfind(fields.kind, 0));
    kinds.insert(fields.kind);
    // The same event renders the same line every time.
    EXPECT_EQ(line, HexEngine::TextEventEncoder::line(event, names));
  }
  EXPECT_EQ(events.size(), kinds.size());

  EXPECT_EQ("moved unit=g-ge-41-armour hex=F26 value=F27 F26",
            HexEngine::TextEventEncoder::line(events[1], names));
  EXPECT_EQ("die value=4 combat", HexEngine::TextEventEncoder::line(events[11], names));
}
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------

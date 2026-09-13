// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
#include "TrcRecordFixture.h"

#include <gtest/gtest.h>

#include <optional>
#include <string>
#include <vector>

TEST(ReplayTest, ScriptAppliesInOrder)
{
  const std::shared_ptr<const HexRules::GameDefinition> definition = TrcRecord::definition();
  const HexEngine::DefaultPolicySet defaults(*definition);
  const HexRecord::Record record =
      HexRecord::readRecord(TrcRecord::script(), *definition, *defaults.policies().grammar);

  EXPECT_EQ(HexRecord::Kind::Script, record.kind);
  EXPECT_EQ("trc", record.game);
  EXPECT_EQ(20260912u, record.seed);
  ASSERT_EQ(6u, record.log.size());
  // The scenario the script names supplies its position: eleven counters on the board.
  EXPECT_EQ(11u, [&] {
    std::size_t placed = 0;
    for (const HexModel::UnitSpec& spec : definition->roster->units()) {
      if (record.position.unit(spec.id).where.has_value()) {
        ++placed;
      }
    }
    return placed;
  }());

  HexEngine::Session session = HexRecord::sessionFor(record, definition, defaults.policies());
  const std::vector<HexRecord::ScriptedMove> played = HexRecord::playRecord(session, record);

  ASSERT_EQ(6u, played.size());
  for (std::size_t i = 0; i < played.size(); ++i) {
    EXPECT_EQ(static_cast<int>(i) + 1, played[i].n);
    EXPECT_EQ("axis", played[i].side);
    EXPECT_FALSE(played[i].events.empty());
  }
  EXPECT_EQ("axis-i1-move", played[0].phase);
  EXPECT_EQ("axis-i1-combat", played[3].phase);
  ASSERT_TRUE(played[3].outcome.has_value());
  EXPECT_EQ("EX", *played[3].outcome);
  ASSERT_EQ(1u, played[3].draws.size());
  EXPECT_EQ("combat#1=5", played[3].draws.front());

  // The same script from the same seed lands on the same position, every time.
  HexEngine::Session again = HexRecord::sessionFor(record, definition, defaults.policies());
  HexRecord::playRecord(again, record);
  EXPECT_EQ(session.position().digest(), again.position().digest());
}

TEST(ReplayTest, StrictModeFindsFirstDivergence)
{
  const std::shared_ptr<const HexRules::GameDefinition> definition = TrcRecord::definition();
  const HexEngine::DefaultPolicySet defaults(*definition);

  // The golden replays cleanly against itself.
  {
    HexRecord::Record record =
        HexRecord::readRecord(TrcRecord::golden(), *definition, *defaults.policies().grammar);
    record.position = HexRecord::readRecord(TrcRecord::script(), *definition,
                                             *defaults.policies().grammar)
                           .position;
    HexEngine::Session session = HexRecord::sessionFor(record, definition, defaults.policies());
    EXPECT_FALSE(HexRecord::replay(session, record, true).has_value());
  }

  // A golden whose fourth move records a different die is caught on that move.
  {
    HexRecord::Record record =
        HexRecord::readRecord(TrcRecord::golden(), *definition, *defaults.policies().grammar);
    record.position = HexRecord::readRecord(TrcRecord::script(), *definition,
                                             *defaults.policies().grammar)
                           .position;
    ASSERT_EQ(6u, record.log.size());
    ASSERT_EQ(1u, record.log[3].draws.size());
    record.log[3].draws[0] = "combat#1=4";

    HexEngine::Session session = HexRecord::sessionFor(record, definition, defaults.policies());
    const std::optional<HexRecord::Divergence> divergence = HexRecord::replay(session, record, true);
    ASSERT_TRUE(divergence.has_value());
    EXPECT_EQ(4, divergence->moveNumber);
    EXPECT_EQ("draw", divergence->what);
    EXPECT_EQ("combat#1=4", divergence->expected);
    EXPECT_EQ("combat#1=5", divergence->actual);
  }
}
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------

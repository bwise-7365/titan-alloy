// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
#include "TrcFixture.h"

#include "hexengine/Defaults.h"

#include <gtest/gtest.h>

#include <algorithm>
#include <optional>
#include <span>
#include <stdexcept>
#include <vector>

namespace {

  using HexModel::HexIndex;
  using HexModel::UnitId;

  bool
  holdsP(const std::vector<HexIndex>& hexes, HexIndex wanted)
  {
    return hexes.end() != std::find(hexes.begin(), hexes.end(), wanted);
  }

}  // namespace

TEST(SessionTest, MoveThroughReachable)
{
  const std::shared_ptr<const HexRules::GameDefinition> definition = TrcFixture::definition();
  const HexEngine::DefaultPolicySet defaults(*definition);
  HexEngine::Session session(definition, defaults.policies(), TrcFixture::scenario(*definition), 20260912ull);

  const UnitId armour = TrcFixture::unitOf(*definition, "g-ge-41-armour");
  const std::span<const UnitId> one(&armour, 1);
  const HexEngine::Reachability field = session.reachable(one, HexModel::ModeId{0});

  ASSERT_FALSE(field.hexes.empty());
  EXPECT_EQ(field.hexes.size(), field.paths.size());
  EXPECT_EQ(field.hexes.size(), field.costs.size());
  const HexIndex start = TrcFixture::hexOf(*definition, "F27");
  for (std::size_t i = 0; i < field.hexes.size(); ++i) {
    // Nothing beyond the counter's printed seven hexes, and nothing on the sea.
    EXPECT_GE(HexModel::MovementPoints::whole(7), field.costs[i]);
    const HexRules::Terrain& terrain =
        definition->rules->hexTerrain()[definition->board->terrain(field.hexes[i]).value];
    EXPECT_FALSE(std::holds_alternative<HexModel::Prohibited>(terrain.moveCost))
        << definition->board->id(field.hexes[i]).text;
    EXPECT_EQ(start, field.paths[i].front());
    EXPECT_EQ(field.hexes[i], field.paths[i].back());
  }

  const std::size_t pick = field.hexes.size() / 2;
  const HexEngine::Applied applied =
      session.apply(HexEngine::MoveUnit{{armour}, HexModel::ModeId{0}, field.paths[pick]});
  EXPECT_LT(0u, applied.eventCount);
  EXPECT_TRUE(std::holds_alternative<HexEngine::UnitMoved>(session.events().events()[applied.firstEvent]));
  EXPECT_EQ(HexModel::Location(field.hexes[pick]), *session.position().unit(armour).where);

  // A hex the field does not list is not a legal destination, however plausible it looks.
  const std::vector<HexIndex> unreachable{TrcFixture::hexOf(*definition, "P12")};
  EXPECT_FALSE(holdsP(field.hexes, unreachable.front()));
  EXPECT_THROW((void)session.apply(HexEngine::MoveUnit{
                    {armour}, HexModel::ModeId{0}, {field.hexes[pick], unreachable.front()}}),
                std::invalid_argument);
}

TEST(SessionTest, IllegalCommandLeavesPositionUntouched)
{
  const std::shared_ptr<const HexRules::GameDefinition> definition = TrcFixture::definition();
  const HexEngine::DefaultPolicySet defaults(*definition);
  HexEngine::Session session(definition, defaults.policies(), TrcFixture::scenario(*definition), 20260912ull);

  const std::uint64_t before = session.position().digest();
  const std::size_t events = session.events().size();

  // A Russian unit in the Axis movement phase.
  const UnitId russian = TrcFixture::unitOf(*definition, "r-ru-11-infantry");
  EXPECT_THROW((void)session.apply(HexEngine::MoveUnit{{russian},
                                                        HexModel::ModeId{0},
                                                        {TrcFixture::hexOf(*definition, "F24"),
                                                         TrcFixture::hexOf(*definition, "F25")}}),
                std::invalid_argument);
  EXPECT_EQ(before, session.position().digest());
  EXPECT_EQ(events, session.events().size());

  // An attack in a movement phase.
  const UnitId armour = TrcFixture::unitOf(*definition, "g-ge-41-armour");
  EXPECT_THROW((void)session.apply(HexEngine::DeclareAttack{
                    {armour}, TrcFixture::hexOf(*definition, "F24"), {}}),
                std::invalid_argument);
  EXPECT_EQ(before, session.position().digest());
  EXPECT_EQ(events, session.events().size());
}

TEST(SessionTest, PromptAndPhaseEnd)
{
  const std::shared_ptr<const HexRules::GameDefinition> definition = TrcFixture::definition();
  const HexEngine::DefaultPolicySet defaults(*definition);
  HexEngine::Session session(definition, defaults.policies(), TrcFixture::scenario(*definition), 20260912ull);

  const HexEngine::Prompt start = session.prompt();
  EXPECT_EQ(1, start.turn);
  EXPECT_EQ(definition->rules->phase("axis-i1-move"), start.phase);
  ASSERT_TRUE(start.side.has_value());
  EXPECT_EQ(definition->rules->side("axis"), *start.side);
  EXPECT_FALSE(start.decisionPendingP);
  EXPECT_FALSE(start.overP);

  // Ending the phase is always among the legal commands.
  const std::vector<HexEngine::Command> legal = session.legalCommands();
  ASSERT_FALSE(legal.empty());
  EXPECT_TRUE(std::holds_alternative<HexEngine::EndPhase>(legal.back()));

  session.apply(HexEngine::EndPhase{});
  EXPECT_EQ(definition->rules->phase("axis-i1-combat"), session.prompt().phase);
}

TEST(SessionTest, PendingDecisionGate)
{
  const std::shared_ptr<const HexRules::GameDefinition> definition = TrcFixture::definition();
  const HexEngine::DefaultPolicySet defaults(*definition);
  HexModel::Position position = TrcFixture::scenario(*definition);

  // Two German corps against one Russian, so that an attacker's loss is a choice rather than a
  // foregone conclusion. F25 is adjacent to F26 and G26 on the TRC lattice.
  const UnitId first = TrcFixture::unitOf(*definition, "g-ge-41-armour");
  const UnitId second = TrcFixture::unitOf(*definition, "g-ge-56-armour");
  const UnitId target = TrcFixture::unitOf(*definition, "r-ru-11-infantry");
  const HexIndex defended = TrcFixture::hexOf(*definition, "F25");
  const HexModel::Board& board = *definition->board;
  std::vector<HexIndex> neighbours;
  for (int d = 0; d < HexCoord::kDirections; ++d) {
    if (const std::optional<HexIndex> n = board.neighbour(defended, static_cast<HexModel::Direction>(d))) {
      neighbours.push_back(*n);
    }
  }
  ASSERT_LE(2u, neighbours.size());
  position.place(first, neighbours[0]);
  position.place(second, neighbours[1]);
  position.place(target, defended);
  position.clock().phase = definition->rules->phase("axis-i1-combat");

  // The seed decides the row of the table, so the test picks the first seed whose result costs the
  // attacker a step: with two corps attacking, that step is a choice and becomes a ChooseLoss.
  std::optional<HexEngine::Session> asking;
  for (std::uint64_t seed = 1; seed < 200 && !asking; ++seed) {
    HexEngine::Session trial(definition, defaults.policies(), position, seed);
    trial.apply(HexEngine::DeclareAttack{{first, second}, defended, {}});
    if (trial.prompt().decisionPendingP) {
      asking.emplace(std::move(trial));
    }
  }
  ASSERT_TRUE(asking.has_value()) << "no seed in the first two hundred cost the attacker a step";
  HexEngine::Session& session = *asking;
  EXPECT_THROW((void)session.apply(HexEngine::EndPhase{}), std::invalid_argument);

  const std::vector<HexEngine::Command> legal = session.legalCommands();
  ASSERT_FALSE(legal.empty());
  for (const HexEngine::Command& command : legal) {
    EXPECT_TRUE(std::holds_alternative<HexEngine::DecisionAnswer>(command));
  }
  session.apply(legal.front());
  EXPECT_FALSE(session.prompt().decisionPendingP);
}

TEST(SessionTest, ForkIsIndependent)
{
  const std::shared_ptr<const HexRules::GameDefinition> definition = TrcFixture::definition();
  const HexEngine::DefaultPolicySet defaults(*definition);
  HexEngine::Session session(definition, defaults.policies(), TrcFixture::scenario(*definition), 20260912ull);

  HexEngine::Session forked = session.fork();
  EXPECT_EQ(session.position().digest(), forked.position().digest());
  forked.apply(HexEngine::EndPhase{});
  EXPECT_NE(session.position().digest(), forked.position().digest());
  EXPECT_EQ(0u, session.events().size());
}
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------

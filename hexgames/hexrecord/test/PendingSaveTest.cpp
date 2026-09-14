// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
// A save written while a decision is pending reloads asking it (hexsave <resolution>, M6b review):
// the reloaded session offers the same answers to the same side, and answering the first of them
// leaves both sessions owing the same. A game's own obligation round-trips through the game's
// ObligationCodec, and a policy set without one refuses to write it, naming it.
// ----------------------------------------------
#include "TrcRecordFixture.h"

#include <gtest/gtest.h>

#include <filesystem>
#include <optional>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

  using HexModel::HexIndex;
  using HexModel::UnitId;

  UnitId
  unitOf(const HexRules::GameDefinition& definition, const char* counter)
  {
    return *definition.roster->find(HexModel::CounterId{counter});
  }

  // Every legal command as its text: verb and arguments.
  std::vector<std::string>
  spell(const HexEngine::Session& session)
  {
    std::vector<std::string> out;
    const HexEngine::CommandGrammar& grammar = *session.policies().grammar;
    for (const HexEngine::Command& command : session.legalCommands()) {
      std::string line = grammar.verb(command);
      for (const auto& [name, value] : grammar.arguments(command)) {
        line += " " + name + "=" + value;
      }
      out.push_back(line);
    }
    return out;
  }

  std::filesystem::path
  scratchFile(const char* name)
  {
    return std::filesystem::temp_directory_path() / name;
  }

  struct Consult : HexModel::Cloneable<Consult, HexModel::GameObligation> {
    int rounds = 0;
    std::string_view kind() const override { return "consult"; }
    void appendDigest(std::string& s) const override { s += std::to_string(rounds); }
  };

  class ConsultCodec : public HexModel::ObligationCodec {
  public:
    HexModel::Polymorphic<HexModel::GameObligation>
    decode(const std::string& name, const std::vector<HexModel::ObligationArg>& args) const override
    {
      if ("consult" != name || 1 != args.size() || "rounds" != args.front().name) {
        throw std::invalid_argument("ConsultCodec: cannot read obligation '" + name + "'");
      }
      Consult consult;
      consult.rounds = std::stoi(args.front().value);
      return HexModel::makePolymorphic<HexModel::GameObligation, Consult>(consult);
    }
    std::vector<HexModel::ObligationArg>
    encode(const HexModel::GameObligation& owed) const override
    {
      const Consult* consult = dynamic_cast<const Consult*>(&owed);
      if (nullptr == consult) {
        throw std::invalid_argument("ConsultCodec: cannot write obligation '" + std::string(owed.kind()) + "'");
      }
      return {HexModel::ObligationArg{"rounds", std::to_string(consult->rounds)}};
    }
  };

}  // namespace

TEST(PendingSaveTest, SaveMidDecisionReloadsTheDecision)
{
  const std::shared_ptr<const HexRules::GameDefinition> definition = TrcRecord::definition();
  const HexEngine::DefaultPolicySet defaults(*definition, HexEngine::GameSteps::Withheld);
  const HexEngine::CommandGrammar& grammar = *defaults.policies().grammar;
  HexModel::Position position = HexRecord::readRecord(TrcRecord::script(), *definition, defaults.policies()).position;

  // Two German corps against one Russian (as SessionTest.PendingDecisionGate): the first seed
  // whose result leaves a choice gives a session stopped on a decision.
  const UnitId first = unitOf(*definition, "g-ge-41-armour");
  const UnitId second = unitOf(*definition, "g-ge-56-armour");
  const UnitId target = unitOf(*definition, "r-ru-11-infantry");
  const HexIndex defended = definition->board->indexOf(HexCoord::HexId{"F25"});
  std::vector<HexIndex> neighbours;
  for (int d = 0; d < HexCoord::kDirections; ++d) {
    if (const std::optional<HexIndex> n = definition->board->neighbour(defended, static_cast<HexModel::Direction>(d))) {
      neighbours.push_back(*n);
    }
  }
  position.place(first, neighbours[0]);
  position.place(second, neighbours[1]);
  position.place(target, defended);
  position.clock().phase = definition->rules->phase("axis-i1-combat");

  std::optional<HexEngine::Session> asking;
  for (std::uint64_t seed = 1; seed < 200 && !asking; ++seed) {
    HexEngine::Session trial(definition, defaults.policies(), position, seed);
    trial.apply(HexEngine::DeclareAttack{{first, second}, defended, {}});
    if (trial.prompt().decisionPendingP) {
      asking.emplace(std::move(trial));
    }
  }
  ASSERT_TRUE(asking.has_value());

  const std::filesystem::path saved = scratchFile("hexgames-pending-save.xml");
  HexRecord::writeRecord(saved, HexRecord::Kind::Save, *asking, {}, grammar);
  const HexRecord::Record reloaded = HexRecord::readRecord(saved, *definition, defaults.policies());
  std::filesystem::remove(saved);

  ASSERT_EQ(asking->position().resolution().size(), reloaded.position.resolution().size());
  HexEngine::Session again = HexRecord::sessionFor(reloaded, definition, defaults.policies());
  ASSERT_TRUE(again.prompt().decisionPendingP);
  EXPECT_EQ(asking->prompt().side, again.prompt().side);
  const std::vector<std::string> offered = spell(*asking);
  EXPECT_EQ(offered, spell(again));

  // Answering the same way works both stacks down alike.
  asking->apply(asking->legalCommands().front());
  again.apply(again.legalCommands().front());
  EXPECT_EQ(asking->position().resolution().size(), again.position().resolution().size());
  EXPECT_EQ(spell(*asking), spell(again));
}

TEST(PendingSaveTest, GameObligationsNeedTheGameCodec)
{
  const std::shared_ptr<const HexRules::GameDefinition> definition = TrcRecord::definition();
  const HexEngine::DefaultPolicySet defaults(*definition, HexEngine::GameSteps::Withheld);
  const ConsultCodec codec;
  HexEngine::Policies policies = defaults.policies();
  policies.obligationCodec = &codec;

  HexModel::Position position = HexRecord::readRecord(TrcRecord::script(), *definition, policies).position;
  Consult consult;
  consult.rounds = 2;
  position.push(HexModel::makePolymorphic<HexModel::GameObligation, Consult>(consult));
  position.ask(HexModel::GameChoice{"consult", {"again", "done"}});

  const std::filesystem::path saved = scratchFile("hexgames-game-obligation.xml");
  {
    const HexEngine::Session session(definition, policies, position, 1ull);
    HexRecord::writeRecord(saved, HexRecord::Kind::Save, session, {}, *policies.grammar);
  }
  const HexRecord::Record reloaded = HexRecord::readRecord(saved, *definition, policies);
  std::filesystem::remove(saved);

  ASSERT_EQ(1u, reloaded.position.resolution().size());
  const auto& owed = std::get<HexModel::Polymorphic<HexModel::GameObligation>>(reloaded.position.top().owed);
  EXPECT_EQ(2, owed.as<Consult>("the reloaded obligation").rounds);
  ASSERT_TRUE(std::holds_alternative<HexModel::GameChoice>(reloaded.position.pending()));
  EXPECT_EQ((std::vector<std::string>{"again", "done"}), std::get<HexModel::GameChoice>(reloaded.position.pending()).options);

  // Without the game's codec the obligation is refused by name.
  const HexEngine::Session plain(definition, defaults.policies(), position, 1ull);
  try {
    HexRecord::writeRecord(saved, HexRecord::Kind::Save, plain, {}, *policies.grammar);
    FAIL() << "a game obligation was written without its codec";
  } catch (const std::invalid_argument& e) {
    EXPECT_NE(std::string::npos, std::string(e.what()).find("'consult'")) << e.what();
  }
  std::filesystem::remove(saved);
}
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------

// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
// The resolution stack with a game-defined obligation under an engine one: answering the loss on
// top works the stack down to the game's obligation, which asks its own question through the
// ObligationPolicy, and answering that empties the stack. A policy set without an ObligationPolicy
// refuses, and a fork copies the game's obligation as a value.
// ----------------------------------------------
#include "TrcFixture.h"

#include "hexengine/Defaults.h"

#include <gtest/gtest.h>

#include <stdexcept>
#include <string>

namespace {

  using HexModel::UnitId;

  // Tarawa's shape: consult the table again until the player is done.
  struct Consult : HexModel::Cloneable<Consult, HexModel::GameObligation> {
    int rounds = 0;
    std::string_view kind() const override { return "consult"; }
    void appendDigest(std::string& s) const override { s += std::to_string(rounds); }
  };

  class ConsultPolicy : public HexEngine::ObligationPolicy {
  public:
    std::span<const std::string_view> claims() const override { return {}; }

    HexModel::Position
    settle(const HexEngine::Ctx& ctx, HexEngine::PrngStreams&, HexEngine::EventSink& sink) const override
    {
      HexModel::Position next = ctx.position;
      next.ask(HexModel::GameChoice{"consult", {"again", "done"}});
      sink.onEvent(HexEngine::DecisionRequested{"consult"});
      return next;
    }

    HexModel::Position
    answer(const HexEngine::Ctx& ctx, const HexEngine::DecisionAnswer& answer, HexEngine::PrngStreams&,
           HexEngine::EventSink&) const override
    {
      HexModel::Position next = ctx.position;
      next.ask(HexModel::NoDecision{});
      if ("done" == answer.answer) {
        next.pop();
        return next;
      }
      auto& owed = std::get<HexModel::Polymorphic<HexModel::GameObligation>>(next.top().owed);
      owed.as<Consult>("ConsultPolicy: the top obligation").rounds += 1;
      return next;
    }
  };

  // A Consult at the bottom, and on top an Axis loss of one step between two armour corps.
  HexModel::Position
  owing(const HexRules::GameDefinition& definition)
  {
    HexModel::Position position = TrcFixture::scenario(definition);
    const UnitId first = TrcFixture::unitOf(definition, "g-ge-41-armour");
    const UnitId second = TrcFixture::unitOf(definition, "g-ge-56-armour");
    const HexModel::SideId axis = definition.rules->side("axis");
    position.push(HexModel::makePolymorphic<HexModel::GameObligation, Consult>());
    position.push(HexModel::OwedLoss{HexModel::Battle{std::nullopt, {first, second}}, axis, 1});
    position.ask(HexModel::ChooseLoss{axis, {first, second}, 1});
    return position;
  }

}  // namespace

TEST(ResolutionStackTest, GameObligationUnderAnEngineLoss)
{
  const std::shared_ptr<const HexRules::GameDefinition> definition = TrcFixture::definition();
  const HexEngine::DefaultPolicySet defaults(*definition, HexEngine::GameSteps::Withheld);
  const ConsultPolicy consult;
  HexEngine::Policies policies = defaults.policies();
  policies.obligations = &consult;
  HexEngine::Session session(definition, policies, owing(*definition), 20260914ull);

  session.apply(HexEngine::DecisionAnswer{"loss", "g-ge-41-armour"});
  ASSERT_EQ(1u, session.position().resolution().size());
  ASSERT_TRUE(std::holds_alternative<HexModel::GameChoice>(session.position().pending()));
  EXPECT_THROW((void)session.apply(HexEngine::DecisionAnswer{"consult", "maybe"}), std::invalid_argument);

  session.apply(HexEngine::DecisionAnswer{"consult", "again"});
  const auto& owed =
      std::get<HexModel::Polymorphic<HexModel::GameObligation>>(session.position().top().owed);
  EXPECT_EQ(1, owed.as<Consult>("the top obligation").rounds);
  EXPECT_TRUE(std::holds_alternative<HexModel::GameChoice>(session.position().pending()));

  // A fork copies the obligation; answering in the fork leaves the original owing.
  HexEngine::Session forked = session.fork();
  EXPECT_EQ(session.position().digest(), forked.position().digest());
  forked.apply(HexEngine::DecisionAnswer{"consult", "done"});
  EXPECT_TRUE(forked.position().resolution().empty());
  EXPECT_EQ(1u, session.position().resolution().size());
}

TEST(ResolutionStackTest, GameObligationWithoutAPolicyIsRefused)
{
  const std::shared_ptr<const HexRules::GameDefinition> definition = TrcFixture::definition();
  const HexEngine::DefaultPolicySet defaults(*definition, HexEngine::GameSteps::Withheld);
  HexEngine::Session session(definition, defaults.policies(), owing(*definition), 20260914ull);

  const std::uint64_t before = session.position().digest();
  EXPECT_THROW((void)session.apply(HexEngine::DecisionAnswer{"loss", "g-ge-41-armour"}), std::invalid_argument);
  EXPECT_EQ(before, session.position().digest());
}
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------

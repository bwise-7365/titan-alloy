// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
// The resolution stack with a game-defined obligation under an engine one: answering the loss on
// top works the stack down to the game's obligation, which asks its own question through the
// ObligationPolicy, and answering that empties the stack. A policy set without an ObligationPolicy
// refuses, and a fork copies the game's obligation as a value.
// ----------------------------------------------
#include "TrcFixture.h"

#include "hexengine/Adjudicators.h"
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

namespace {

  // A result the game settles itself (M7b): the resolver owes a Consult instead of a loss or a retreat.
  class OwingResolver : public HexEngine::CombatResolver {
  public:
    std::span<const std::string_view> claims() const override { return {}; }
    std::vector<HexEngine::CombatEffect>
    resolve(const HexEngine::Ctx& ctx, const HexEngine::CombatContext& combat, HexEngine::PrngStreams& streams) const override
    {
      return report(ctx, combat, streams).effects;
    }
    HexEngine::CombatReport
    report(const HexEngine::Ctx&, const HexEngine::CombatContext&, HexEngine::PrngStreams&) const override
    {
      HexEngine::CombatReport out{"1-1", "owed", {}, {}};
      out.effects.push_back(HexEngine::OweEffect{HexModel::makePolymorphic<HexModel::GameObligation, Consult>()});
      return out;
    }
  };

  struct NullSink : HexEngine::EventSink {
    void onEvent(const HexEngine::Event&) override { return; }
  };

}  // namespace

TEST(ResolutionStackTest, AnAttackCanOweAGameObligation)
{
  const std::shared_ptr<const HexRules::GameDefinition> definition = TrcFixture::definition();
  const HexEngine::DefaultPolicySet defaults(*definition, HexEngine::GameSteps::Withheld);
  const ConsultPolicy consult;
  const OwingResolver owing;
  HexEngine::Policies policies = defaults.policies();
  policies.obligations = &consult;
  policies.combat = &owing;

  HexModel::Position position = TrcFixture::scenario(*definition);
  const UnitId attacker = TrcFixture::unitOf(*definition, "g-ge-41-armour");
  const HexModel::HexIndex target =
      *definition->board->neighbour(TrcFixture::hexOf(*definition, "F27"), HexModel::Direction::D0);
  position.place(TrcFixture::unitOf(*definition, "r-ru-11-infantry"), target);
  const HexEngine::Ctx ctx{*definition->board, *definition->rules, *definition->roster, position};
  HexEngine::PrngStreams streams(1ull);
  NullSink sink;
  const HexModel::Position next =
      HexEngine::Adjudicators::applyAttack(ctx, policies, HexEngine::DeclareAttack{{attacker}, target, {}}, streams, sink);

  ASSERT_EQ(1u, next.resolution().size());
  EXPECT_EQ("consult", std::get<HexModel::Polymorphic<HexModel::GameObligation>>(next.top().owed).base().kind());
  EXPECT_TRUE(std::holds_alternative<HexModel::GameChoice>(next.pending()));  // settled at once: it asked
}

TEST(ResolutionStackTest, AGameChoiceNamesTheSideThatAnswers)
{
  const std::shared_ptr<const HexRules::GameDefinition> definition = TrcFixture::definition();
  const HexEngine::DefaultPolicySet defaults(*definition, HexEngine::GameSteps::Withheld);
  const HexModel::SideId russian = definition->rules->side("russian");
  HexModel::Position position = TrcFixture::scenario(*definition);
  position.push(HexModel::makePolymorphic<HexModel::GameObligation, Consult>());
  position.ask(HexModel::GameChoice{"consult", {"again", "done"}});
  const HexEngine::Session acting(definition, defaults.policies(), position, 1ull);
  EXPECT_EQ(definition->rules->side("axis"), acting.prompt().side);  // nullopt: the acting side answers

  position.ask(HexModel::GameChoice{"consult", {"again", "done"}, russian});
  const HexEngine::Session other(definition, defaults.policies(), position, 1ull);
  EXPECT_EQ(russian, other.prompt().side);
  EXPECT_NE(acting.position().digest(), other.position().digest());
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

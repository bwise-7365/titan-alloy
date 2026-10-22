// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
#include "PggObligations.h"

#include "PggBattleFlow.h"

#include <array>

namespace Pgg {

  namespace {

    constexpr std::array<std::string_view, 6> kClaims{"split-results",        "overrun-disruption", "advance-after-combat",
                                                      "no-advance-overrun",   "no-strength-units",  "untried-reveal-timing"};

  }  // namespace

  PggObligations::PggObligations(const PggFacts& facts, const PggRetreat& retreat, const PggStacking& stacking)
    : facts_(facts), retreat_(retreat), stacking_(stacking)
  {
  }

  std::span<const std::string_view>
  PggObligations::claims() const
  {
    return kClaims;
  }

  Position
  PggObligations::settle(const Ctx& ctx, HexEngine::PrngStreams&, HexEngine::EventSink& sink) const
  {
    Position next = ctx.position;
    PggBattle battle = battleOn(next);
    const BattleFlow::Env env{facts_, retreat_, stacking_, ctx, sink};
    BattleFlow::step(env, next, battle);
    if (BattleStage::Done == battle.stage) {
      next.pop();
      return next;
    }
    battleOn(next) = std::move(battle);
    return next;
  }

  Position
  PggObligations::answer(const Ctx& ctx, const HexEngine::DecisionAnswer& given, HexEngine::PrngStreams&,
                         HexEngine::EventSink& sink) const
  {
    Position next = ctx.position;
    const HexModel::PendingDecision asked = next.pending();
    PggBattle battle = battleOn(next);
    next.ask(HexModel::NoDecision{});
    const BattleFlow::Env env{facts_, retreat_, stacking_, ctx, sink};
    BattleFlow::answer(env, next, battle, asked, given);
    if (BattleStage::Done == battle.stage) {
      next.pop();
      return next;
    }
    battleOn(next) = std::move(battle);
    return next;
  }

}  // namespace Pgg
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------

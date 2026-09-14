// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
// Working the resolution stack down (M6b; M6's combat plan, PLAN.md obligation b). The top entry
// either settles itself -- a loss with no real choice, a retreat with one way to go -- or asks a
// decision, answered by the side that owes the loss or by the router. A game's own obligation on
// top goes to the policy set's ObligationPolicy.
// ----------------------------------------------
#include "hexengine/AdjudicatorsDetail.h"

#include <algorithm>
#include <stdexcept>
#include <string>
#include <type_traits>

namespace HexEngine::Adjudicators {

  using Detail::boxFor;
  using Detail::hexOf;
  using Detail::loseStep;
  using Detail::lossOrder;
  using Detail::removeToBox;

  namespace {

    enum class Progress : std::uint8_t { Continue, Asked };

    bool
    walkedP(const HexModel::UnitRetreat& walk, HexIndex hex)
    {
      return hex == walk.from || walk.path.end() != std::find(walk.path.begin(), walk.path.end(), hex);
    }

    std::vector<HexIndex>
    freshCandidates(const Ctx& ctx, const Policies& policies, const HexModel::UnitRetreat& walk, HexIndex here)
    {
      std::vector<HexIndex> out;
      for (HexIndex hex : policies.retreat->candidates(ctx, walk.unit, here)) {
        if (!walkedP(walk, hex)) {
          out.push_back(hex);
        }
      }
      return out;
    }

    // Whether a walk that has reached `hex` can still go `more` hexes, trying each candidate with
    // the unit standing there; a retreat is not routed into elimination while another way exists.
    bool
    completableP(const Ctx& ctx, const Policies& policies, HexModel::UnitRetreat walk, HexIndex hex, int more)
    {
      if (0 >= more) {
        return true;
      }
      Position probe = ctx.position;
      probe.place(walk.unit, hex);
      walk.path.push_back(hex);
      const Ctx there{ctx.board, ctx.rules, ctx.roster, probe};
      for (HexIndex onward : freshCandidates(there, policies, walk, hex)) {
        if (completableP(there, policies, walk, onward, more - 1)) {
          return true;
        }
      }
      return false;
    }

    Progress
    settleLoss(const Ctx& ctx, Position& next, const HexModel::OwedLoss& owed, EventSink& sink)
    {
      const Ctx here{ctx.board, ctx.rules, ctx.roster, next};
      const std::vector<UnitId> order = lossOrder(here, next, owed.battle.involved, owed.side);
      if (0 >= owed.count || order.empty()) {
        next.pop();
        return Progress::Continue;
      }
      if (static_cast<int>(order.size()) <= owed.count) {
        // Every candidate loses a step, so there is nothing to choose.
        for (UnitId unit : order) {
          loseStep(here, next, unit, boxFor(ctx, owed.side, true), sink);
        }
        std::get<HexModel::OwedLoss>(next.top().owed).count -= static_cast<int>(order.size());
        return Progress::Continue;
      }
      next.ask(HexModel::ChooseLoss{owed.side, order, owed.count});
      sink.onEvent(DecisionRequested{"loss"});
      return Progress::Asked;
    }

    // One walk per unit of the side still on the map, the first involved unit on top.
    void
    expandRetreat(const Ctx& ctx, Position& next, const HexModel::OwedRetreat& owed)
    {
      std::vector<HexModel::UnitRetreat> walks;
      for (UnitId unit : owed.battle.involved) {
        if (owed.side != ctx.roster.unit(unit).side) {
          continue;
        }
        if (const std::optional<HexIndex> hex = hexOf(next, unit)) {
          walks.push_back(HexModel::UnitRetreat{owed.battle, unit, *hex, owed.fewest, owed.most, {}});
        }
      }
      next.pop();
      for (auto walk = walks.rbegin(); walks.rend() != walk; ++walk) {
        next.push(*walk);
      }
      return;
    }

    Progress
    finishWalk(Position& next, const HexModel::UnitRetreat& walk, EventSink& sink)
    {
      if (!walk.path.empty()) {
        sink.onEvent(Retreated{walk.unit, walk.path});
      }
      next.pop();
      return Progress::Continue;
    }

    // Before the first hex: what the RetreatPolicy says the unit does. nullopt: it walks.
    std::optional<Progress>
    settleFate(const Ctx& ctx, const Policies& policies, Position& next, const HexModel::UnitRetreat& walk,
               EventSink& sink)
    {
      const Ctx now{ctx.board, ctx.rules, ctx.roster, next};
      switch (policies.retreat->fate(now, walk.unit)) {
        case RetreatFate::Walk:
          return std::nullopt;
        case RetreatFate::Stay:
          sink.onEvent(GameEvent{"holds", ctx.roster.unit(walk.unit).counter.text});
          next.pop();
          return Progress::Continue;
        case RetreatFate::Surrender:
          removeToBox(next, walk.unit, boxFor(ctx, ctx.roster.unit(walk.unit).side, false), sink);
          next.pop();
          return Progress::Continue;
      }
      throw std::invalid_argument("settleStack: retreat fate outside Walk, Stay and Surrender");
    }

    Progress
    settleWalk(const Ctx& ctx, const Policies& policies, Position& next, EventSink& sink)
    {
      const HexModel::UnitRetreat walk = std::get<HexModel::UnitRetreat>(next.top().owed);
      const SideId owner = ctx.roster.unit(walk.unit).side;
      const std::optional<HexIndex> here = hexOf(next, walk.unit);
      if (!here) {
        next.pop();
        return Progress::Continue;
      }
      if (walk.path.empty()) {
        if (const std::optional<Progress> fated = settleFate(ctx, policies, next, walk, sink)) {
          return *fated;
        }
      }
      if (static_cast<int>(walk.path.size()) >= walk.most) {
        return finishWalk(next, walk, sink);
      }

      const Ctx now{ctx.board, ctx.rules, ctx.roster, next};
      const bool mayStopP = static_cast<int>(walk.path.size()) >= walk.fewest;
      std::vector<HexIndex> options = freshCandidates(now, policies, walk, *here);
      const int more = walk.fewest - static_cast<int>(walk.path.size()) - 1;
      std::vector<HexIndex> completing;
      for (HexIndex hex : options) {
        if (completableP(now, policies, walk, hex, more)) {
          completing.push_back(hex);
        }
      }
      if (!completing.empty()) {
        options = completing;
      }

      if (options.empty()) {
        if (mayStopP) {
          return finishWalk(next, walk, sink);
        }
        // Unsatisfiable: the rules document's retreat/@unsatisfiable is "eliminate" in every game in view.
        removeToBox(next, walk.unit, boxFor(ctx, owner, true), sink);
        next.pop();
        return Progress::Continue;
      }
      if (1 == options.size() && !mayStopP) {
        next.place(walk.unit, options.front());
        std::get<HexModel::UnitRetreat>(next.top().owed).path.push_back(options.front());
        return Progress::Continue;
      }
      next.ask(HexModel::ChooseRetreat{walk.battle.router.value_or(owner), walk.unit, options, mayStopP});
      sink.onEvent(DecisionRequested{"retreat"});
      return Progress::Asked;
    }

    Progress
    settleGame(const Ctx& ctx, const Policies& policies, Position& next, PrngStreams& streams, EventSink& sink)
    {
      if (nullptr == policies.obligations) {
        const auto& owed = std::get<HexModel::Polymorphic<HexModel::GameObligation>>(next.top().owed);
        throw std::invalid_argument("settleStack: game obligation '" + std::string(owed.base().kind()) +
                                    "' is on the resolution stack and the policy set has no ObligationPolicy");
      }
      next = policies.obligations->settle(Ctx{ctx.board, ctx.rules, ctx.roster, next}, streams, sink);
      return std::holds_alternative<HexModel::NoDecision>(next.pending()) ? Progress::Continue : Progress::Asked;
    }

    Progress
    settleTop(const Ctx& ctx, const Policies& policies, Position& next, PrngStreams& streams, EventSink& sink)
    {
      const HexModel::Obligation& head = next.top().owed;
      if (const HexModel::OwedLoss* owed = std::get_if<HexModel::OwedLoss>(&head)) {
        return settleLoss(ctx, next, HexModel::OwedLoss(*owed), sink);
      }
      if (const HexModel::OwedRetreat* retreat = std::get_if<HexModel::OwedRetreat>(&head)) {
        expandRetreat(ctx, next, HexModel::OwedRetreat(*retreat));
        return Progress::Continue;
      }
      if (std::holds_alternative<HexModel::UnitRetreat>(head)) {
        return settleWalk(ctx, policies, next, sink);
      }
      return settleGame(ctx, policies, next, streams, sink);
    }

  }  // namespace

  Position
  settleStack(const Ctx& ctx, const Policies& policies, PrngStreams& streams, EventSink& sink)
  {
    if (nullptr == policies.retreat) {
      throw std::invalid_argument("settleStack: the policy set has no RetreatPolicy");
    }
    Position next = ctx.position;
    while (!next.resolution().empty()) {
      if (Progress::Asked == settleTop(ctx, policies, next, streams, sink)) {
        return next;
      }
    }
    return next;
  }

}  // namespace HexEngine::Adjudicators
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------

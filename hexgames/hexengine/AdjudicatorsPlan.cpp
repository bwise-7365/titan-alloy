// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
// Working a combat plan down (added in M6, PLAN.md obligation b). The head step either settles
// itself -- a loss with no real choice, a retreat with one way to go -- or becomes the position's
// PendingDecision, answered by the side that owes the loss or by the router.
// ----------------------------------------------
#include "hexengine/AdjudicatorsDetail.h"

#include <algorithm>
#include <stdexcept>
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
    settleLoss(const Ctx& ctx, Position& next, CombatPlan& plan, HexModel::OwedLoss owed, EventSink& sink)
    {
      const Ctx here{ctx.board, ctx.rules, ctx.roster, next};
      const std::vector<UnitId> order = lossOrder(here, next, plan.involved, owed.side);
      if (0 >= owed.count || order.empty()) {
        plan.steps.erase(plan.steps.begin());
        return Progress::Continue;
      }
      if (static_cast<int>(order.size()) <= owed.count) {
        // Every candidate loses a step, so there is nothing to choose.
        for (UnitId unit : order) {
          loseStep(here, next, unit, boxFor(ctx, owed.side, true), sink);
        }
        std::get<HexModel::OwedLoss>(plan.steps.front()).count -= static_cast<int>(order.size());
        return Progress::Continue;
      }
      next.setPending(HexModel::ChooseLoss{owed.side, order, owed.count});
      sink.onEvent(DecisionRequested{"loss"});
      return Progress::Asked;
    }

    void
    expandRetreat(const Ctx& ctx, Position& next, CombatPlan& plan, HexModel::OwedRetreat owed)
    {
      std::vector<HexModel::CombatStep> walks;
      for (UnitId unit : plan.involved) {
        if (owed.side != ctx.roster.unit(unit).side) {
          continue;
        }
        if (const std::optional<HexIndex> hex = hexOf(next, unit)) {
          walks.push_back(HexModel::UnitRetreat{unit, *hex, owed.fewest, owed.most, {}});
        }
      }
      plan.steps.erase(plan.steps.begin());
      plan.steps.insert(plan.steps.begin(), walks.begin(), walks.end());
      return;
    }

    Progress
    settleWalk(const Ctx& ctx, const Policies& policies, Position& next, CombatPlan& plan, EventSink& sink)
    {
      HexModel::UnitRetreat& walk = std::get<HexModel::UnitRetreat>(plan.steps.front());
      const SideId owner = ctx.roster.unit(walk.unit).side;
      const std::optional<HexIndex> here = hexOf(next, walk.unit);
      const Ctx now{ctx.board, ctx.rules, ctx.roster, next};
      const auto finish = [&]() {
        if (!walk.path.empty()) {
          sink.onEvent(Retreated{walk.unit, walk.path});
        }
        plan.steps.erase(plan.steps.begin());
        return Progress::Continue;
      };

      if (!here) {
        plan.steps.erase(plan.steps.begin());
        return Progress::Continue;
      }
      if (walk.path.empty()) {
        switch (policies.retreat->fate(now, walk.unit)) {
          case RetreatFate::Walk:
            break;
          case RetreatFate::Stay:
            sink.onEvent(GameEvent{"holds", ctx.roster.unit(walk.unit).counter.text});
            plan.steps.erase(plan.steps.begin());
            return Progress::Continue;
          case RetreatFate::Surrender:
            removeToBox(next, walk.unit, boxFor(ctx, owner, false), sink);
            plan.steps.erase(plan.steps.begin());
            return Progress::Continue;
        }
      }
      if (static_cast<int>(walk.path.size()) >= walk.most) {
        return finish();
      }

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
          return finish();
        }
        // Unsatisfiable: the rules document's retreat/@unsatisfiable is "eliminate" in every game in view.
        removeToBox(next, walk.unit, boxFor(ctx, owner, true), sink);
        plan.steps.erase(plan.steps.begin());
        return Progress::Continue;
      }
      if (1 == options.size() && !mayStopP) {
        next.place(walk.unit, options.front());
        walk.path.push_back(options.front());
        return Progress::Continue;
      }
      next.setPending(HexModel::ChooseRetreat{plan.router.value_or(owner), walk.unit, options, mayStopP});
      sink.onEvent(DecisionRequested{"retreat"});
      return Progress::Asked;
    }

  }  // namespace

  Position
  settlePlan(const Ctx& ctx, const Policies& policies, EventSink& sink)
  {
    if (nullptr == policies.retreat) {
      throw std::invalid_argument("settlePlan: the policy set has no RetreatPolicy");
    }
    Position next = ctx.position;
    if (!next.combatPlan()) {
      return next;
    }
    CombatPlan plan = *next.combatPlan();
    while (!plan.steps.empty()) {
      Progress progress = Progress::Continue;
      const HexModel::CombatStep head = plan.steps.front();
      if (const HexModel::OwedLoss* owed = std::get_if<HexModel::OwedLoss>(&head)) {
        progress = settleLoss(ctx, next, plan, *owed, sink);
      } else if (const HexModel::OwedRetreat* retreat = std::get_if<HexModel::OwedRetreat>(&head)) {
        expandRetreat(ctx, next, plan, *retreat);
      } else {
        progress = settleWalk(ctx, policies, next, plan, sink);
      }
      if (Progress::Asked == progress) {
        next.setCombatPlan(std::move(plan));
        return next;
      }
    }
    next.setCombatPlan(std::nullopt);
    next.setPending(HexModel::NoDecision{});
    return next;
  }

}  // namespace HexEngine::Adjudicators
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------

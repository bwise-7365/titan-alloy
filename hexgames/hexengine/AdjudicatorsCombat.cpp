// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
// Combat and the decisions it raises. A battle is declared and resolved at once; what the result
// leaves owing -- losses a side chooses, retreats the router walks -- goes on the resolution stack,
// which settleStack (AdjudicatorsStack.cpp) works down one answer at a time.
// ----------------------------------------------
#include "hexengine/AdjudicatorsDetail.h"

#include "hexengine/Defaults.h"

#include <algorithm>
#include <stdexcept>
#include <string>
#include <type_traits>

namespace HexEngine::Adjudicators {

  using Detail::boxFor;
  using Detail::loseStep;
  using Detail::removeToBox;

  namespace {

    std::vector<UnitId>
    defendersAt(const Ctx& ctx, HexIndex target, SideId attacker)
    {
      const SideMask enemies = enemiesOf(ctx.rules, attacker);
      std::vector<UnitId> out;
      for (UnitId unit : ctx.position.unitsAt(target)) {
        if (enemies.test(ctx.roster.unit(unit).side.value)) {
          out.push_back(unit);
        }
      }
      return out;
    }

    std::vector<Direction>
    attackDirections(const Ctx& ctx, HexIndex target, const std::vector<UnitId>& attackers)
    {
      std::vector<Direction> out;
      for (UnitId unit : attackers) {
        const std::optional<HexIndex> from = Detail::hexOf(ctx.position, unit);
        if (!from) {
          continue;
        }
        for (int d = 0; d < HexCoord::kDirections; ++d) {
          const Direction direction = static_cast<Direction>(d);
          const std::optional<HexIndex> neighbour = ctx.board.neighbour(target, direction);
          if (neighbour && *neighbour == *from) {
            out.push_back(direction);
            break;
          }
        }
      }
      return out;
    }

    void
    clearSide(const Ctx& ctx, Position& next, const std::vector<UnitId>& involved, SideId side,
               bool returnsP, EventSink& sink)
    {
      const std::optional<SpaceId> box = boxFor(ctx, side, returnsP);
      for (UnitId unit : involved) {
        if (side == ctx.roster.unit(unit).side && next.unit(unit).where.has_value()) {
          removeToBox(next, unit, box, sink);
        }
      }
      return;
    }

    // The rules document's retreat/@routed-by: "attacker" names the attacking side as router of
    // every retreat; any other reading leaves each unit's owner to route its own.
    std::optional<SideId>
    routerOf(const Ctx& ctx, SideId attacker)
    {
      if (ctx.rules.retreat() && "attacker" == ctx.rules.retreat()->routedBy) {
        return attacker;
      }
      return std::nullopt;
    }

  }  // namespace

  Position
  applyAttack(const Ctx& ctx, const Policies& policies, const DeclareAttack& command, PrngStreams& streams,
               EventSink& sink)
  {
    if (nullptr == policies.combat) {
      throw std::invalid_argument("applyAttack: the policy set has no CombatResolver");
    }
    if (command.attackers.empty()) {
      throw std::invalid_argument("applyAttack: an attack needs at least one attacker");
    }
    const SideId attackerSide = ctx.roster.unit(command.attackers.front()).side;
    const std::vector<UnitId> defenders = defendersAt(ctx, command.target, attackerSide);
    if (defenders.empty()) {
      throw std::invalid_argument("applyAttack: the target hex holds no enemy unit");
    }

    CombatContext combat;
    combat.attackers = command.attackers;
    combat.target = command.target;
    combat.defenders = defenders;
    combat.attackDirections = attackDirections(ctx, command.target, command.attackers);
    combat.declared = command.modifiers;

    sink.onEvent(CombatDeclared{command.attackers, command.target});
    const CombatReport report = policies.combat->report(ctx, combat, streams);
    for (int die : report.dice) {
      sink.onEvent(DieRolled{StreamTag::Combat, die});
    }
    sink.onEvent(CombatResolved{command.target, report.odds, report.outcome});

    HexModel::Battle battle;
    battle.router = routerOf(ctx, attackerSide);
    battle.involved = command.attackers;
    battle.involved.insert(battle.involved.end(), defenders.begin(), defenders.end());

    Position next = ctx.position;
    std::vector<HexModel::Obligation> owed;
    for (UnitId unit : command.attackers) {
      next.state(unit).flags.attackedP = true;
    }
    for (UnitId unit : defenders) {
      next.state(unit).flags.defendedP = true;
    }

    // What leaves play outright happens now; losses and retreats keep the result's own order.
    for (const CombatEffect& effect : report.effects) {
      std::visit(
          [&](auto&& e) {
            using T = std::decay_t<decltype(e)>;
            if constexpr (std::is_same_v<T, Eliminate>) {
              clearSide(ctx, next, battle.involved, e.side, true, sink);
            } else if constexpr (std::is_same_v<T, Surrender>) {
              clearSide(ctx, next, battle.involved, e.side, false, sink);
            } else if constexpr (std::is_same_v<T, StepLoss>) {
              owed.push_back(HexModel::OwedLoss{battle, e.side, e.steps});
            } else if constexpr (std::is_same_v<T, RetreatEffect>) {
              owed.push_back(HexModel::OwedRetreat{battle, e.side, e.fewest, e.most});
            } else if constexpr (std::is_same_v<T, NoEffect>) {
              sink.onEvent(GameEvent{"contact", "no loss or retreat by either side"});
            } else if constexpr (std::is_same_v<T, RevealAndReconsult>) {
              sink.onEvent(GameEvent{"reveal", "the table is consulted again once the defender shows"});
            } else if constexpr (std::is_same_v<T, GameEffect>) {
              sink.onEvent(GameEvent{"result", e.code});
            } else if constexpr (std::is_same_v<T, OweEffect>) {
              owed.push_back(e.owed);
            }
          },
          effect);
    }

    // The result's first obligation is settled first, so it goes on last.
    for (auto entry = owed.rbegin(); owed.rend() != entry; ++entry) {
      next.push(*entry);
    }
    const Ctx planned{ctx.board, ctx.rules, ctx.roster, next};
    return settleStack(planned, policies, streams, sink);
  }

  namespace {

    Position
    answerLoss(const Ctx& ctx, const Policies& policies, const HexModel::ChooseLoss& loss,
                const DecisionAnswer& answer, const GameNames& names, PrngStreams& streams, EventSink& sink)
    {
      if ("loss" != answer.what) {
        throw std::invalid_argument("applyDecision: the position is waiting for a 'loss' answer, not '" +
                                     answer.what + "'");
      }
      const UnitId chosen = names.unitOf(answer.answer);
      if (loss.candidates.end() == std::find(loss.candidates.begin(), loss.candidates.end(), chosen)) {
        throw std::invalid_argument("applyDecision: counter '" + answer.answer +
                                     "' is not among the units that may take the loss");
      }
      Position next = ctx.position;
      next.ask(HexModel::NoDecision{});
      sink.onEvent(DecisionAnswered{answer.what, answer.answer});
      loseStep(ctx, next, chosen, boxFor(ctx, ctx.roster.unit(chosen).side, true), sink);
      HexModel::OwedLoss& owed = std::get<HexModel::OwedLoss>(next.top().owed);
      owed.count -= 1;
      if (0 >= owed.count) {
        next.pop();
      }
      const Ctx answered{ctx.board, ctx.rules, ctx.roster, next};
      return settleStack(answered, policies, streams, sink);
    }

    Position
    answerRetreat(const Ctx& ctx, const Policies& policies, const HexModel::ChooseRetreat& retreat,
                   const DecisionAnswer& answer, const GameNames& names, PrngStreams& streams, EventSink& sink)
    {
      if ("retreat" != answer.what) {
        throw std::invalid_argument("applyDecision: the position is waiting for a 'retreat' answer, not '" +
                                     answer.what + "'");
      }
      Position next = ctx.position;
      next.ask(HexModel::NoDecision{});
      const HexModel::UnitRetreat walk = std::get<HexModel::UnitRetreat>(next.top().owed);
      if ("stop" == answer.answer) {
        if (!retreat.mayStopP) {
          throw std::invalid_argument("applyDecision: counter '" + names.counter(retreat.unit) +
                                       "' has not yet retreated as far as it must");
        }
        sink.onEvent(DecisionAnswered{answer.what, answer.answer});
        sink.onEvent(Retreated{walk.unit, walk.path});
        next.pop();
      } else {
        const HexIndex to = names.hexOf(answer.answer);
        if (retreat.candidates.end() == std::find(retreat.candidates.begin(), retreat.candidates.end(), to)) {
          throw std::invalid_argument("applyDecision: hex '" + answer.answer +
                                       "' is not a hex the unit may retreat into");
        }
        sink.onEvent(DecisionAnswered{answer.what, answer.answer});
        next.place(walk.unit, to);
        std::get<HexModel::UnitRetreat>(next.top().owed).path.push_back(to);
      }
      const Ctx answered{ctx.board, ctx.rules, ctx.roster, next};
      return settleStack(answered, policies, streams, sink);
    }

    // A decision a game obligation asked: the engine checks a GameChoice's verb and options, the
    // game's ObligationPolicy does the rest.
    Position
    answerGame(const Ctx& ctx, const Policies& policies, const DecisionAnswer& answer, PrngStreams& streams,
                EventSink& sink)
    {
      if (const HexModel::GameChoice* choice = std::get_if<HexModel::GameChoice>(&ctx.position.pending())) {
        if (choice->verb != answer.what) {
          throw std::invalid_argument("applyDecision: the position is waiting for a '" + choice->verb +
                                       "' answer, not '" + answer.what + "'");
        }
        if (choice->options.end() == std::find(choice->options.begin(), choice->options.end(), answer.answer)) {
          throw std::invalid_argument("applyDecision: '" + answer.answer + "' is not one of the options");
        }
      }
      if (nullptr == policies.obligations) {
        throw std::invalid_argument("applyDecision: a game obligation asked this decision and the policy set has "
                                    "no ObligationPolicy");
      }
      sink.onEvent(DecisionAnswered{answer.what, answer.answer});
      Position next = policies.obligations->answer(ctx, answer, streams, sink);
      const Ctx answered{ctx.board, ctx.rules, ctx.roster, next};
      return settleStack(answered, policies, streams, sink);
    }

  }  // namespace

  Position
  applyDecision(const Ctx& ctx, const Policies& policies, const DecisionAnswer& answer,
                 const GameNames& names, PrngStreams& streams, EventSink& sink)
  {
    const HexModel::PendingDecision pending = ctx.position.pending();
    if (std::holds_alternative<HexModel::NoDecision>(pending)) {
      throw std::invalid_argument("applyDecision: no decision is pending");
    }
    if (std::holds_alternative<HexModel::Polymorphic<HexModel::GameObligation>>(ctx.position.top().owed)) {
      return answerGame(ctx, policies, answer, streams, sink);
    }
    if (const HexModel::ChooseLoss* loss = std::get_if<HexModel::ChooseLoss>(&pending)) {
      return answerLoss(ctx, policies, *loss, answer, names, streams, sink);
    }
    if (const HexModel::ChooseRetreat* retreat = std::get_if<HexModel::ChooseRetreat>(&pending)) {
      return answerRetreat(ctx, policies, *retreat, answer, names, streams, sink);
    }
    throw std::invalid_argument("applyDecision: an engine obligation asked a decision it cannot answer");
  }

}  // namespace HexEngine::Adjudicators
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------

// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
// Combat and the decisions it raises.
// ----------------------------------------------
#include "hexengine/AdjudicatorsDetail.h"

#include "hexengine/Defaults.h"

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
    const SideId defenderSide = ctx.roster.unit(defenders.front()).side;

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

    std::vector<UnitId> involved = command.attackers;
    involved.insert(involved.end(), defenders.begin(), defenders.end());

    Position next = ctx.position;
    for (UnitId unit : command.attackers) {
      next.state(unit).flags.attackedP = true;
    }

    // The order below is the order the rules read in: what leaves play, then what the defender
    // loses, then what retreats, and only then the choice the attacker has to make. Keeping the
    // attacker's choice last is what lets one decision finish the battle.
    int attackerLosses = 0;
    std::vector<std::pair<SideId, int>> retreats;
    for (const CombatEffect& effect : report.effects) {
      std::visit(
          [&](auto&& e) {
            using T = std::decay_t<decltype(e)>;
            if constexpr (std::is_same_v<T, Eliminate>) {
              clearSide(ctx, next, involved, e.side, true, sink);
            } else if constexpr (std::is_same_v<T, Surrender>) {
              clearSide(ctx, next, involved, e.side, false, sink);
            } else if constexpr (std::is_same_v<T, StepLoss>) {
              if (e.side == attackerSide) {
                attackerLosses += e.steps;
              } else {
                const Ctx here{ctx.board, ctx.rules, ctx.roster, next};
                const std::vector<UnitId> order = lossOrder(here, next, involved, e.side);
                for (int taken = 0; taken < e.steps && taken < static_cast<int>(order.size()); ++taken) {
                  loseStep(here, next, order[static_cast<std::size_t>(taken)],
                            boxFor(ctx, e.side, true), sink);
                }
              }
            } else if constexpr (std::is_same_v<T, RetreatEffect>) {
              retreats.emplace_back(e.side, e.hexes);
            } else if constexpr (std::is_same_v<T, NoEffect>) {
              sink.onEvent(GameEvent{"contact", "no loss or retreat by either side"});
            } else if constexpr (std::is_same_v<T, RevealAndReconsult>) {
              sink.onEvent(GameEvent{"reveal", "the table is consulted again once the defender shows"});
            } else if constexpr (std::is_same_v<T, GameEffect>) {
              sink.onEvent(GameEvent{"result", e.code});
            }
          },
          effect);
    }

    for (const auto& [side, hexes] : retreats) {
      for (UnitId unit : involved) {
        if (side != ctx.roster.unit(unit).side || !next.unit(unit).where.has_value()) {
          continue;
        }
        const Ctx here{ctx.board, ctx.rules, ctx.roster, next};
        next = applyRetreat(here, policies, unit, hexes, sink);
      }
    }

    if (0 < attackerLosses) {
      const Ctx here{ctx.board, ctx.rules, ctx.roster, next};
      const std::vector<UnitId> order = lossOrder(here, next, involved, attackerSide);
      if (order.empty()) {
        // Nothing of the attacker's is left to lose; the result is spent.
      } else if (1 == order.size()) {
        loseStep(here, next, order.front(), boxFor(ctx, attackerSide, true), sink);
      } else {
        next.setPending(HexModel::ChooseLoss{order, attackerLosses});
        sink.onEvent(DecisionRequested{"loss"});
      }
    }
    static_cast<void>(defenderSide);
    return next;
  }

  Position
  applyDecision(const Ctx& ctx, const Policies& policies, const DecisionAnswer& answer,
                 const GameNames& names, EventSink& sink)
  {
    Position next = ctx.position;
    const HexModel::PendingDecision pending = ctx.position.pending();

    if (const HexModel::ChooseLoss* loss = std::get_if<HexModel::ChooseLoss>(&pending)) {
      if ("loss" != answer.what) {
        throw std::invalid_argument("applyDecision: the position is waiting for a 'loss' answer, not '" +
                                     answer.what + "'");
      }
      const UnitId chosen = names.unitOf(answer.answer);
      if (loss->candidates.end() ==
          std::find(loss->candidates.begin(), loss->candidates.end(), chosen)) {
        throw std::invalid_argument("applyDecision: counter '" + answer.answer +
                                     "' is not among the units that may take the loss");
      }
      sink.onEvent(DecisionAnswered{answer.what, answer.answer});
      loseStep(ctx, next, chosen, Detail::boxFor(ctx, ctx.roster.unit(chosen).side, true), sink);
      if (1 < loss->count) {
        std::vector<UnitId> remaining;
        for (UnitId unit : loss->candidates) {
          if (unit != chosen && next.unit(unit).where.has_value()) {
            remaining.push_back(unit);
          }
        }
        next.setPending(HexModel::ChooseLoss{remaining, loss->count - 1});
        sink.onEvent(DecisionRequested{"loss"});
      } else {
        next.setPending(HexModel::NoDecision{});
      }
      return next;
    }

    if (const HexModel::ChooseRetreat* retreat = std::get_if<HexModel::ChooseRetreat>(&pending)) {
      if ("retreat" != answer.what) {
        throw std::invalid_argument("applyDecision: the position is waiting for a 'retreat' answer, not '" +
                                     answer.what + "'");
      }
      const HexIndex to = names.hexOf(answer.answer);
      if (retreat->candidates.end() ==
          std::find(retreat->candidates.begin(), retreat->candidates.end(), to)) {
        throw std::invalid_argument("applyDecision: hex '" + answer.answer +
                                     "' is not a hex the unit may retreat into");
      }
      sink.onEvent(DecisionAnswered{answer.what, answer.answer});
      next.place(retreat->unit, to);
      sink.onEvent(Retreated{retreat->unit, {to}});
      next.setPending(HexModel::NoDecision{});
      return next;
    }

    if (const HexModel::GameChoice* choice = std::get_if<HexModel::GameChoice>(&pending)) {
      if (choice->verb != answer.what) {
        throw std::invalid_argument("applyDecision: the position is waiting for a '" + choice->verb +
                                     "' answer, not '" + answer.what + "'");
      }
      if (choice->options.end() ==
          std::find(choice->options.begin(), choice->options.end(), answer.answer)) {
        throw std::invalid_argument("applyDecision: '" + answer.answer + "' is not one of the options");
      }
      sink.onEvent(DecisionAnswered{answer.what, answer.answer});
      next.setPending(HexModel::NoDecision{});
      return next;
    }

    static_cast<void>(policies);
    throw std::invalid_argument("applyDecision: no decision is pending");
  }

}  // namespace HexEngine::Adjudicators
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------

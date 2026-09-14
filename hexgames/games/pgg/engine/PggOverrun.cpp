// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
#include "PggOverrun.h"

#include "PggState.h"
#include "PggUnits.h"

#include "hexengine/Adjudicators.h"

#include <algorithm>
#include <array>
#include <stdexcept>

namespace Pgg {

  namespace {

    constexpr std::array<std::string_view, 1> kClaims{"overrun-eligibility"};

    [[noreturn]] void
    refuse(const std::string& why)
    {
      throw std::invalid_argument("PggOverrun: " + why);
    }

    std::vector<std::string>
    tokensOf(const std::string& text)
    {
      std::vector<std::string> out;
      std::string current;
      for (char c : text + " ") {
        if (' ' == c) {
          if (!current.empty()) {
            out.push_back(current);
          }
          current.clear();
        } else {
          current += c;
        }
      }
      return out;
    }

  }  // namespace

  PggOverrun::PggOverrun(const PggFacts& facts, const PggMovement& movement, const PggCombat& combat)
    : facts_(facts), movement_(movement), combat_(combat)
  {
  }

  std::span<const std::string_view>
  PggOverrun::claims()
  {
    return kClaims;
  }

  void
  PggOverrun::check(const Ctx& ctx, const std::vector<UnitId>& units, HexIndex target) const
  {
    const std::optional<SideId> acting = ctx.position.clock().actingSide;
    const std::optional<HexIndex> from = Units::hexOf(ctx.position, units.front());
    if (!acting || !from) {
      refuse("the overrunning units must be on the map in their own side's phase");
    }
    const std::optional<Direction> direction = facts_.directionTo(*from, target);
    if (!direction || facts_.lakeHexsideP(*from, *direction) || !Units::enemyAtP(ctx, target, *acting)) {
      refuse("hex '" + ctx.board.id(target).text + "' is not an enemy-occupied hex next to the overrunning units (6.51)");
    }
    if (facts_.soviet() == *acting && 6 >= ctx.position.clock().turn && facts_.westmostColumnsP(target)) {
      refuse("a Soviet unit may not overrun into hex-columns 01 and 02 on turns 1-6 (6.4)");
    }
    const PggSideState& mine = stateOf(ctx.position).side(*acting);
    bool attackerP = false;
    for (UnitId unit : units) {
      const std::string& id = ctx.roster.unit(unit).counter.text;
      if (*acting != ctx.roster.unit(unit).side || Units::hexOf(ctx.position, unit) != from) {
        refuse("counter '" + id + "' is not the acting side's, or not in the overrunning hex (6.54)");
      }
      if (!movement_.movementPhaseP(ctx, unit) || mine.disrupted.contains(unit) || mine.halted.contains(unit)) {
        refuse("counter '" + id + "' may not move in this phase, is Disrupted, or was halted by an overrun (6.52, 6.62)");
      }
      if (kCostHalves > movement_.remainingHalves(ctx, unit)) {
        refuse("counter '" + id + "' has fewer than the three movement points an overrun costs (6.51)");
      }
      if (mine.beyondRadius.contains(unit)) {
        refuse("counter '" + id + "' began the phase beyond every Leader's radius (6.55)");
      }
      attackerP = attackerP || !facts_.leaderP(unit);
    }
    if (!attackerP) {
      refuse("Leaders may not overrun by themselves (6.53)");
    }
    return;
  }

  Position
  PggOverrun::overrun(const HexEngine::CommandCall& call) const
  {
    const HexEngine::GameCommand* command = std::get_if<HexEngine::GameCommand>(&call.command);
    if (nullptr == command || 2 != command->args.size()) {
      refuse("'overrun' needs units and a target");
    }
    const Ctx& ctx = call.ctx;
    std::vector<UnitId> units;
    for (const std::string& token : tokensOf(command->args[0])) {
      units.push_back(facts_.counter(token));
    }
    if (units.empty()) {
      refuse("'overrun' names no unit");
    }
    const HexIndex target = facts_.hex(command->args[1]);
    check(ctx, units, target);

    Position next = ctx.position;
    PggSideState& mine = stateOf(next).side(ctx.roster.unit(units.front()).side);
    for (UnitId unit : units) {
      mine.spent[unit] += kCostHalves;
      mine.continuing.erase(unit);
      next.state(unit).flags.movedP = true;
    }
    const Ctx spent{ctx.board, ctx.rules, ctx.roster, next};
    call.sink.onEvent(HexEngine::CombatDeclared{units, target});
    const HexEngine::CombatReport report =
        combat_.resolveBattle(spent, combat_.assess(spent, units, target, true), target, true, call.streams);
    for (int die : report.dice) {
      call.sink.onEvent(HexEngine::DieRolled{HexEngine::StreamTag::Combat, die});
    }
    call.sink.onEvent(HexEngine::CombatResolved{target, report.odds, report.outcome});
    for (const HexEngine::CombatEffect& effect : report.effects) {
      next.push(std::get<HexEngine::OweEffect>(effect).owed);
    }
    const Ctx owed{ctx.board, ctx.rules, ctx.roster, next};
    return HexEngine::Adjudicators::settleStack(owed, call.policies, call.streams, call.sink);
  }

}  // namespace Pgg
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------

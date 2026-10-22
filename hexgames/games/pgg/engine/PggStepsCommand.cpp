// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
// Before-command checks (a move's whole path, an attack's legality, the first-turn rules) and
// after-command effects (movement points spent, rail hexes passed and cuts repaired, the hex a unit
// entered an enemy zone from, control of the Victory Point hexes).
// ----------------------------------------------
#include "PggState.h"
#include "PggSteps.h"
#include "PggUnits.h"

#include <set>

namespace Pgg {

  namespace {

    using HexEngine::CheckCall;
    using HexEngine::StepCall;

    [[noreturn]] void
    refuse(const std::string& why)
    {
      throw std::invalid_argument("Pgg: " + why);
    }

    // 6.1, 6.2, 6.3: one stack, one connected path it may afford, through no enemy unit, stopping at the
    // first enemy zone of control (8.13), and within the rail capacity (6.31).
    void
    checkMove(const PggParts& parts, const Ctx& ctx, const HexEngine::MoveUnit& move)
    {
      if (2 > move.path.size() || move.units.empty()) {
        refuse("a move needs units and a path of two hexes or more");
      }
      const SideId side = ctx.roster.unit(move.units.front()).side;
      const std::set<HexIndex> distinct(move.path.begin(), move.path.end());
      if (distinct.size() != move.path.size()) {
        refuse("a move's path may not enter a hex twice");
      }
      for (std::size_t i = 1; i < move.path.size(); ++i) {
        if (Units::enemyAtP(ctx, move.path[i], side)) {
          refuse("hex '" + ctx.board.id(move.path[i]).text + "' holds an enemy unit (6.21)");
        }
        const bool lastP = move.path.size() - 1 == i;
        if (!lastP && parts.zoc.blockedForP(ctx, move.path[i], side, HexRules::Purpose::Movement)) {
          refuse("the path does not stop in the enemy zone of control at '" + ctx.board.id(move.path[i]).text + "' (6.22)");
        }
      }
      int railPoints = 0;
      for (UnitId unit : move.units) {
        const std::string& id = ctx.roster.unit(unit).counter.text;
        if (Units::hexOf(ctx.position, unit) != move.path.front()) {
          refuse("counter '" + id + "' does not stand on the path's first hex");
        }
        const std::optional<int> cost = parts.movement.pathHalves(ctx, unit, move.path, move.mode);
        const int allowance = std::get<MovementPoints>(parts.movement.allowance(ctx, unit, move.mode)).halves;
        if (!cost || allowance < *cost) {
          refuse("counter '" + id + "' may not take this path in this phase, or cannot afford it (6.12, 6.23)");
        }
        railPoints += parts.facts.railMode() == move.mode ? parts.facts.railPoints(unit) : 0;
      }
      if (PggMovement::kRailUnits < stateOf(ctx.position).railUnits + railPoints) {
        refuse("rail movement carries eight combat units a turn, armour counting three (6.31)");
      }
      return;
    }

    // 9.14, 9.23, 6.62, 10.22, 10.35, 12.3, 8.17.
    void
    checkAttack(const PggParts& parts, const Ctx& ctx, const HexEngine::DeclareAttack& attack)
    {
      const PggFacts& facts = parts.facts;
      HexSearch::SearchScratch scratch;
      for (UnitId unit : attack.attackers) {
        const SideId side = ctx.roster.unit(unit).side;
        const std::string& id = ctx.roster.unit(unit).counter.text;
        const HexModel::UnitState& state = ctx.position.unit(unit);
        if (facts.leaderP(unit)) {
          refuse("counter '" + id + "' is a Leader, with no Attack Strength of its own (10.22)");
        }
        if (stateOf(ctx.position).side(side).disrupted.contains(unit)) {
          refuse("counter '" + id + "' is Disrupted and may not attack (6.62)");
        }
        if (state.flags.attackedP) {
          refuse("counter '" + id + "' has already attacked this phase (9.14)");
        }
        if (state.flags.revealedP && 0 == Units::attackOf(ctx, unit)) {
          refuse("counter '" + id + "' has no Attack Strength and may only defend (12.3)");
        }
        const std::optional<HexIndex> from = Units::hexOf(ctx.position, unit);
        const std::optional<Direction> direction = from ? facts.directionTo(*from, attack.target) : std::nullopt;
        if (direction && facts.lakeHexsideP(*from, *direction)) {
          refuse("no combat is allowed across a Lake hexside (6.7)");
        }
        if (facts.soviet() == side && !parts.supply.withinRadiusP(ctx, scratch, unit)) {
          refuse("counter '" + id + "' is beyond every working Leader's radius and may not attack (10.35)");
        }
      }
      for (UnitId unit : ctx.position.unitsAt(attack.target)) {
        if (ctx.position.unit(unit).flags.defendedP) {
          refuse("counter '" + ctx.roster.unit(unit).counter.text + "' has already been attacked this phase (9.14)");
        }
      }
      return;
    }

    Position
    spendMovement(const PggParts& parts, const StepCall& call)
    {
      const HexEngine::MoveUnit& move = commandOf<HexEngine::MoveUnit>(call);
      Position next = call.ctx.position;
      PggState& state = stateOf(next);
      PggSideState& mine = state.side(call.ctx.roster.unit(move.units.front()).side);
      for (UnitId unit : move.units) {
        const std::optional<int> cost = parts.movement.pathHalves(call.ctx, unit, move.path, move.mode);
        if (!cost) {
          refuse("step '" + call.step.id + "': the move's cost cannot be recomputed");
        }
        mine.spent[unit] += *cost;
        mine.continuing.erase(unit);
        if (parts.facts.railMode() == move.mode) {
          state.railUnits += parts.facts.railPoints(unit);
        }
      }
      return next;
    }

    Position
    noteZocEntry(const PggParts& parts, const StepCall& call)
    {
      const HexEngine::MoveUnit& move = commandOf<HexEngine::MoveUnit>(call);
      Position next = call.ctx.position;
      const SideId side = call.ctx.roster.unit(move.units.front()).side;
      PggSideState& mine = stateOf(next).side(side);
      const bool enteredP = parts.zoc.enemyZocP(call.ctx, move.path.back(), side);
      for (UnitId unit : move.units) {
        if (enteredP) {
          mine.zocEntry[unit] = move.path[move.path.size() - 2];  // 10.23
        } else {
          mine.zocEntry.erase(unit);
        }
      }
      return next;
    }

  }  // namespace

  void
  registerPggCommandSteps(HexEngine::StepRegistry& registry, const PggParts& parts)
  {
    registry.addCheck("check-move", [parts](const CheckCall& call) {
      checkMove(parts, call.ctx, commandOf<HexEngine::MoveUnit>(call));
      return;
    });
    registry.addCheck("check-attack", [parts](const CheckCall& call) {
      checkAttack(parts, call.ctx, commandOf<HexEngine::DeclareAttack>(call));
      return;
    });
    registry.addCheck("check-first-turn", [parts](const CheckCall& call) {
      parts.firstTurn.check(call.ctx, call.command);
      return;
    });
    registry.addEffect("spend-movement", [parts](const StepCall& call) { return spendMovement(parts, call); });
    registry.addEffect("note-passage", [parts](const StepCall& call) {
      const Position passed = parts.interdiction.notePassage(call.ctx, commandOf<HexEngine::MoveUnit>(call), call.sink);
      const Ctx after{call.ctx.board, call.ctx.rules, call.ctx.roster, passed};
      return noteZocEntry(parts, StepCall{after, call.policies, call.streams, call.scratch, call.sink, call.command, call.step});
    });
    registry.addEffect("note-control", [parts](const StepCall& call) {
      return parts.victory.noteControl(call.ctx, call.command, call.sink);
    });
    return;
  }

}  // namespace Pgg
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------

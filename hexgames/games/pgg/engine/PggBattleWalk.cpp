// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
// Retreats (9.7): each unit of the side walks the result's number of hexes in turn, routed by the
// opposing player (9.72), never back onto a hex it has left; with one way to go the step is taken
// without asking, with none the unit is eliminated, and a unit that ends on an overstacked hex is
// eliminated (9.73). A unit that ends on a friendly stack is marked for 9.75.
// ----------------------------------------------
#include "PggBattleFlow.h"

#include <algorithm>

namespace Pgg::BattleFlow {

  namespace {

    void
    finishWalk(const Env& env, Position& next, PggBattle& battle)
    {
      const RetreatWalk walk = *battle.walk;
      battle.walk.reset();
      battle.retreated.push_back(walk.unit);
      const std::optional<HexIndex> here = Units::hexOf(next, walk.unit);
      if (!here) {
        return;
      }
      if (!walk.path.empty()) {
        env.sink.onEvent(HexEngine::Retreated{walk.unit, walk.path});
      }
      const SideId side = env.base.roster.unit(walk.unit).side;
      if (battle.defenderSide == side && battle.pathOfRetreat.empty()) {
        battle.pathOfRetreat = walk.path;
      }
      std::vector<UnitId> stack;
      for (UnitId unit : next.unitsAt(*here)) {
        if (!env.facts.markerP(unit)) {
          stack.push_back(unit);
        }
      }
      if (1 < stack.size()) {
        if (!env.stacking.legalP(stack)) {
          Units::eliminate(env.facts, next, walk.unit, env.sink);  // 9.73
          return;
        }
        stateOf(next).side(side).retreatedOnto.insert(walk.unit);  // 9.75
      }
      return;
    }

  }  // namespace

  void
  startWalk(const Env&, Position& next, PggBattle& battle)
  {
    const UnitId unit = battle.retreatQueue.front();
    battle.retreatQueue.erase(battle.retreatQueue.begin());
    const std::optional<HexIndex> here = Units::hexOf(next, unit);
    if (!here) {
      return;
    }
    const CrtResult result = crtResultOf(battle.code);
    battle.walk = RetreatWalk{unit, *here, resultOfStage(battle, result).steps, {}};
    return;
  }

  void
  walkTo(const Env&, Position& next, PggBattle& battle, HexIndex to)
  {
    next.place(battle.walk->unit, to);
    battle.walk->path.push_back(to);
    battle.walk->hexesLeft -= 1;
    return;
  }

  void
  walkStep(const Env& env, Position& next, PggBattle& battle)
  {
    const RetreatWalk& walk = *battle.walk;
    const std::optional<HexIndex> here = Units::hexOf(next, walk.unit);
    if (!here || 0 >= walk.hexesLeft) {
      finishWalk(env, next, battle);
      return;
    }
    std::vector<HexIndex> options;
    for (HexIndex hex : env.retreat.candidates(now(env, next), walk.unit, *here)) {
      if (hex != walk.from && walk.path.end() == std::find(walk.path.begin(), walk.path.end(), hex)) {
        options.push_back(hex);
      }
    }
    if (options.empty()) {
      const UnitId unit = walk.unit;
      battle.walk.reset();
      battle.retreated.push_back(unit);
      Units::eliminate(env.facts, next, unit, env.sink);  // retreat/@unsatisfiable, 6.4
      return;
    }
    if (1 == options.size()) {
      walkTo(env, next, battle, options.front());
      return;
    }
    const SideId router = env.facts.enemy(env.base.roster.unit(walk.unit).side);
    env.sink.onEvent(HexEngine::DecisionRequested{"retreat"});
    next.ask(HexModel::ChooseRetreat{router, walk.unit, options, false});
    return;
  }

}  // namespace Pgg::BattleFlow
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------

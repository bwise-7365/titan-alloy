// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
// After the result. An overrun that clears the hex without a split, Engaged or adverse result moves
// every overrunning unit into it for free, and they may move on (6.51); one that does not clear it
// halts them (6.52), and there is never an advance after combat (6.57, 9.88). After ordinary combat
// that vacates the hex, and not on Engaged, the attacker's units that did not retreat may advance
// one at a time into the vacated hex (9.81-9.85), and one more hex along the path of retreat or, when
// the whole stack was eliminated, into any empty hex (9.86); a Soviet unit never into the westernmost
// columns on turns 1-6 (6.4). An advanced unit may not be attacked again this phase (9.85).
// ----------------------------------------------
#include "PggBattleFlow.h"

#include <algorithm>

namespace Pgg::BattleFlow {

  namespace {

    bool
    vacatedP(const Env& env, const Position& next, const PggBattle& battle)
    {
      for (UnitId unit : next.unitsAt(battle.target)) {
        if (battle.defenderSide == env.base.roster.unit(unit).side) {
          return false;
        }
      }
      return true;
    }

    bool
    barredP(const Env& env, const Position& next, const PggBattle& battle, HexIndex hex)
    {
      return env.facts.soviet() == battle.attackerSide && 6 >= next.clock().turn && env.facts.westmostColumnsP(hex);
    }

    std::vector<HexIndex>
    secondHexes(const Env& env, const Position& next, const PggBattle& battle)
    {
      std::vector<HexIndex> out;
      if (2 <= battle.pathOfRetreat.size()) {
        const HexIndex along = battle.pathOfRetreat.front();
        if (next.unitsAt(along).empty() && !barredP(env, next, battle, along)) {
          out.push_back(along);
        }
        return out;
      }
      if (!battle.pathOfRetreat.empty()) {
        return out;  // a one-hex retreat leaves its survivor on the only other hex of the path
      }
      for (int d = 0; d < HexCoord::kDirections; ++d) {
        const Direction direction = static_cast<Direction>(d);
        const std::optional<HexIndex> to = env.base.board.neighbour(battle.target, direction);
        if (to && !env.facts.lakeHexsideP(battle.target, direction) && next.unitsAt(*to).empty() &&
            !barredP(env, next, battle, *to)) {
          out.push_back(*to);
        }
      }
      return out;
    }

    std::vector<UnitId>
    eligible(const Env& env, const Position& next, const PggBattle& battle)
    {
      std::vector<UnitId> out;
      for (UnitId unit : standing(env, next, battle, battle.attackerSide)) {
        const bool retreatedP = battle.retreated.end() != std::find(battle.retreated.begin(), battle.retreated.end(), unit);
        const bool advancedP = battle.advanced.end() != std::find(battle.advanced.begin(), battle.advanced.end(), unit);
        if (!retreatedP && !advancedP && Units::hexOf(next, unit) != battle.target) {
          out.push_back(unit);
        }
      }
      return out;
    }

    void
    overrunEntry(const Env& env, Position& next, PggBattle& battle, const CrtResult& result)
    {
      const std::vector<UnitId> units = eligible(env, next, battle);
      PggSideState& mine = stateOf(next).side(battle.attackerSide);
      if (vacatedP(env, next, battle) && !result.engagedP && !result.splitP && !battle.attackerHitP && !units.empty()) {
        for (UnitId unit : units) {
          const HexIndex from = *Units::hexOf(next, unit);
          next.place(unit, battle.target);
          mine.continuing.insert(unit);
          env.sink.onEvent(HexEngine::UnitMoved{unit, {from, battle.target}});
        }
      } else {
        for (UnitId unit : units) {
          mine.halted.insert(unit);
        }
      }
      battle.stage = BattleStage::Done;
      return;
    }

  }  // namespace

  void
  advanceStep(const Env& env, Position& next, PggBattle& battle)
  {
    const CrtResult result = crtResultOf(battle.code);
    if (battle.overrunP) {
      overrunEntry(env, next, battle, result);
      return;
    }
    if (!vacatedP(env, next, battle) || result.engagedP || barredP(env, next, battle, battle.target)) {
      battle.stage = BattleStage::Done;
      return;
    }
    if (battle.advancing) {
      const std::vector<HexIndex> hexes = secondHexes(env, next, battle);
      if (hexes.empty()) {
        battle.advancing.reset();
        return;
      }
      HexModel::GameChoice choice{"advance-on", {"stop"}, battle.attackerSide};
      for (HexIndex hex : hexes) {
        choice.options.push_back(env.base.board.id(hex).text);
      }
      env.sink.onEvent(HexEngine::DecisionRequested{choice.verb});
      next.ask(std::move(choice));
      return;
    }
    const std::vector<UnitId> units = eligible(env, next, battle);
    if (units.empty()) {
      battle.stage = BattleStage::Done;
      return;
    }
    HexModel::GameChoice choice{"advance", {"done"}, battle.attackerSide};
    for (UnitId unit : units) {
      choice.options.push_back(env.base.roster.unit(unit).counter.text);
    }
    env.sink.onEvent(HexEngine::DecisionRequested{choice.verb});
    next.ask(std::move(choice));
    return;
  }

}  // namespace Pgg::BattleFlow
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------

// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
// Reveal (12.2, 12.3, 10.23) and each side's half of the result (9.64-9.67): the defender first, then
// the attacker; an overrun defender that loses or retreats on a "*" result is Disrupted (6.61), and
// overrunning units halted by any adverse, split or Engaged result (6.52).
// ----------------------------------------------
#include "PggBattleFlow.h"

#include <algorithm>
#include <set>
#include <stdexcept>

namespace Pgg::BattleFlow {

  namespace {

    bool
    revealedByP(const Env& env, UnitId unit, bool attackingP)
    {
      const std::optional<HexRules::Concealment>& hidden =
          env.base.rules.unitTypes()[env.base.roster.unit(unit).type.value].concealment;
      if (!hidden) {
        return false;
      }
      for (HexRules::RevealTrigger trigger : hidden->reveal) {
        switch (trigger) {
          case HexRules::RevealTrigger::Combat:
            return true;
          case HexRules::RevealTrigger::Attacking:
            if (attackingP) {
              return true;
            }
            break;
          case HexRules::RevealTrigger::Attacked:
            if (!attackingP) {
              return true;
            }
            break;
          case HexRules::RevealTrigger::Adjacent:
          case HexRules::RevealTrigger::Rule:
          case HexRules::RevealTrigger::Owner:
            break;
        }
      }
      return false;
    }

    // 10.23, 12.3: a committed 0-1-6 goes back to the hex it entered the enemy zone from, or is lost.
    void
    withdraw(const Env& env, Position& next, UnitId unit)
    {
      const SideId side = env.base.roster.unit(unit).side;
      PggSideState& mine = stateOf(next).side(side);
      const auto entry = mine.zocEntry.find(unit);
      const Ctx here = now(env, next);
      if (mine.zocEntry.end() != entry && !Units::enemyAtP(here, entry->second, side)) {
        const HexIndex from = *Units::hexOf(next, unit);
        const HexIndex to = entry->second;
        next.place(unit, to);
        mine.zocEntry.erase(unit);
        env.sink.onEvent(HexEngine::Retreated{unit, {from, to}});
        return;
      }
      Units::eliminate(env.facts, next, unit, env.sink);
      return;
    }

    void
    finishSide(const Env& env, Position& next, PggBattle& battle)
    {
      const CrtResult result = crtResultOf(battle.code);
      if (BattleStage::Defender == battle.stage) {
        if (battle.overrunP && result.disruptsP && battle.defenderHitP) {
          for (UnitId unit : standing(env, next, battle, battle.defenderSide)) {
            stateOf(next).side(battle.defenderSide).disrupted.insert(unit);
            env.sink.onEvent(HexEngine::GameEvent{"disrupted", env.base.roster.unit(unit).counter.text});
          }
        }
        battle.stage = BattleStage::Attacker;
      } else {
        if (battle.overrunP && (battle.attackerHitP || result.engagedP || result.splitP)) {
          for (UnitId unit : standing(env, next, battle, battle.attackerSide)) {
            stateOf(next).side(battle.attackerSide).halted.insert(unit);
          }
        }
        battle.stage = BattleStage::Advance;
      }
      battle.startedP = false;
      battle.choseP = false;
      battle.stepsLeft = 0;
      return;
    }

    void
    startSide(const Env& env, Position& next, PggBattle& battle)
    {
      const CrtResult result = crtResultOf(battle.code);
      const SideResult& half = resultOfStage(battle, result);
      const SideId side = sideOfStage(battle);
      battle.startedP = true;
      battle.stepsLeft = half.steps;
      if (BattleStage::Defender == battle.stage && Loss::None != half.loss) {
        std::set<UnitId>& onto = stateOf(next).side(side).retreatedOnto;
        for (UnitId unit : standing(env, next, battle, side)) {
          if (onto.contains(unit) && Units::hexOf(next, unit) == battle.target) {
            Units::eliminate(env.facts, next, unit, env.sink);  // 9.75
          }
        }
      }
      if (Loss::All == half.loss) {
        for (UnitId unit : standing(env, next, battle, side)) {
          Units::eliminate(env.facts, next, unit, env.sink);
        }
        (BattleStage::Defender == battle.stage ? battle.defenderHitP : battle.attackerHitP) = true;
      }
      return;
    }

  }  // namespace

  Ctx
  now(const Env& env, const Position& position)
  {
    return Ctx{env.base.board, env.base.rules, env.base.roster, position};
  }

  SideId
  sideOfStage(const PggBattle& battle)
  {
    return BattleStage::Defender == battle.stage ? battle.defenderSide : battle.attackerSide;
  }

  const SideResult&
  resultOfStage(const PggBattle& battle, const CrtResult& result)
  {
    return BattleStage::Defender == battle.stage ? result.defender : result.attacker;
  }

  std::vector<UnitId>
  standing(const Env&, const Position& position, const PggBattle& battle, SideId side)
  {
    const std::vector<UnitId>& units = battle.defenderSide == side ? battle.defenders : battle.attackers;
    std::vector<UnitId> out;
    for (UnitId unit : units) {
      if (Units::hexOf(position, unit)) {
        out.push_back(unit);
      }
    }
    return out;
  }

  void
  loseStep(const Env& env, Position& next, PggBattle& battle, UnitId unit)
  {
    Units::loseStep(env.facts, next, unit, env.sink);
    const std::optional<UnitId> second = env.facts.successor(unit);
    if (second && !Units::hexOf(next, unit) && Units::hexOf(next, *second)) {
      for (std::vector<UnitId>* list : {&battle.attackers, &battle.defenders}) {
        std::replace(list->begin(), list->end(), unit, *second);
      }
    }
    return;
  }

  void
  reveal(const Env& env, Position& next, PggBattle& battle)
  {
    for (bool attackingP : {true, false}) {
      for (UnitId unit : attackingP ? battle.attackers : battle.defenders) {
        if (!Units::hexOf(next, unit) || next.unit(unit).flags.revealedP || !revealedByP(env, unit, attackingP)) {
          continue;
        }
        Units::reveal(next, unit, env.sink);
        const Ctx here = now(env, next);
        if (0 == Units::attackOf(here, unit) && 0 == Units::defenceOf(here, unit)) {
          env.sink.onEvent(HexEngine::GameEvent{"no-strength", env.base.roster.unit(unit).counter.text});
          Units::eliminate(env.facts, next, unit, env.sink);  // 12.3
        } else if (attackingP && 0 == Units::attackOf(here, unit)) {
          withdraw(env, next, unit);  // 12.3: a 0-1-6 may only defend
        }
      }
    }
    battle.stage = "void" == battle.code ? BattleStage::Done : BattleStage::Defender;
    return;
  }

  void
  sideStep(const Env& env, Position& next, PggBattle& battle)
  {
    if (battle.walk) {
      walkStep(env, next, battle);
      return;
    }
    if (!battle.retreatQueue.empty()) {
      startWalk(env, next, battle);
      return;
    }
    if (!battle.startedP) {
      startSide(env, next, battle);
      return;
    }
    const CrtResult result = crtResultOf(battle.code);
    const SideId side = sideOfStage(battle);
    const std::vector<UnitId> units = standing(env, next, battle, side);
    if (0 >= battle.stepsLeft || units.empty()) {
      finishSide(env, next, battle);
      return;
    }
    const bool mayRetreatP = !battle.choseP && !result.engagedP;
    HexModel::GameChoice choice{mayRetreatP ? "result" : "loss", {}, side};
    if (mayRetreatP) {
      choice.options.push_back("retreat");
    }
    for (UnitId unit : units) {
      choice.options.push_back(env.base.roster.unit(unit).counter.text);
    }
    env.sink.onEvent(HexEngine::DecisionRequested{choice.verb});
    next.ask(std::move(choice));
    return;
  }

  void
  step(const Env& env, Position& next, PggBattle& battle)
  {
    switch (battle.stage) {
      case BattleStage::Reveal:
        reveal(env, next, battle);
        return;
      case BattleStage::Defender:
      case BattleStage::Attacker:
        sideStep(env, next, battle);
        return;
      case BattleStage::Advance:
        advanceStep(env, next, battle);
        return;
      case BattleStage::Done:
        return;
    }
    throw std::invalid_argument("Pgg::BattleFlow: battle stage outside the five");
  }

}  // namespace Pgg::BattleFlow
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------

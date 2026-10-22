// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
// The answers a battle asks: "result" (retreat, or the unit that loses a step), "loss" (the unit
// that loses a further step), "retreat" (the router's hex), "advance" (a unit, or done) and
// "advance-on" (one hex further, or stop). The engine has checked a GameChoice's verb and options;
// the retreat hex is checked here.
// ----------------------------------------------
#include "PggBattleFlow.h"

#include <algorithm>
#include <stdexcept>

namespace Pgg::BattleFlow {

  namespace {

    UnitId
    counterNamed(const Env& env, const std::string& id)
    {
      const std::optional<UnitId> unit = env.base.roster.find(CounterId{id});
      if (!unit) {
        throw std::invalid_argument("Pgg::BattleFlow: no counter '" + id + "'");
      }
      return *unit;
    }

    void
    hit(PggBattle& battle)
    {
      (BattleStage::Defender == battle.stage ? battle.defenderHitP : battle.attackerHitP) = true;
      return;
    }

    void
    answerChoice(const Env& env, Position& next, PggBattle& battle, const HexModel::GameChoice& choice,
                 const HexEngine::DecisionAnswer& given)
    {
      if ("result" == choice.verb && "retreat" == given.answer) {
        battle.retreatQueue = standing(env, next, battle, sideOfStage(battle));
        battle.stepsLeft = 0;
        battle.choseP = true;
        hit(battle);
        return;
      }
      if ("result" == choice.verb || "loss" == choice.verb) {
        loseStep(env, next, battle, counterNamed(env, given.answer));
        battle.stepsLeft -= 1;
        battle.choseP = true;
        hit(battle);
        return;
      }
      if ("advance" == choice.verb && "done" == given.answer) {
        battle.stage = BattleStage::Done;
        return;
      }
      if ("advance" == choice.verb) {
        const UnitId unit = counterNamed(env, given.answer);
        const HexIndex from = *Units::hexOf(next, unit);
        next.place(unit, battle.target);
        next.state(unit).flags.defendedP = true;  // 9.85
        battle.advanced.push_back(unit);
        battle.advancing = unit;
        env.sink.onEvent(HexEngine::UnitMoved{unit, {from, battle.target}});
        return;
      }
      if ("advance-on" == choice.verb) {
        const UnitId unit = *battle.advancing;
        battle.advancing.reset();
        if ("stop" != given.answer) {
          const HexIndex to = env.base.board.indexOf(HexCoord::HexId{given.answer});
          next.place(unit, to);
          env.sink.onEvent(HexEngine::UnitMoved{unit, {battle.target, to}});
        }
        return;
      }
      throw std::invalid_argument("Pgg::BattleFlow: a battle asked no '" + choice.verb + "' choice");
    }

  }  // namespace

  void
  answer(const Env& env, Position& next, PggBattle& battle, const HexModel::PendingDecision& asked,
         const HexEngine::DecisionAnswer& given)
  {
    if (const HexModel::GameChoice* choice = std::get_if<HexModel::GameChoice>(&asked)) {
      answerChoice(env, next, battle, *choice, given);
      return;
    }
    if (const HexModel::ChooseRetreat* retreat = std::get_if<HexModel::ChooseRetreat>(&asked)) {
      if ("retreat" != given.what || !battle.walk) {
        throw std::invalid_argument("Pgg::BattleFlow: the battle is waiting for a 'retreat' answer, not '" +
                                    given.what + "'");
      }
      const std::optional<HexIndex> to = env.base.board.find(HexCoord::HexId{given.answer});
      if (!to || retreat->candidates.end() == std::find(retreat->candidates.begin(), retreat->candidates.end(), *to)) {
        throw std::invalid_argument("Pgg::BattleFlow: hex '" + given.answer + "' is not a hex the unit may retreat into");
      }
      walkTo(env, next, battle, *to);
      return;
    }
    throw std::invalid_argument("Pgg::BattleFlow: the battle asked a decision it cannot answer");
  }

}  // namespace Pgg::BattleFlow
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------

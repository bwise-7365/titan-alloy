// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
// How a PggBattle is worked, one transition per call so that the engine's settleStack loop always
// progresses: PggBattleFlow.cpp (reveal and each side's result), PggBattleWalk.cpp (retreats),
// PggBattleAdvance.cpp (advance after combat and the overrun's entry), PggBattleAnswer.cpp (answers).
// ----------------------------------------------
#pragma once
#include "PggBattle.h"
#include "PggCrt.h"
#include "PggRetreat.h"
#include "PggStacking.h"
#include "PggUnits.h"

#include "hexengine/Command.h"

namespace Pgg::BattleFlow {

  struct Env {
    const PggFacts& facts;
    const PggRetreat& retreat;
    const PggStacking& stacking;
    const Ctx& base;  // board, rules and roster; its position is the one the call began with
    HexEngine::EventSink& sink;
  };

  Ctx now(const Env&, const Position&);

  // One transition of the battle; the caller pops it once its stage is Done.
  void step(const Env&, Position&, PggBattle&);
  // Applies the answer to the decision the battle asked (already cleared from the position).
  void answer(const Env&, Position&, PggBattle&, const HexModel::PendingDecision& asked, const HexEngine::DecisionAnswer&);

  // ---- shared by the parts ----
  void reveal(const Env&, Position&, PggBattle&);
  void sideStep(const Env&, Position&, PggBattle&);
  void startWalk(const Env&, Position&, PggBattle&);
  void walkStep(const Env&, Position&, PggBattle&);
  void walkTo(const Env&, Position&, PggBattle&, HexIndex);
  void advanceStep(const Env&, Position&, PggBattle&);
  // The side's battle units still on the map, in battle order.
  std::vector<UnitId> standing(const Env&, const Position&, const PggBattle&, SideId);
  // A step from one battle unit; a German infantry division's second counter takes its place in the lists.
  void loseStep(const Env&, Position&, PggBattle&, UnitId);
  SideId sideOfStage(const PggBattle&);
  const SideResult& resultOfStage(const PggBattle&, const CrtResult&);

}  // namespace Pgg::BattleFlow
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------

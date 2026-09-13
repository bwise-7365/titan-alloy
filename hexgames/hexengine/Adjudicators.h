// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
// The adjudicators: pure functions from a position and a command to the next position, writing
// every step through an EventSink. None of them knows a Session exists, none holds state, and none
// blocks on a choice -- a choice the rules require becomes Position::pending() and is answered by
// the next command.
// ----------------------------------------------
#pragma once
#include "hexengine/Command.h"
#include "hexengine/Event.h"
#include "hexengine/GameNames.h"
#include "hexengine/PhaseCursor.h"
#include "hexengine/Policies.h"
#include "hexengine/PrngStreams.h"
#include "hexsearch/Search.h"

#include <vector>

namespace HexEngine::Adjudicators {

  // Moves every unit of the command along the path, marks them moved, and takes control of the
  // featured hexes the path ends on (the last toucher owns a city).
  Position applyMove(const Ctx&, const Policies&, const MoveUnit&, EventSink&);

  // Declares the battle, resolves it through the CombatResolver on the Combat stream, and turns the
  // effects into eliminations, step losses and retreats. The defender's own choices are settled by
  // a fixed rule (fewest steps, then lowest counter id); a loss the attacker owes with more than one
  // candidate becomes a ChooseLoss the attacker answers next.
  Position applyAttack(const Ctx&, const Policies&, const DeclareAttack&, PrngStreams&, EventSink&);

  // Answers the position's pending decision. Throws unless the answer matches it.
  Position applyDecision(const Ctx&, const Policies&, const DecisionAnswer&, const GameNames&, EventSink&);

  // Walks one unit back the given number of hexes through the RetreatPolicy's candidates, taking
  // the first candidate at each step; an unsatisfiable retreat eliminates the unit.
  Position applyRetreat(const Ctx&, const Policies&, UnitId, int hexes, EventSink&);

  // Removes what the StackingPolicy calls excess from every occupied hex.
  Position applyStackingRepair(const Ctx&, const Policies&, EventSink&);

  // Marks every unit of the side supplied or isolated. It never removes a unit, whatever the trace
  // spec's @fatal says: what an out-of-supply unit suffers is a game's own rule.
  Position applySupplyCheck(const Ctx&, const Policies&, HexSearch::SearchScratch&, SideId, EventSink&);

  // Moves the clock to the next stop of the phase tree.
  Position advancePhase(const Ctx&, const Policies&, const PhaseCursor&, EventSink&);

}  // namespace HexEngine::Adjudicators
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------

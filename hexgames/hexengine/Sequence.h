// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
// Running the rules document's phase steps (M6b). Which steps run is read off the phase tree:
//   before-command and after-command: the steps of the phase a command is issued in and of every
//     phase containing it, outermost phase first, each phase's in document order, filtered by
//     @commands (the grammar verb) and @turns;
//   at a phase boundary: the end steps of every phase the clock leaves, innermost first, then the
//     clock moves, then the enter steps of every phase it enters, outermost first; @turns filters
//     by the turn of the phase ending or beginning. Which phase comes next is judged by the
//     PhaseGate on the position as the phase ends, before its end steps run.
// ----------------------------------------------
#pragma once
#include "hexengine/PhaseCursor.h"
#include "hexengine/Steps.h"
#include "hexrules/Package.h"

#include <string>
#include <vector>

namespace HexEngine::Sequence {

  struct Env {
    const HexRules::GameDefinition& definition;
    const Policies& policies;  // policies.steps and policies.phases set
    PrngStreams& streams;
    HexSearch::SearchScratch& scratch;
    EventSink& sink;
  };

  // Pure selections, exposed for the tests.
  std::vector<const HexRules::Step*> commandSteps(const PhaseCursor&, HexModel::PhaseId, int turn, HexRules::StepAt,
                                                  const std::string& verb);
  struct Boundary {
    std::vector<const HexRules::Step*> end;    // innermost phase first
    PhaseCursor::Stop next;
    std::vector<const HexRules::Step*> enter;  // outermost phase first
  };
  Boundary boundary(const PhaseCursor&, const PhaseCursor::Stop& current, const PhaseCursor::ActiveFilter&);

  // Runs the before-command checks; throws when one refuses.
  void check(const Env&, const Position&, const Command&, const std::string& verb);
  // Runs the after-command effects of the phase the command was issued in (`issued`).
  Position afterCommand(const Env&, const Position& applied, const Command&, const std::string& verb,
                        const HexModel::TurnClock& issued);
  // The end steps, the clock's move to the next stop, the enter steps.
  Position endPhase(const Env&, const Position&, const Command&);

}  // namespace HexEngine::Sequence
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------

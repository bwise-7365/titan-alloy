// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
// PGG's sequence of play as registered behaviours. The rules document's phase steps say where each
// runs (game_rules/xml/panzergruppe-guderian.xml, <sequence>); this module says what each does:
//   PggStepsCommand.cpp  before-command checks and after-command effects;
//   PggStepsPhase.cpp    enter and end effects;
//   PggStepsVerbs.cpp    place and the PGG verbs.
// ----------------------------------------------
#pragma once
#include "PggArrivals.h"
#include "PggCombat.h"
#include "PggFacts.h"
#include "PggFirstTurn.h"
#include "PggInterdiction.h"
#include "PggMovement.h"
#include "PggOverrun.h"
#include "PggStacking.h"
#include "PggSupply.h"
#include "PggVictory.h"
#include "PggZoc.h"

#include "hexengine/Steps.h"

#include <stdexcept>
#include <string>

namespace Pgg {

  struct PggParts {
    const PggFacts& facts;
    const PggZoc& zoc;
    const PggMovement& movement;
    const PggSupply& supply;
    const PggStacking& stacking;
    const PggCombat& combat;
    const PggInterdiction& interdiction;
    const PggArrivals& arrivals;
    const PggFirstTurn& firstTurn;
    const PggVictory& victory;
    const PggOverrun& overrun;
  };

  void registerPggSteps(HexEngine::StepRegistry&, const PggParts&);
  void registerPggCommandSteps(HexEngine::StepRegistry&, const PggParts&);
  void registerPggPhaseSteps(HexEngine::StepRegistry&, const PggParts&);
  void registerPggVerbs(HexEngine::StepRegistry&, const PggParts&);

  // The command a step was run for, as the type its @commands promise; throws naming the step when
  // the rules document's @commands let another command through.
  template <class T, class Call>
  const T&
  commandOf(const Call& call)
  {
    if (const T* typed = std::get_if<T>(&call.command)) {
      return *typed;
    }
    throw std::invalid_argument("Pgg: step '" + call.step.id + "' (does '" + call.step.does +
                                "') was run for another kind of command; check its @commands");
  }

}  // namespace Pgg
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------

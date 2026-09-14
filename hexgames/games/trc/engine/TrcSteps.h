// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
// TRC's sequence of play as registered behaviours (M6b, replacing TrcGame). The rules document's
// phase steps say where each runs (game_rules/xml/the-russian-campaign.xml, <sequence>); this module
// only says what each does:
//   TrcStepsCommand.cpp  before-command checks and after-command effects (the old check and settle);
//   TrcStepsPhase.cpp    enter and end effects (the old enterPhase and endPhase);
//   TrcStepsVerbs.cpp    place and the TRC verbs (the old apply).
// ----------------------------------------------
#pragma once
#include "TrcAir.h"
#include "TrcArrivals.h"
#include "TrcCombat.h"
#include "TrcControl.h"
#include "TrcFacts.h"
#include "TrcMandatory.h"
#include "TrcPolitics.h"
#include "TrcRail.h"
#include "TrcSpecialMoves.h"
#include "TrcStacking.h"
#include "TrcSupply.h"
#include "TrcVictory.h"
#include "TrcWeather.h"
#include "TrcZoc.h"

#include "hexengine/Steps.h"

#include <stdexcept>
#include <string>

namespace Trc {

  struct TrcParts {
    const TrcFacts& facts;
    const TrcZoc& zoc;
    const TrcWeather& weather;
    const TrcStacking& stacking;
    const TrcSupply& supply;
    const TrcCombat& combat;
    const TrcAir& air;
    const TrcControl& control;
    const TrcRail& rail;
    const TrcVictory& victory;
    const TrcMandatory& mandatory;
    const TrcArrivals& arrivals;
    const TrcSpecialMoves& special;
    const TrcPolitics& politics;
  };

  void registerTrcSteps(HexEngine::StepRegistry&, const TrcParts&);
  void registerTrcCommandSteps(HexEngine::StepRegistry&, const TrcParts&);
  void registerTrcPhaseSteps(HexEngine::StepRegistry&, const TrcParts&);
  void registerTrcVerbs(HexEngine::StepRegistry&, const TrcParts&);

  // The command a step was run for, as the type its @commands promise; throws naming the step when
  // the rules document's @commands let another command through.
  template <class T, class Call>
  const T&
  commandOf(const Call& call)
  {
    if (const T* typed = std::get_if<T>(&call.command)) {
      return *typed;
    }
    throw std::invalid_argument("Trc: step '" + call.step.id + "' (does '" + call.step.does +
                                "') was run for another kind of command; check its @commands");
  }

}  // namespace Trc
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------

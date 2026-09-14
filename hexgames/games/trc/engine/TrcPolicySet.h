// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
// The whole TRC policy set: owns one of each TRC policy over one GameDefinition (which must outlive
// it) and hands a Session its Policies. claims() is the union of every policy's claimed rule ids
// and the rules named by the phase steps whose behaviour TRC registers (M6b), which is what
// trc_ledger_test checks the ledger against.
// ----------------------------------------------
#pragma once
#include "TrcAir.h"
#include "TrcArrivals.h"
#include "TrcCombat.h"
#include "TrcCommandGrammar.h"
#include "TrcControl.h"
#include "TrcFacts.h"
#include "TrcSteps.h"
#include "TrcMandatory.h"
#include "TrcMovement.h"
#include "TrcPhaseGate.h"
#include "TrcPolitics.h"
#include "TrcRail.h"
#include "TrcRetreat.h"
#include "TrcSpecialMoves.h"
#include "TrcStacking.h"
#include "TrcState.h"
#include "TrcSupply.h"
#include "TrcVictory.h"
#include "TrcWeather.h"
#include "TrcZoc.h"

#include "hexengine/GameNames.h"

namespace Trc {

  class TrcPolicySet {
  public:
    explicit TrcPolicySet(const HexRules::GameDefinition&);
    TrcPolicySet(const TrcPolicySet&) = delete;
    TrcPolicySet& operator=(const TrcPolicySet&) = delete;

    const HexEngine::Policies& policies() const { return policies_; }
    const HexEngine::GameNames& names() const { return names_; }
    const TrcFacts& facts() const { return facts_; }
    const TrcCombat& combat() const { return combat_; }
    const TrcSupply& supply() const { return supply_; }
    const TrcWeather& weather() const { return weather_; }
    const TrcMovement& movement() const { return movement_; }
    const TrcStacking& stacking() const { return stacking_; }
    const TrcZoc& zoc() const { return zoc_; }
    const TrcVictory& victory() const { return victory_; }
    const HexEngine::StepRegistry& steps() const { return steps_; }
    std::vector<std::string_view> claims() const;

  private:
    HexEngine::GameNames names_;
    TrcFacts facts_;
    TrcZoc zoc_;
    TrcWeather weather_;
    TrcMovement movement_;
    TrcStacking stacking_;
    TrcSupply supply_;
    TrcCombat combat_;
    TrcAir air_;
    TrcRetreat retreat_;
    TrcControl control_;
    TrcRail rail_;
    TrcVictory victory_;
    TrcMandatory mandatory_;
    TrcArrivals arrivals_;
    TrcSpecialMoves special_;
    TrcPolitics politics_;
    TrcPhaseGate phases_;
    TrcCommandGrammar grammar_;
    TrcStateCodec state_;
    HexEngine::NoObligationCodec obligationCodec_;  // TRC pushes no game obligations
    HexEngine::StepRegistry steps_;  // the engine's behaviours and TRC's, for the rules' phase steps
    HexEngine::Policies policies_;
  };

}  // namespace Trc
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------

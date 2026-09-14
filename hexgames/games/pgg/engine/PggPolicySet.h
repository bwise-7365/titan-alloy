// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
// The whole PGG policy set: owns one of each PGG part over one GameDefinition (which must outlive it)
// and hands a Session its Policies. claims() is the union of every part's claimed rule ids and the
// rules named by the phase steps whose behaviour PGG registers, which is what pgg_ledger_test checks
// the ledger against.
// ----------------------------------------------
#pragma once
#include "PggArrivals.h"
#include "PggBattle.h"
#include "PggCombat.h"
#include "PggCommandGrammar.h"
#include "PggFacts.h"
#include "PggFirstTurn.h"
#include "PggInterdiction.h"
#include "PggMovement.h"
#include "PggObligations.h"
#include "PggOverrun.h"
#include "PggPhaseGate.h"
#include "PggRetreat.h"
#include "PggStacking.h"
#include "PggState.h"
#include "PggSteps.h"
#include "PggSupply.h"
#include "PggVictory.h"
#include "PggZoc.h"

#include "hexengine/GameNames.h"

namespace Pgg {

  class PggPolicySet {
  public:
    explicit PggPolicySet(const HexRules::GameDefinition&);
    PggPolicySet(const PggPolicySet&) = delete;
    PggPolicySet& operator=(const PggPolicySet&) = delete;

    const HexEngine::Policies& policies() const { return policies_; }
    const HexEngine::GameNames& names() const { return names_; }
    const PggFacts& facts() const { return facts_; }
    const PggZoc& zoc() const { return zoc_; }
    const PggMovement& movement() const { return movement_; }
    const PggSupply& supply() const { return supply_; }
    const PggStacking& stacking() const { return stacking_; }
    const PggCombat& combat() const { return combat_; }
    const PggVictory& victory() const { return victory_; }
    const PggStateCodec& stateCodec() const { return state_; }
    const PggBattleCodec& battleCodec() const { return battleCodec_; }
    const HexEngine::StepRegistry& steps() const { return steps_; }
    std::vector<std::string_view> claims() const;

  private:
    HexEngine::GameNames names_;
    PggFacts facts_;
    PggZoc zoc_;
    PggMovement movement_;
    PggSupply supply_;
    PggStacking stacking_;
    PggCombat combat_;
    PggRetreat retreat_;
    PggObligations obligations_;
    PggInterdiction interdiction_;
    PggArrivals arrivals_;
    PggFirstTurn firstTurn_;
    PggVictory victory_;
    PggOverrun overrun_;
    PggPhaseGate phases_;
    PggCommandGrammar grammar_;
    PggStateCodec state_;
    PggBattleCodec battleCodec_;
    HexEngine::StepRegistry steps_;  // the engine's behaviours and PGG's, for the rules' phase steps
    HexEngine::Policies policies_;
  };

}  // namespace Pgg
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------

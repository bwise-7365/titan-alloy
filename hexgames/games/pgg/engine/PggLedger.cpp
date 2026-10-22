// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
#include "PggLedger.h"

#include <array>

namespace Pgg {

  namespace {

    using HexRules::Implemented;
    using HexRules::LedgerEntry;
    using HexRules::OptionalNotImplemented;

    const std::array<LedgerEntry, 47> kLedger{{
        // ---- sequence ----
        {"soviet-first-turn-die", Implemented{"Pgg::PggFirstTurn (step roll-first-turn-armies, check-first-turn)"}},
        {"soviet-first-turn-mandatory", Implemented{"Pgg::PggFirstTurn::check (step check-first-turn)"}},
        // ---- stacking ----
        {"stacking-soviet-leader-bonus", Implemented{"Pgg::PggStacking (step repair-stacking)"}},
        {"divisional-integration", Implemented{"Pgg::PggCombat::integratedP"}},
        // ---- zones of control ----
        {"zoc-disrupted-none", Implemented{"Pgg::PggZoc"}},
        {"zoc-mutual", Implemented{"Pgg::PggZoc"}},
        {"zoc-stop-and-leave", Implemented{"Pgg::PggMovement (8.13), HexEngine::Session::reachable (the stop)"}},
        // ---- movement ----
        {"overrun-eligibility", Implemented{"Pgg::PggOverrun (step check-movement-supply records the radius)"}},
        {"leader-movement", Implemented{"Pgg::PggMovement (MoveClass::Leader), Pgg::PggFacts::railPoints"}},
        {"soviet-forbidden-hexrows", Implemented{"Pgg::PggMovement, Pgg::PggOverrun, Pgg::BattleFlow (advance)"}},
        {"out-of-supply-movement", Implemented{"Pgg::PggMovement::fullHalves (step check-movement-supply)"}},
        // ---- supply ----
        {"supply-never-fatal", Implemented{"Pgg::PggSupply (marks units, never removes them)"}},
        {"supply-first-turn", Implemented{"Pgg::PggSupply, Pgg::PggArrivals (PggSideState::entered)"}},
        {"supply-rail-cut-blocks-supply", Implemented{"Pgg::PggMovement (rail only in rail-move), Pgg::PggSupply"}},
        // ---- combat ----
        {"split-results", Implemented{"Pgg::PggObligations, Pgg::BattleFlow"}},
        {"overrun-disruption", Implemented{"Pgg::PggObligations, Pgg::PggZoc, Pgg::PggMovement, Pgg::PggSupply"}},
        {"no-strength-units", Implemented{"Pgg::BattleFlow::reveal, Pgg::PggCombat::assess"}},
        {"soviet-attack-radius", Implemented{"step check-attack, Pgg::PggSupply::withinRadiusP, Pgg::PggCombat (10.36)"}},
        // ---- retreat ----
        {"advance-after-combat", Implemented{"Pgg::BattleFlow::advanceStep"}},
        {"no-advance-overrun", Implemented{"Pgg::BattleFlow::advanceStep"}},
        {"forbidden-hexrow-retreat", Implemented{"Pgg::PggRetreat"}},
        // ---- victory ----
        {"german-vp-occupation", Implemented{"Pgg::PggVictory (steps note-control, declare-victory)"}},
        {"german-vp-swf", Implemented{"Pgg::PggVictory::germanPoints"}},
        {"soviet-vp-eliminated-division", Implemented{"Pgg::PggVictory::sovietPoints"}},
        {"soviet-vp-recapture", Implemented{"Pgg::PggVictory (steps record-german-cities, score-recaptures)"}},
        {"historical-result", OptionalNotImplemented{}},
        // ---- untried units ----
        {"untried-placement", Implemented{"Pgg::PggArrivals (drawn face down), games/pgg/scenario/pgg-1941.xml"}},
        {"untried-reveal-timing", Implemented{"Pgg::BattleFlow::reveal (the battle's first stage)"}},
        {"untried-zoc", Implemented{"Pgg::PggZoc"}},
        {"untried-dead-pile", Implemented{"Pgg::Units::eliminate, Pgg::PggArrivals (draws)"}},
        // ---- interdiction ----
        {"german-interdiction-placement", Implemented{"Pgg::PggInterdiction::interdict"}},
        {"german-interdiction-effect", Implemented{"Pgg::PggMovement (step remove-air-interdiction)"}},
        {"soviet-interdiction-effect", Implemented{"Pgg::PggInterdiction, Pgg::PggMovement, Pgg::PggSupply"}},
        // ---- reinforcement ----
        {"soviet-provisional-reinforcement", Implemented{"Pgg::PggArrivals::rollProvisional (provisional areas)"}},
        {"soviet-swf-reinforcement", Implemented{"Pgg::PggArrivals::reinforce"}},
        {"reinforcement-turn-12", Implemented{"Pgg::sovietSchedule"}},
        {"german-reinforcement-26th", Implemented{"Pgg::germanSchedule"}},
        {"german-reinforcement-268th", Implemented{"Pgg::germanSchedule"}},
        // ---- rail cut ----
        {"rail-cut-place", Implemented{"Pgg::PggInterdiction::cutRail, Pgg::PggMovement"}},
        {"rail-cut-repair", Implemented{"Pgg::PggInterdiction::notePassage, liftCut"}},
        {"integration-order", Implemented{"Pgg::PggCombat::assess"}},
        // ---- optional systems ----
        {"german-side-bidding", OptionalNotImplemented{}},
        {"step-loss-vp-variant", OptionalNotImplemented{}},
        // ---- ordering hazards ----
        {"order-integration-before-terrain", Implemented{"Pgg::PggCombat::assess"}},
        {"order-reveal-mid-phase", Implemented{"Pgg::PggCombat::assess (at the instant), Pgg::BattleFlow::reveal"}},
        {"order-interdiction-removal", Implemented{"rules step remove-air-interdiction (end of soviet-move)"}},
        {"order-rail-cut-latency", Implemented{"Pgg::PggMovement (PggState::railRepaired)"}},
    }};

  }  // namespace

  std::span<const HexRules::LedgerEntry>
  ledger()
  {
    return kLedger;
  }

}  // namespace Pgg
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------

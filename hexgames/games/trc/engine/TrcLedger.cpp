// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
#include "TrcLedger.h"

#include <array>

namespace Trc {

  namespace {

    using HexRules::Common;
    using HexRules::EngineFeature;
    using HexRules::Implemented;
    using HexRules::LedgerEntry;
    using HexRules::OptionalNotImplemented;
    using HexRules::OutOfScope;

    const std::array<LedgerEntry, 54> kLedger{{
        // ---- sequence ----
        {"second-impulse", Implemented{"Trc::TrcMovement (allowance by impulse), Trc::TrcPhaseGate (no rail or sea)"}},
        {"withdrawals-first", OutOfScope{"no input document carries a withdrawal schedule: the 2020 counter backs "
                                         "print no withdrawal dates, so there is nothing to withdraw first"}},
        // ---- stacking ----
        {"stacking-sizes", Implemented{"Trc::TrcStacking"}},
        {"stacking-no-value", Implemented{"Trc::TrcStacking"}},
        // ---- zones of control ----
        {"zoc-contested", Implemented{"Trc::TrcControl"}},
        {"zoc-pinning", Implemented{"Trc::TrcMovement"}},
        {"zoc-partisan", Implemented{"Trc::TrcZoc, Trc::TrcSupply, Trc::TrcControl"}},
        {"zoc-battlegroup", OptionalNotImplemented{}},
        {"zoc-av-exception", Implemented{"Trc::TrcSpecialMoves::automaticVictory"}},
        // ---- movement ----
        {"hq-movement", Implemented{"Trc::TrcMovement"}},
        {"leader-movement", Implemented{"Trc::TrcMovement"}},
        {"leader-loss", Implemented{"Trc::TrcMovement, steps note-leaders, wake-leader-freeze, spend-leader-freeze"}},
        {"national-rows", OptionalNotImplemented{}},
        {"strategic-movement", OptionalNotImplemented{}},
        // ---- supply ----
        {"supply-exempt", Implemented{"Trc::TrcSupply"}},
        {"supply-partisan-city", Implemented{"Trc::TrcSupply"}},
        {"combat-supply-halving", Implemented{"Trc::TrcCombat, Trc::TrcSupply::combatSuppliedP"}},
        // ---- combat ----
        {"eliminate-vs-surrender", Common{EngineFeature::Spaces}},
        {"control-frozen-ex", Implemented{"Trc::TrcControl, HexModel::Position::resolution"}},
        {"av-trap", Implemented{"Trc::TrcMandatory"}},
        {"av-russian-date", Implemented{"Trc::TrcSpecialMoves::automaticVictory"}},
        {"air-range", Implemented{"Trc::TrcAir"}},
        {"fortress-cities", OptionalNotImplemented{}},
        // ---- retreat ----
        {"retreat-woods", Implemented{"Trc::TrcRetreat::fate, Trc::TrcCombat (AR/DR to C)"}},
        {"retreat-leaders-workers", Implemented{"Trc::TrcRetreat::fate, step workers-surrender"}},
        // ---- weather ----
        {"weather-fixed-turns", Implemented{"Trc::TrcWeather"}},
        {"weather-drm", Implemented{"Trc::TrcWeather"}},
        {"weather-effects", Implemented{"Trc::TrcMovement, Trc::TrcSupply, Trc::TrcAir, Trc::TrcSpecialMoves"}},
        // ---- rail ----
        {"rail-railhead-advance", Implemented{"Trc::TrcRail"}},
        {"rail-railhead-pushback", Implemented{"Trc::TrcRail"}},
        {"rail-prior-turn", Implemented{"Trc::TrcRail (conversions only in end phases)"}},
        {"rail-city-conversion", Implemented{"Trc::TrcRail"}},
        {"rail-junction", Implemented{"Trc::TrcControl, Trc::TrcFacts::junctionP"}},
        // ---- control ----
        {"control-last-toucher", Implemented{"Trc::TrcControl"}},
        // ---- politics ----
        {"surrender-hungary", Implemented{"Trc::TrcPolitics (needs the sheet's countries membership)"}},
        {"surrender-finland-rumania", Implemented{"Trc::TrcPolitics"}},
        {"surrender-italy", Implemented{"Trc::TrcPolitics"}},
        {"surrender-finland-1944", Implemented{"Trc::TrcPolitics, Trc::TrcMovement, Trc::TrcControl"}},
        {"surrender-effect", Implemented{"Trc::TrcPolitics::surrender"}},
        {"garrison-bucharest", OutOfScope{"the summoned 'named Axis mountain corps' cannot be identified: no counter "
                                          "of the 2020 set is bound to unit type mountain"}},
        {"garrison-warsaw", Implemented{"Trc::TrcPolitics"}},
        // ---- off-board encirclement ----
        {"encirclement-debt", OutOfScope{"units cannot exit the map: the sheet has no off-board edge boxes and the "
                                         "engine no exit move, so no debt can arise"}},
        // ---- partisans ----
        {"partisan-placement", Implemented{"Trc::TrcArrivals::placePartisan (needs the sheet's countries membership)"}},
        {"partisan-cycle", Implemented{"Trc::TrcPolitics"}},
        // ---- reinforcement and replacement ----
        {"reinforcement-schedule", Implemented{"Trc::TrcArrivals (arrival by delay; the dates are not printed)"}},
        {"omb-diversion", Implemented{"Trc::TrcArrivals"}},
        {"south-edge-entry", Implemented{"Trc::TrcArrivals"}},
        {"axis-replacements", Implemented{"Trc::TrcArrivals::replace"}},
        {"russian-replacements", Implemented{"Trc::TrcArrivals::replace, grantReplacementPoints"}},
        {"industrial-evacuation", OptionalNotImplemented{}},
        {"battlegroups", OptionalNotImplemented{}},
        {"bidding", OptionalNotImplemented{}},
        // ---- ordering hazards ----
        {"order-railheads-before-supply", Implemented{"rules steps convert-rail then eliminate-unsupplied (end phases)"}},
        {"order-partisan-removal", Implemented{"rules step remove-exposed-partisans (Axis movement phases), Trc::TrcPolitics"}},
    }};

  }  // namespace

  std::span<const HexRules::LedgerEntry>
  ledger()
  {
    return kLedger;
  }

}  // namespace Trc
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------

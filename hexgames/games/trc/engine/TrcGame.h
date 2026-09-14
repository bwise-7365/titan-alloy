// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
// TRC's sequence of play around the engine (the GameAdjudicator). check() refuses what a TRC rule
// forbids before the engine adjudicates; apply() runs place and the TRC verbs; settle() follows
// every command (rail capacity, air used, control, stacking after a battle, a lost leader, a
// worker's surrender, the Warsaw trigger); endPhase() runs the rules that close a phase and
// enterPhase() those that open one. The order inside an end phase is 4.1.6: railheads first, then
// supply (order-railheads-before-supply).
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

  class TrcGame : public HexEngine::GameAdjudicator {
  public:
    explicit TrcGame(TrcParts);
    std::span<const std::string_view> claims() const override;
    void check(const Ctx&, const HexEngine::Command&) const override;
    Position apply(const Ctx&, const HexEngine::Command&, HexEngine::PrngStreams&, HexEngine::EventSink&) const override;
    Position settle(const Ctx&, const HexEngine::Command&, HexEngine::EventSink&) const override;
    Position endPhase(const Ctx&, HexSearch::SearchScratch&, HexEngine::EventSink&) const override;
    Position enterPhase(const Ctx&, HexEngine::PrngStreams&, HexEngine::EventSink&) const override;

  private:
    void checkAttack(const Ctx&, const HexEngine::DeclareAttack&) const;
    void checkMove(const Ctx&, const HexEngine::MoveUnit&) const;
    Position settleMove(const Ctx&, const HexEngine::MoveUnit&) const;
    Position repairStacking(const Ctx&, HexEngine::EventSink&) const;
    Position noteLeaders(const Ctx&) const;
    Position workersSurrender(const Ctx&, HexEngine::EventSink&) const;
    Position closeSideTurn(const Ctx&, SideId, HexSearch::SearchScratch&, HexEngine::EventSink&) const;
    Position openImpulse(const Ctx&, SideId, Impulse, HexEngine::PrngStreams&, HexEngine::EventSink&) const;
    Position startTurn(const Ctx&, HexEngine::PrngStreams&, HexEngine::EventSink&) const;
    TrcParts parts_;
  };

}  // namespace Trc
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------

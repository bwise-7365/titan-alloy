// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
// The Soviet first-turn rules (5.2). As the first Soviet Movement Phase opens, a die for the 16th and
// for the 19th Army: on 4-6 that army stands for the turn (5.21), its units moving at most one hex, and
// only off an overstacked hex (5.22). The 13th and 20th Armies may not rail, never move west, never
// re-enter a hex, must spend their whole allowance, and may not end the phase in hex-columns 01 or 02
// (5.23). The armies' units are named in the scenario's state.
// ----------------------------------------------
#pragma once
#include "PggFacts.h"
#include "PggMovement.h"
#include "PggStacking.h"

#include "hexengine/Command.h"

namespace Pgg {

  class PggFirstTurn {
  public:
    PggFirstTurn(const PggFacts&, const PggMovement&, const PggStacking&);
    static std::span<const std::string_view> claims();
    Position rollArmies(const Ctx&, HexEngine::PrngStreams&, HexEngine::EventSink&) const;
    void check(const Ctx&, const HexEngine::Command&) const;  // throws naming the rule

  private:
    void checkMove(const Ctx&, const HexEngine::MoveUnit&) const;
    void checkEnd(const Ctx&) const;
    bool mayGoOnP(const Ctx&, UnitId, HexIndex at, int halvesLeft, const std::vector<HexIndex>& visited) const;

    const PggFacts& facts_;
    const PggMovement& movement_;
    const PggStacking& stacking_;
  };

}  // namespace Pgg
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------

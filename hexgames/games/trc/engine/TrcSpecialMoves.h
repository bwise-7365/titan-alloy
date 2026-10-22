// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
// The TRC moves that are commands of their own rather than walks: sea movement (10.0, the sheet's
// two sea-area panels), paradrops (18.0) and automatic victory (16.0).
// ----------------------------------------------
#pragma once
#include "TrcCombat.h"
#include "TrcFacts.h"
#include "TrcWeather.h"
#include "TrcZoc.h"

namespace Trc {

  class TrcSpecialMoves {
  public:
    TrcSpecialMoves(const TrcFacts&, const TrcZoc&, const TrcWeather&, const TrcCombat&);
    static std::span<const std::string_view> claims();

    // sea-move units=<one counter> to=<hex or omb>
    Position seaMove(const Ctx&, const HexEngine::GameCommand&, HexEngine::PrngStreams&, HexEngine::EventSink&) const;
    // paradrop units=<one counter> to=<hex>
    Position paradrop(const Ctx&, const HexEngine::GameCommand&, HexEngine::EventSink&) const;
    // av-attack units=<counters> target=<hex>
    Position automaticVictory(const Ctx&, const HexEngine::GameCommand&, HexEngine::EventSink&) const;

  private:
    std::vector<UnitId> unitsOf(const Ctx&, const std::string& list) const;
    int portsHeld(const Ctx&, SideId, SeaArea) const;
    const TrcFacts& facts_;
    const TrcZoc& zoc_;
    const TrcWeather& weather_;
    const TrcCombat& combat_;
  };

}  // namespace Trc
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------

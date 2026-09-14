// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
// Every `place` command in TRC: a reinforcement or a unit held in the Off-Map Units Box entering
// the map (20.0, 20.1, 20.4, 20.5), a unit railed off its own edge into the box (20.1), a
// replacement rebuilt from its side's pool (21.0, 22.0), and a partisan placed or relocated
// (19.1). Reinforcements and held units wait in the box; one whose @delay is still above zero has
// not arrived. They enter on a friendly city or the owner's board edge, costing no movement.
// ----------------------------------------------
#pragma once
#include "TrcFacts.h"
#include "TrcMovement.h"
#include "TrcWeather.h"
#include "TrcZoc.h"

namespace Trc {

  class TrcArrivals {
  public:
    TrcArrivals(const TrcFacts&, const TrcZoc&, const TrcWeather&);
    static std::span<const std::string_view> claims();

    Position place(const Ctx&, const HexEngine::Place&, HexEngine::EventSink&) const;
    // The Russian replacement points of a turn (22.0, 22.7): workers on the map, doubled from
    // May/June 1943, plus the Archangel die from January/February 1942.
    Position grantReplacementPoints(const Ctx&, HexEngine::PrngStreams&, HexEngine::EventSink&) const;

  private:
    Position enter(const Ctx&, UnitId, HexIndex, HexEngine::EventSink&) const;
    Position divert(const Ctx&, UnitId, HexEngine::EventSink&) const;
    Position replace(const Ctx&, UnitId, HexIndex, HexEngine::EventSink&) const;
    Position placePartisan(const Ctx&, UnitId, HexIndex, HexEngine::EventSink&) const;
    void checkEntryHex(const Ctx&, SideId, HexIndex) const;
    std::string axisCategory(const Ctx&, UnitId) const;
    const TrcFacts& facts_;
    const TrcZoc& zoc_;
    const TrcWeather& weather_;
  };

}  // namespace Trc
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------

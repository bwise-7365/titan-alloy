// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
// TRC normal and rail movement. Every land hex costs one hex; terrain only forces stops (woods,
// mountain, swamp except in snow, and after the Kerch Strait). The allowance comes from the
// Movement Allowance Chart by unit class, weather and impulse (TrcMovementChart.cpp), with the
// rules that override it: one move per phase, HQs only in the second impulse at full allowance,
// leaders only by rail or sea, pinning in the second impulse, a lost leader's country frozen for
// an impulse. Rail movement is first impulse only, along links the side owns, never in or through
// an enemy zone, within the turn's capacity.
// ----------------------------------------------
#pragma once
#include "TrcFacts.h"
#include "TrcWeather.h"
#include "TrcZoc.h"

namespace Trc {

  class TrcMovement : public HexEngine::MovementPolicy {
  public:
    TrcMovement(const TrcFacts&, const TrcZoc&, const TrcWeather&);
    std::span<const std::string_view> claims() const override;
    HexEngine::EntryVerdict enter(const Ctx&, UnitId, HexIndex from, Direction, ModeId) const override;
    Budget allowance(const Ctx&, UnitId, ModeId) const override;
    bool stopsInP(const Ctx&, UnitId, HexIndex, ModeId) const override;

    static int railCapacity(bool axisP, Weather);  // 9.2: six (three in snow) for the Axis, five Russian
    int railMovesUsed(const Position&, SideId) const;

  private:
    HexEngine::EntryVerdict enterNormally(const Ctx&, UnitId, HexIndex from, HexIndex to, Direction) const;
    HexEngine::EntryVerdict enterByRail(const Ctx&, UnitId, HexIndex from, HexIndex to) const;
    int normalHexes(const Ctx&, UnitId) const;
    int railHexes(const Ctx&, UnitId) const;
    std::optional<Impulse> impulseNow(const Ctx&) const;

    const TrcFacts& facts_;
    const TrcZoc& zoc_;
    const TrcWeather& weather_;
  };

  // The Movement Allowance Chart: hexes a unit may move by class, weather and impulse.
  int chartAllowance(const TrcFacts&, UnitId, Weather, Impulse, int printed);

}  // namespace Trc
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------

// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
// Movement (6.0): points in halves (a road costs half a point), one move per unit per phase, a
// fresh allowance in each friendly movement phase and in the Mechanized Movement Phase for everything
// but infantry divisions (6.23, 4.2), halved without supply (6.25, 11.31), none while Disrupted
// (6.62) or halted by an overrun (6.52), and a unit freed by a successful overrun moves on with what
// it has left (6.51). A unit in an enemy zone may not leave it (8.13). Soviet units may not enter the
// westernmost two hex-columns on turns 1-6 (6.4). Air Interdiction adds a point to enter its hex (four
// by rail) for the other side (13.31, 13.32, 13.43). Rail movement (6.3): Soviet only, thirty hexes,
// rail hex to connected rail hex, never into an enemy zone, never through a Rail Cut or over a line
// repaired this turn, eight combat units a turn with armour counting three.
// ----------------------------------------------
#pragma once
#include "PggFacts.h"
#include "PggZoc.h"

#include <optional>
#include <vector>

namespace Pgg {

  class PggMovement : public HexEngine::MovementPolicy {
  public:
    PggMovement(const PggFacts&, const PggZoc&);
    std::span<const std::string_view> claims() const override;
    HexEngine::EntryVerdict enter(const Ctx&, UnitId, HexIndex from, Direction, ModeId) const override;
    Budget allowance(const Ctx&, UnitId, ModeId) const override;
    bool stopsInP(const Ctx&, UnitId, HexIndex, ModeId) const override;  // never: zones stop units (Session)

    // The cost of a whole path for one unit, in halves; nullopt when any step may not be taken.
    std::optional<int> pathHalves(const Ctx&, UnitId, const std::vector<HexIndex>& path, ModeId) const;
    // The entry cost of a reinforcement's first hex (14.0), in halves.
    int entryHalves(const Ctx&, UnitId, HexIndex) const;
    // Whether the unit's own side is moving in this phase and this unit may move in it at all.
    bool movementPhaseP(const Ctx&, UnitId) const;
    // The printed allowance, halved without supply, in halves (before anything is spent).
    int fullHalves(const Ctx&, UnitId) const;
    // What the unit has left this phase, in halves, whether or not it has moved: an overrun follows the
    // move that brought the units up (6.51). Zero outside its movement phase, Disrupted or halted.
    int remainingHalves(const Ctx&, UnitId) const;
    static constexpr int kRailHalves = 60;  // thirty hexes (6.33)
    static constexpr int kRailUnits = 8;    // 6.31

  private:
    std::optional<int> normalHalves(const Ctx&, UnitId, HexIndex from, Direction) const;
    std::optional<int> railHalves(const Ctx&, UnitId, HexIndex from, Direction) const;
    int interdictionHalves(const Ctx&, SideId mover, HexIndex, bool railP) const;
    int normalAllowance(const Ctx&, UnitId) const;
    int railAllowance(const Ctx&, UnitId) const;

    const PggFacts& facts_;
    const PggZoc& zoc_;
  };

}  // namespace Pgg
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------

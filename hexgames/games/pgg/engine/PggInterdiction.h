// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
// Interdiction (13.0) and rail cuts (errata 6.37). "interdict target": the German Player places his
// three Air Interdiction Markers before the game and in each Air Interdiction Phase, one to a hex, no
// farther east than hex-column 40 unless he took Smolensk a turn ago and it has a line to the west
// edge (13.2); they come off at the end of the Soviet Movement Phase (13.33). The Soviet Player places
// his one marker in his Interdiction Phase on at most three turns, never turn 12, never on a German
// unit (13.41, 13.45); it comes off at the end of the German Player-Turn (13.46). "cut-rail target":
// a Railroad hex a German combat unit entered this movement phase is cut, six at most; "lift-cut
// target" frees the marker the same way. A Soviet unit's normal move through a cut repairs it, and
// rail movement over it resumes the next turn.
// ----------------------------------------------
#pragma once
#include "PggFacts.h"
#include "PggSupply.h"

#include "hexengine/Command.h"

namespace Pgg {

  class PggInterdiction {
  public:
    PggInterdiction(const PggFacts&, const PggSupply&);
    static std::span<const std::string_view> claims();

    Position interdict(const Ctx&, const HexEngine::GameCommand&, HexEngine::EventSink&) const;
    Position cutRail(const Ctx&, const HexEngine::GameCommand&, HexEngine::EventSink&) const;
    Position liftCut(const Ctx&, const HexEngine::GameCommand&, HexEngine::EventSink&) const;

    Position removeAir(const Ctx&, HexEngine::EventSink&) const;
    Position removeSoviet(const Ctx&, HexEngine::EventSink&) const;
    Position notePassage(const Ctx&, const HexEngine::MoveUnit&, HexEngine::EventSink&) const;

    static constexpr std::size_t kAirMarkers = 3;
    static constexpr std::size_t kRailCuts = 6;
    static constexpr int kSovietTurns = 3;
    static constexpr int kEastmostColumn = 40;

  private:
    HexIndex targetOf(const HexEngine::GameCommand&) const;
    Position placeAir(const Ctx&, HexIndex, HexEngine::EventSink&) const;
    Position placeSoviet(const Ctx&, HexIndex, HexEngine::EventSink&) const;
    void checkGermanMovement(const Ctx&, const HexEngine::GameCommand&) const;

    const PggFacts& facts_;
    const PggSupply& supply_;
  };

}  // namespace Pgg
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------

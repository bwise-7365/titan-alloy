// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
// Reinforcements (14.0, 16.0). German counters and scheduled Soviet Leaders wait in their arrivals box
// with a delay of turns; once it is spent, "place" brings one onto a hex of its entrance area in its
// side's first movement phase, paying that hex's entry cost (14.0). Soviet divisions are drawn at
// random by type from the counter pool, face down, and from the dead pile once the pool has none of
// the type (12.1, 12.4, 14.14): "reinforce type where source" draws one onto a hex of an entrance still
// owed a division of that type by the schedule (source scheduled), or onto a south-edge hex at or east
// of Z as a South-Western Front division, five a turn and ten a game from turn 2 (source
// south-western, 14.2). Each Soviet Movement Phase opens with the turn's schedule owed (none on turn
// 12) and one Provisional Reinforcement rolled for and placed at once (14.1). Every arrival is in
// supply for its first player-turn (11.33).
// ----------------------------------------------
#pragma once
#include "PggFacts.h"
#include "PggMovement.h"
#include "PggZoc.h"

#include "hexengine/Command.h"

namespace Pgg {

  class PggArrivals {
  public:
    PggArrivals(const PggFacts&, const PggMovement&, const PggZoc&);
    static std::span<const std::string_view> claims();

    Position place(const Ctx&, const HexEngine::Place&, HexEngine::EventSink&) const;
    Position reinforce(const Ctx&, const HexEngine::GameCommand&, HexEngine::PrngStreams&, HexEngine::EventSink&) const;
    Position scheduleOwed(const Ctx&) const;
    Position rollProvisional(const Ctx&, HexEngine::PrngStreams&, HexEngine::EventSink&) const;
    Position ageArrivals(const Ctx&) const;

    static constexpr int kSwfPerTurn = 5;
    static constexpr int kSwfPerGame = 10;

  private:
    // A random counter of the arm, pool first, dead pile next (Setup stream); nullopt when none is left.
    std::optional<UnitId> draw(const Ctx&, Arm, HexEngine::PrngStreams&, HexEngine::EventSink&) const;
    void enter(const Ctx&, Position&, UnitId, HexIndex, HexEngine::EventSink&) const;
    std::optional<HexIndex> provisionalHex(const Ctx&, Area) const;

    const PggFacts& facts_;
    const PggMovement& movement_;
    const PggZoc& zoc_;
  };

}  // namespace Pgg
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------

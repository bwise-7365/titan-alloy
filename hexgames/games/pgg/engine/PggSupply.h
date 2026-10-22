// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
// Supply (11.0). A line may not pass through a Lake, an enemy unit or an enemy zone of control a
// friendly unit does not negate, and may enter a Swamp hex but not leave it. German (11.1): within
// twenty hexes of a Road hex joined by road to 0120, the road itself so uninterrupted and never
// through the Soviet Interdiction Marker (13.44); or twenty movement points, by the unit's own costs,
// straight to a west-edge hex. Soviet (11.2): a Leader is supplied by a line of any length to the
// east edge; any other unit by a Line of Communications no longer than a working (not Disrupted)
// Leader's rating to a supplied Leader. Units in their first player-turn in play are supplied
// (11.33), and nothing is ever eliminated for lack of supply (11.34).
// ----------------------------------------------
#pragma once
#include "PggFacts.h"
#include "PggZoc.h"

#include <vector>

namespace Pgg {

  class PggSupply : public HexEngine::SupplyTrace {
  public:
    PggSupply(const PggFacts&, const PggZoc&);
    std::span<const std::string_view> claims() const override;
    HexEngine::SupplyReport trace(const Ctx&, HexSearch::SearchScratch&, SideId) const override;

    bool suppliedP(const Ctx&, HexSearch::SearchScratch&, UnitId) const;
    // 10.35, 6.55: a Line of Communications to a working Leader, supplied or not.
    bool withinRadiusP(const Ctx&, HexSearch::SearchScratch&, UnitId) const;
    // 15.11, 13.2: a line of any length from the hex to the west edge, free of Soviet units and of
    // Soviet zones a German unit does not negate.
    bool lineWestP(const Ctx&, HexSearch::SearchScratch&, HexIndex) const;

  private:
    bool passableP(const Ctx&, HexIndex, SideId) const;
    bool throughP(const Ctx&, HexIndex, SideId) const;
    std::vector<bool> roadSupplied(const Ctx&, HexSearch::SearchScratch&) const;
    bool germanSuppliedP(const Ctx&, HexSearch::SearchScratch&, UnitId, const std::vector<bool>& roads) const;
    bool leaderSuppliedP(const Ctx&, HexSearch::SearchScratch&, UnitId) const;
    // Leaders of the side reachable within their rating; `supplied` filters to the supplied ones.
    bool communicatesP(const Ctx&, HexSearch::SearchScratch&, UnitId, const std::vector<UnitId>& leaders) const;
    std::vector<UnitId> workingLeaders(const Ctx&, HexSearch::SearchScratch&, bool suppliedOnlyP) const;

    const PggFacts& facts_;
    const PggZoc& zoc_;
  };

}  // namespace Pgg
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------

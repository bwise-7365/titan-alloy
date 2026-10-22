// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
// TRC rail conversion (9.4). Each rail link has an owner (Position::linkOwner); only links a side
// owned before its turn carry its rail movement and supply, because every conversion happens in
// an end phase. In a side's end phase:
//   railheads -- every rail hex the side's units entered this turn that still traces, along the
//   railroad and clear of enemy zones and enemy cities, back to a friendly city or the owner's
//   edge, has the links of that path converted (the Axis advance 9.4.4; the Russian push-back
//   9.4.5 is the same rule for the other side);
//   cities -- links touching a friendly city convert, and so do whole lines between two friendly
//   cities that no enemy unit, zone or city interrupts (9.4.3).
// A hex counts as touched only if it traced back when it was entered too (the two-point test).
// ----------------------------------------------
#pragma once
#include "TrcFacts.h"
#include "TrcZoc.h"

namespace Trc {

  class TrcRail {
  public:
    TrcRail(const TrcFacts&, const TrcZoc&);
    static std::span<const std::string_view> claims();

    // After a move: the rail hexes on its path that trace back now are remembered for the end phase.
    Position noteTouched(const Ctx&, const HexEngine::MoveUnit&) const;
    // The side's end phase: railheads, then cities.
    Position convert(const Ctx&, SideId, HexSearch::SearchScratch&, HexEngine::EventSink&) const;
    // The rail links from `hex` back to a friendly city or edge, shortest first; nullopt if none.
    std::optional<std::vector<std::size_t>> traceBack(const Ctx&, HexSearch::SearchScratch&, HexIndex, SideId) const;

  private:
    bool openP(const Ctx&, HexIndex, SideId) const;
    Position own(const Ctx&, const std::vector<std::size_t>& links, SideId, HexEngine::EventSink&) const;
    std::vector<std::size_t> linksOf(const std::vector<HexSearch::NodeIndex>& path) const;
    const TrcFacts& facts_;
    const TrcZoc& zoc_;
  };

}  // namespace Trc
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------

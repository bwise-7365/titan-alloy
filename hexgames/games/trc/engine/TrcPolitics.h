// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
// TRC politics (24.0, 19.x, 20.8): the minor allies' surrender triggers, what a surrender does,
// the partisan cycle, and the Warsaw garrison summons.
// ----------------------------------------------
#pragma once
#include "TrcFacts.h"
#include "TrcZoc.h"

namespace Trc {

  class TrcPolitics {
  public:
    TrcPolitics(const TrcFacts&, const TrcZoc&);
    static std::span<const std::string_view> claims();

    // The Russian end phase: Hungary (five Russian units inside), Finland and Rumania (capital lost).
    Position russianEndPhase(const Ctx&, HexEngine::EventSink&) const;
    // The start of September/October 1943: Italy.
    Position italy(const Ctx&, HexEngine::EventSink&) const;
    // The start of each Axis turn from September/October 1944: Finland unless Leningrad is Axis.
    Position finland1944(const Ctx&, HexEngine::EventSink&) const;
    // Every unit of the nation, on the map and off, to the Axis Surrendered Units box.
    Position surrender(const Ctx&, Nation, HexEngine::EventSink&) const;

    // Partisans in an Axis zone or within five hexes of an SS unit, at the end of an Axis movement phase.
    Position removeExposedPartisans(const Ctx&, HexEngine::EventSink&) const;
    // All partisans, at the end of the Russian second impulse, ready to be placed again.
    Position liftPartisans(const Ctx&, HexEngine::EventSink&) const;

    // 20.8: a Russian unit within two hexes of Warsaw from 1944 summons two named corps.
    Position noteWarsaw(const Ctx&, const HexEngine::MoveUnit&) const;
    Position garrisonWarsaw(const Ctx&, HexEngine::EventSink&) const;

  private:
    const TrcFacts& facts_;
    const TrcZoc& zoc_;
  };

}  // namespace Trc
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------

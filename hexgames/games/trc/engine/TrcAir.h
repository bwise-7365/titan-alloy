// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
// TRC air power (15.0): Stukas and Sturmoviks are declared on an attack as the rules document's
// stuka-shift and sturmovik-shift modifiers, in the owner's first-impulse combat phase only, as many
// per impulse as the Airpower Availability panel allows for the year and weather (none in snow),
// one Stuka per battle and no more Stukas than German Army Group HQs on the map, and only when
// every defender is within eight hexes of one friendly HQ (Stavka, or Stalin once Stavka is gone).
// ----------------------------------------------
#pragma once
#include "TrcFacts.h"
#include "TrcWeather.h"

namespace Trc {

  class TrcAir {
  public:
    TrcAir(const TrcFacts&, const TrcWeather&);
    static std::span<const std::string_view> claims();

    // Throws std::invalid_argument, naming the rule, when the declared modifiers break 15.x.
    void check(const Ctx&, const HexEngine::DeclareAttack&) const;
    // Air units a side may commit in one impulse (the sheet's panel; 1945 from the digest's 2.8).
    static int available(bool axisP, int year, Weather);
    static int used(const Position&, SideId);

  private:
    bool inRangeP(const Ctx&, SideId, HexIndex target) const;
    const TrcFacts& facts_;
    const TrcWeather& weather_;
  };

}  // namespace Trc
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------

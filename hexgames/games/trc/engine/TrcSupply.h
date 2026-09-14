// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
// TRC supply (17.0). General supply: a line of at most eight hexes (four in snow) to a friendly
// city, or to a friendly rail hex joined by friendly rail to a friendly city or the owner's board
// edge; never through an enemy zone of control or an enemy city, into but not through a partisan;
// paratroops, partisans and this turn's sea invaders are exempt. Combat supply: Russian and
// Finnish units always; other Axis units in Russia on the snow turns of the first two winters only
// in or next to an Axis city (one hex further, through a hex free of Russian zones, in 1942-43).
// ----------------------------------------------
#pragma once
#include "TrcFacts.h"
#include "TrcWeather.h"
#include "TrcZoc.h"

namespace Trc {

  class TrcSupply : public HexEngine::SupplyTrace {
  public:
    TrcSupply(const TrcFacts&, const TrcZoc&, const TrcWeather&);
    std::span<const std::string_view> claims() const override;
    HexEngine::SupplyReport trace(const Ctx&, HexSearch::SearchScratch&, SideId) const override;

    bool combatSuppliedP(const Ctx&, UnitId) const;

  private:
    std::vector<bool> sources(const Ctx&, HexSearch::SearchScratch&, SideId) const;
    bool exemptP(const Ctx&, UnitId) const;
    bool tracesP(const Ctx&, HexSearch::SearchScratch&, UnitId, const std::vector<bool>& sources) const;
    bool axisCityNearP(const Ctx&, HexIndex) const;

    const TrcFacts& facts_;
    const TrcZoc& zoc_;
    const TrcWeather& weather_;
  };

}  // namespace Trc
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------

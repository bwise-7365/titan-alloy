// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
// TRC zones of control (7.0): own hex plus the six adjacent hexes, never across a blocked hexside,
// the Kerch Strait or into water; partisans (and battle groups) only into their own hex. What a
// partisan's zone does is a subset of the ordinary effects (7.3, 19.2): it stops movement, blocks
// rail movement and retreat and inhibits conversion, but gives no control and forces no attack.
// ----------------------------------------------
#pragma once
#include "TrcFacts.h"

namespace Trc {

  class TrcZoc : public HexEngine::ZocPolicy {
  public:
    explicit TrcZoc(const TrcFacts&);
    std::span<const std::string_view> claims() const override;
    bool projectsIntoP(const Ctx&, UnitId, HexIndex from, HexIndex to) const override;
    bool blockedForP(const Ctx&, HexIndex, SideId mover, HexRules::Purpose) const override;

    // Whether a unit of `side`'s enemy projects a zone into `hex`; partisans count only when asked.
    bool enemyZocP(const Ctx&, HexIndex, SideId side, bool partisansCountP) const;

  private:
    bool adjacentUnblockedP(const Ctx&, HexIndex from, HexIndex to) const;
    const TrcFacts& facts_;
  };

}  // namespace Trc
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------

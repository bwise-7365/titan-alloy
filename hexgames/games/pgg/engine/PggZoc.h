// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
// Zones of control (8.0): the six hexes round every unit, Untried ones included (12.3), except a
// Disrupted unit's (8.11), and never across a Lake hexside (8.17). A friendly unit in the hex negates
// an enemy zone for supply, leadership radius and retreat, never for movement (8.15). Every zone is
// mutual (8.16) because each side's units project the same way.
// ----------------------------------------------
#pragma once
#include "PggFacts.h"

namespace Pgg {

  class PggZoc : public HexEngine::ZocPolicy {
  public:
    explicit PggZoc(const PggFacts&);
    std::span<const std::string_view> claims() const override;
    bool projectsIntoP(const Ctx&, UnitId, HexIndex from, HexIndex to) const override;
    // Movement, Network, Zoc, Control, Grouping: any enemy zone. Supply, Retreat: an enemy zone the
    // hex holds no friendly unit to negate.
    bool blockedForP(const Ctx&, HexIndex, SideId mover, HexRules::Purpose) const override;
    bool enemyZocP(const Ctx&, HexIndex, SideId side) const;

  private:
    const PggFacts& facts_;
  };

}  // namespace Pgg
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------

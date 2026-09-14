// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
// Overrun (6.5), the verb "overrun units target": in a friendly movement phase, units standing
// together next to an enemy-occupied hex spend three movement points each and attack it at once at
// half strength; the result goes on the resolution stack as a PggBattle, which reveals, Disrupts,
// halts or frees the overrunning units. Not by Leaders alone (6.53), not by Disrupted or halted units,
// not by a Soviet unit that began the phase beyond every Leader's radius (6.55), not into the
// westernmost columns by a Soviet unit on turns 1-6 (6.4). An overrun is movement, not combat: no one
// is marked as having attacked or been attacked (9.14).
// ----------------------------------------------
#pragma once
#include "PggCombat.h"
#include "PggFacts.h"
#include "PggMovement.h"

#include "hexengine/Steps.h"

namespace Pgg {

  class PggOverrun {
  public:
    PggOverrun(const PggFacts&, const PggMovement&, const PggCombat&);
    static std::span<const std::string_view> claims();
    Position overrun(const HexEngine::CommandCall&) const;

    static constexpr int kCostHalves = 6;  // three movement points

  private:
    void check(const Ctx&, const std::vector<UnitId>&, HexIndex target) const;
    const PggFacts& facts_;
    const PggMovement& movement_;
    const PggCombat& combat_;
  };

}  // namespace Pgg
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------

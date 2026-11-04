// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
// What a PGG agent may know of a unit's strength. An Untried Soviet division is hidden from both players
// (12.1), so it counts at the mean of the counters of its arm that are still untried, wherever they are.
// The engine's Units::attackOf and defenceOf give true values whatever a counter's revealed flag says,
// and a counter's id spells its printed values, so an agent reads strengths only from here. A German
// counter's strength includes divisional integration (7.3).
// ----------------------------------------------
#pragma once
#include "PggPolicySet.h"

#include "hexai/Tactics.h"

#include <vector>

namespace Pgg {

  class PggSeen {
  public:
    PggSeen(const PggPolicySet&, const Ctx&);
    double attack(UnitId) const;
    double defence(UnitId) const;
    std::vector<HexAi::Piece> pieces(SideId) const;  // the side's units on the map, strength as attack

  private:
    struct Means {
      double attack = 0.0;
      double defence = 0.0;
    };
    Means meansOf(Arm) const;
    double doubled(UnitId) const;

    const PggPolicySet& set_;
    const Ctx ctx_;
    Means rifle_;
    Means armour_;
  };

}  // namespace Pgg
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------

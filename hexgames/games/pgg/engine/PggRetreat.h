// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
// Where a retreating unit may step (9.71, 9.73): not across a Lake hexside, not into an enemy unit,
// not into an enemy zone of control unless a friendly unit stands there, and on turns 1-6 not into
// the westernmost two hex-columns for a Soviet unit (6.4). A vacant hex must be taken if there is one;
// only when none is may the unit step onto a friendly stack.
// ----------------------------------------------
#pragma once
#include "PggFacts.h"
#include "PggZoc.h"

#include <vector>

namespace Pgg {

  class PggRetreat : public HexEngine::RetreatPolicy {
  public:
    PggRetreat(const PggFacts&, const PggZoc&);
    std::span<const std::string_view> claims() const override;
    std::vector<HexIndex> candidates(const Ctx&, UnitId, HexIndex origin) const override;
    HexEngine::RetreatFate fate(const Ctx&, UnitId) const override;  // always Walk

  private:
    const PggFacts& facts_;
    const PggZoc& zoc_;
  };

}  // namespace Pgg
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------

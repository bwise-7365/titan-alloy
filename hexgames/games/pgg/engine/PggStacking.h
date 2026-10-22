// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
// Stacking (7.1, 7.2): three units a hex, four for the Soviet Player when one of them is a Leader;
// markers never count. The excess is the units most recently placed on the hex, Leaders last, which
// stands in for the owning player's choice (see the task file).
// ----------------------------------------------
#pragma once
#include "PggFacts.h"

#include <vector>

namespace Pgg {

  class PggStacking : public HexEngine::StackingPolicy {
  public:
    explicit PggStacking(const PggFacts&);
    std::span<const std::string_view> claims() const override;
    std::vector<UnitId> excess(const Ctx&, HexIndex) const override;
    bool legalP(const std::vector<UnitId>&) const;

  private:
    const PggFacts& facts_;
  };

}  // namespace Pgg
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------

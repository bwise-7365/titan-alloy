// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
// TRC stacking (6.1, 6.3): two armies, or three corps, and a mixed stack of corps and armies at
// most two. HQs, Stavka, leaders, workers, partisans and the 2-7 SS Reserve count for nothing.
// ----------------------------------------------
#pragma once
#include "TrcFacts.h"

namespace Trc {

  class TrcStacking : public HexEngine::StackingPolicy {
  public:
    explicit TrcStacking(const TrcFacts&);
    std::span<const std::string_view> claims() const override;
    // The newest units first until what is left is legal.
    std::vector<UnitId> excess(const Ctx&, HexIndex) const override;

    bool legalP(const std::vector<UnitId>& counted) const;
    std::vector<UnitId> counted(const Ctx&, HexIndex) const;

  private:
    const TrcFacts& facts_;
  };

}  // namespace Trc
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------

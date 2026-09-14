// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
#include "PggStacking.h"

#include <algorithm>
#include <array>

namespace Pgg {

  namespace {

    constexpr std::array<std::string_view, 1> kClaims{"stacking-soviet-leader-bonus"};

  }  // namespace

  PggStacking::PggStacking(const PggFacts& facts) : facts_(facts)
  {
  }

  std::span<const std::string_view>
  PggStacking::claims() const
  {
    return kClaims;
  }

  bool
  PggStacking::legalP(const std::vector<UnitId>& units) const
  {
    const bool leaderP = std::any_of(units.begin(), units.end(), [this](UnitId unit) { return facts_.leaderP(unit); });
    return (leaderP ? 4u : 3u) >= units.size();
  }

  std::vector<UnitId>
  PggStacking::excess(const Ctx& ctx, HexIndex hex) const
  {
    std::vector<UnitId> kept;
    for (UnitId unit : ctx.position.unitsAt(hex)) {
      if (!facts_.markerP(unit)) {
        kept.push_back(unit);
      }
    }
    std::vector<UnitId> over;
    while (!legalP(kept)) {
      auto newest = std::find_if(kept.rbegin(), kept.rend(), [this](UnitId unit) { return !facts_.leaderP(unit); });
      if (kept.rend() == newest) {
        newest = kept.rbegin();
      }
      over.push_back(*newest);
      kept.erase(std::next(newest).base());
    }
    return over;
  }

}  // namespace Pgg
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------

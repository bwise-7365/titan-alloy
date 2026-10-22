// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
#include "TrcStacking.h"

#include <array>

namespace Trc {

  namespace {

    constexpr std::array<std::string_view, 2> kClaims{"stacking-sizes", "stacking-no-value"};

  }  // namespace

  TrcStacking::TrcStacking(const TrcFacts& facts) : facts_(facts)
  {
  }

  std::span<const std::string_view>
  TrcStacking::claims() const
  {
    return kClaims;
  }

  std::vector<UnitId>
  TrcStacking::counted(const Ctx& ctx, HexIndex hex) const
  {
    std::vector<UnitId> out;
    for (UnitId unit : ctx.position.unitsAt(hex)) {
      if (!facts_.noStackingValueP(unit)) {
        out.push_back(unit);
      }
    }
    return out;
  }

  bool
  TrcStacking::legalP(const std::vector<UnitId>& units) const
  {
    int armies = 0;
    int corps = 0;
    for (UnitId unit : units) {
      switch (facts_.echelon(unit)) {
        case Echelon::Army:
        case Echelon::ArmyGroup:
          ++armies;
          break;
        case Echelon::Corps:
        case Echelon::None:
          ++corps;
          break;
      }
    }
    if (0 == armies) {
      return 3 >= corps;
    }
    return 2 >= armies + corps;
  }

  std::vector<UnitId>
  TrcStacking::excess(const Ctx& ctx, HexIndex hex) const
  {
    std::vector<UnitId> kept = counted(ctx, hex);
    std::vector<UnitId> over;
    while (!legalP(kept)) {
      over.push_back(kept.back());
      kept.pop_back();
    }
    return over;
  }

}  // namespace Trc
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------

// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
#include "PggRetreat.h"

#include "PggUnits.h"

#include <array>

namespace Pgg {

  namespace {

    constexpr std::array<std::string_view, 1> kClaims{"forbidden-hexrow-retreat"};

  }  // namespace

  PggRetreat::PggRetreat(const PggFacts& facts, const PggZoc& zoc) : facts_(facts), zoc_(zoc)
  {
  }

  std::span<const std::string_view>
  PggRetreat::claims() const
  {
    return kClaims;
  }

  std::vector<HexIndex>
  PggRetreat::candidates(const Ctx& ctx, UnitId unit, HexIndex origin) const
  {
    const SideId side = ctx.roster.unit(unit).side;
    const bool forbiddenColumnsP = facts_.soviet() == side && 6 >= ctx.position.clock().turn;
    std::vector<HexIndex> vacant;
    std::vector<HexIndex> friendly;
    for (int d = 0; d < HexCoord::kDirections; ++d) {
      const Direction direction = static_cast<Direction>(d);
      const std::optional<HexIndex> to = ctx.board.neighbour(origin, direction);
      if (!to || facts_.lakeHexsideP(origin, direction) || Units::enemyAtP(ctx, *to, side) ||
          zoc_.blockedForP(ctx, *to, side, HexRules::Purpose::Retreat)) {
        continue;
      }
      if (forbiddenColumnsP && facts_.westmostColumnsP(*to)) {
        continue;  // 6.4: forced there, the unit is eliminated instead
      }
      (Units::friendlyAtP(ctx, *to, side) ? friendly : vacant).push_back(*to);
    }
    return vacant.empty() ? friendly : vacant;  // 9.73
  }

  HexEngine::RetreatFate
  PggRetreat::fate(const Ctx&, UnitId) const
  {
    return HexEngine::RetreatFate::Walk;
  }

}  // namespace Pgg
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------

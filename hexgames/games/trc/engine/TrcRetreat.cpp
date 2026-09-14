// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
#include "TrcRetreat.h"

#include <array>

namespace Trc {

  namespace {

    constexpr std::array<std::string_view, 2> kClaims{"retreat-woods", "retreat-leaders-workers"};

  }  // namespace

  TrcRetreat::TrcRetreat(const TrcFacts& facts, const TrcZoc& zoc) : facts_(facts), zoc_(zoc)
  {
  }

  std::span<const std::string_view>
  TrcRetreat::claims() const
  {
    return kClaims;
  }

  std::vector<HexIndex>
  TrcRetreat::candidates(const Ctx& ctx, UnitId unit, HexIndex origin) const
  {
    const SideId side = ctx.roster.unit(unit).side;
    std::vector<HexIndex> out;
    for (int d = 0; d < HexCoord::kDirections; ++d) {
      const Direction direction = static_cast<Direction>(d);
      const std::optional<HexIndex> to = ctx.board.neighbour(origin, direction);
      if (!to || facts_.waterP(*to) || facts_.blockedHexsideP(origin, direction)) {
        continue;
      }
      bool enemyHeldP = false;
      for (UnitId occupant : ctx.position.unitsAt(*to)) {
        enemyHeldP = enemyHeldP || side != ctx.roster.unit(occupant).side;
      }
      if (enemyHeldP || zoc_.blockedForP(ctx, *to, side, HexRules::Purpose::Retreat)) {
        continue;
      }
      out.push_back(*to);
    }
    return out;
  }

  HexEngine::RetreatFate
  TrcRetreat::fate(const Ctx& ctx, UnitId unit) const
  {
    if (facts_.typeP(unit, "leader") || facts_.typeP(unit, "worker")) {
      return HexEngine::RetreatFate::Surrender;
    }
    const HexModel::UnitState& state = ctx.position.unit(unit);
    const HexIndex hex = std::get<HexIndex>(*state.where);
    if ("woods" == ctx.rules.hexTerrain()[ctx.board.terrain(hex).value].id) {
      return HexEngine::RetreatFate::Stay;
    }
    return HexEngine::RetreatFate::Walk;
  }

}  // namespace Trc
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------

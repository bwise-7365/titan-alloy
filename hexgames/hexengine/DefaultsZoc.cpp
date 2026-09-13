// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
// SixHexZoc: the first <zoc> of the rules document, read as a projection rule.
// ----------------------------------------------
#include "hexengine/Defaults.h"

#include "hexsearch/Search.h"

#include <algorithm>
#include <stdexcept>

namespace HexEngine {

  namespace {

    const HexRules::ZocSpec&
    firstZoc(const HexRules::RuleSet& rules)
    {
      if (rules.zocs().empty()) {
        throw std::invalid_argument("SixHexZoc: the rules document '" + rules.gameId() +
                                     "' declares no <zoc>");
      }
      return rules.zocs().front();
    }

    int
    rangeOf(const HexRules::ZocSpec& spec)
    {
      if (const HexCount* hexes = std::get_if<HexCount>(&spec.range)) {
        return hexes->value;
      }
      throw std::invalid_argument("SixHexZoc: zoc '" + spec.id +
                                   "' has a range that is not a hex count; a game with an "
                                   "unlimited or authored zone of control supplies its own policy");
    }

  }  // namespace

  SixHexZoc::SixHexZoc(const HexRules::RuleSet& rules)
    : spec_(firstZoc(rules)), range_(rangeOf(firstZoc(rules)))
  {
  }

  std::span<const std::string_view>
  SixHexZoc::claims() const
  {
    return {};
  }

  bool
  SixHexZoc::projectsIntoP(const Ctx& ctx, UnitId unit, HexIndex from, HexIndex to) const
  {
    const UnitSpec& spec = ctx.roster.unit(unit);
    if (!spec_.projectedBy.empty()) {
      if (spec_.projectedBy.end() ==
          std::find(spec_.projectedBy.begin(), spec_.projectedBy.end(), spec.type)) {
        return false;
      }
    }
    const HexRules::UnitType& type = ctx.rules.unitTypes()[spec.type.value];
    const std::string zoc = type.zoc.value_or("full");
    if ("none" == zoc) {
      return false;
    }
    if (from == to) {
      return true;
    }
    if ("own-hex" == zoc) {
      return false;
    }
    return reachesP(ctx, from, to);
  }

  bool
  SixHexZoc::blockedEdgeP(const Ctx& ctx, HexIndex hex, Direction direction) const
  {
    for (EdgeTerrainId edge : ctx.board.edge(hex, direction)) {
      if (spec_.blockedBy.end() != std::find(spec_.blockedBy.begin(), spec_.blockedBy.end(), edge)) {
        return true;
      }
    }
    return false;
  }

  // Blocked-by hexsides stop a projection rather than shortening it, so the reach is a flood that
  // refuses those hexsides. Range one -- every game in view -- needs no flood at all.
  bool
  SixHexZoc::reachesP(const Ctx& ctx, HexIndex from, HexIndex to) const
  {
    if (from == to) {
      return true;
    }
    if (1 == range_) {
      for (int d = 0; d < HexCoord::kDirections; ++d) {
        const Direction direction = static_cast<Direction>(d);
        const std::optional<HexIndex> neighbour = ctx.board.neighbour(from, direction);
        if (neighbour && *neighbour == to) {
          return !blockedEdgeP(ctx, from, direction);
        }
      }
      return false;
    }
    const std::vector<HexIndex> reach =
        HexSearch::regionFlood(ctx.board, from, [&](HexIndex hex, Direction direction) {
          return range_ <= ctx.board.distance(from, hex) || blockedEdgeP(ctx, hex, direction);
        });
    return reach.end() != std::find(reach.begin(), reach.end(), to);
  }

  bool
  SixHexZoc::blockedForP(const Ctx& ctx, HexIndex hex, SideId mover, Purpose purpose) const
  {
    const bool appliesP = Purpose::Movement == purpose
                              ? spec_.stopsMovementP
                              : spec_.blocks.test(static_cast<std::size_t>(purpose));
    if (!appliesP) {
      return false;
    }
    if (spec_.negatedByFriendlyP) {
      for (UnitId occupant : ctx.position.unitsAt(hex)) {
        if (mover == ctx.roster.unit(occupant).side) {
          return false;
        }
      }
    }

    const SideMask enemies = enemiesOf(ctx.rules, mover);
    const auto anyProjectorP = [&](HexIndex from) {
      for (UnitId occupant : ctx.position.unitsAt(from)) {
        const UnitSpec& spec = ctx.roster.unit(occupant);
        if (enemies.test(spec.side.value) && projectsIntoP(ctx, occupant, from, hex)) {
          return true;
        }
      }
      return false;
    };

    if (anyProjectorP(hex)) {
      return true;
    }
    if (1 == range_) {
      for (int d = 0; d < HexCoord::kDirections; ++d) {
        const Direction direction = static_cast<Direction>(d);
        if (blockedEdgeP(ctx, hex, direction)) {
          continue;
        }
        const std::optional<HexIndex> neighbour = ctx.board.neighbour(hex, direction);
        if (neighbour && anyProjectorP(*neighbour)) {
          return true;
        }
      }
      return false;
    }
    const std::vector<HexIndex> around =
        HexSearch::regionFlood(ctx.board, hex, [&](HexIndex from, Direction direction) {
          return range_ <= ctx.board.distance(hex, from) || blockedEdgeP(ctx, from, direction);
        });
    for (HexIndex near : around) {
      if (anyProjectorP(near)) {
        return true;
      }
    }
    return false;
  }

}  // namespace HexEngine
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------

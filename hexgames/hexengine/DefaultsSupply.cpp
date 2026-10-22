// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
// BoundedTrace: the rules document's first <trace>, segment by segment.
// ----------------------------------------------
#include "hexengine/Defaults.h"

#include "hexsearch/Search.h"

#include <algorithm>
#include <functional>
#include <optional>
#include <span>
#include <stdexcept>

namespace HexEngine {

  namespace {

    int
    lengthOf(const HexModel::Extent& extent, int unlimited)
    {
      if (const HexCount* hexes = std::get_if<HexCount>(&extent)) {
        return hexes->value;
      }
      return unlimited;  // "unlimited" and "authored" both walk as far as the board allows
    }

  }  // namespace

  BoundedTrace::BoundedTrace(const HexRules::RuleSet& rules, const ZocPolicy& zoc)
    : rules_(rules), zoc_(zoc)
  {
    if (rules_.traces().empty()) {
      throw std::invalid_argument("BoundedTrace: the rules document '" + rules_.gameId() +
                                   "' declares no <trace>");
    }
  }

  std::span<const std::string_view>
  BoundedTrace::claims() const
  {
    return {};
  }

  SupplyReport
  BoundedTrace::trace(const Ctx& ctx, HexSearch::SearchScratch& scratch, SideId side) const
  {
    const HexRules::Trace& spec = rules_.traces().front();
    const SideMask enemies = enemiesOf(ctx.rules, side);

    // A source, as the engine reads "a friendly-controlled city": a hex this side controls that
    // carries a drawn feature. The rules document holds the real set as prose, so a game whose
    // sources are something else supplies its own SupplyTrace.
    std::vector<bool> sourceP(ctx.board.hexCount(), false);
    for (std::size_t h = 0; h < ctx.board.hexCount(); ++h) {
      const HexIndex hex{static_cast<std::uint32_t>(h)};
      const std::optional<SideId> owner = ctx.position.control(hex);
      sourceP[h] = owner.has_value() && *owner == side && !ctx.board.features(hex).empty();
    }

    // A hex a trace may not pass through: an enemy zone of control where the trace says so, or an
    // enemy-controlled hex carrying a drawn feature (an enemy city athwart the road).
    const auto passableP = [&](HexIndex hex) {
      const std::optional<SideId> owner = ctx.position.control(hex);
      if (owner && enemies.test(owner->value) && !ctx.board.features(hex).empty()) {
        return false;
      }
      if (spec.blockedByZocP && zoc_.blockedForP(ctx, hex, side, Purpose::Supply)) {
        return false;
      }
      return true;
    };

    const HexSearch::HexAdjacencyGraph hexes(ctx.board, [](HexIndex, Direction) { return true; });
    const std::function<bool(HexSearch::NodeIndex, std::size_t)> walkable =
        [&](HexSearch::NodeIndex from, std::size_t arcIndex) {
          const std::optional<HexIndex> to =
              ctx.board.neighbour(HexIndex{from}, static_cast<Direction>(arcIndex));
          return to.has_value() && passableP(*to);
        };

    SupplyReport report;
    for (UnitId unit : ctx.roster.ofSide(side)) {
      const HexModel::UnitState& state = ctx.position.unit(unit);
      if (!state.where || !std::holds_alternative<HexIndex>(*state.where)) {
        continue;  // a unit off the map traces nothing
      }
      const HexIndex from = std::get<HexIndex>(*state.where);

      int found = 0;
      std::vector<HexSearch::NodeIndex> onNetwork;

      for (const HexRules::Segment& segment : spec.segments) {
        if ("free" == segment.kind) {
          const HexSearch::NodeIndex start = from.value;
          const HexSearch::Field<HexSearch::CostHalves> field = HexSearch::Algorithms::bfsFlood(
              hexes, scratch, std::span<const HexSearch::NodeIndex>(&start, 1), walkable,
              lengthOf(segment.length, static_cast<int>(ctx.board.hexCount())));
          for (HexSearch::NodeIndex reached : field.reached()) {
            if (sourceP[reached]) {
              ++found;
            }
            onNetwork.push_back(reached);
          }
        } else if ("network" == segment.kind && segment.network) {
          const HexSearch::NetworkGraph network(ctx.board, *segment.network, [&](std::size_t link) {
            const std::optional<SideId> owner = ctx.position.linkOwner(*segment.network, link);
            return !owner.has_value() || !enemies.test(owner->value);
          });
          std::vector<HexSearch::NodeIndex> seeds = onNetwork;
          if (seeds.empty()) {
            seeds.push_back(from.value);
          }
          // The graph's own link predicate has already refused an enemy-owned link, so every arc it
          // offers is allowed; arcIndex here is a link, not a direction.
          const std::function<bool(HexSearch::NodeIndex, std::size_t)> anyLink =
              [](HexSearch::NodeIndex, std::size_t) { return true; };
          const HexSearch::Field<HexSearch::CostHalves> field = HexSearch::Algorithms::bfsFlood(
              network, scratch, std::span<const HexSearch::NodeIndex>(seeds), anyLink,
              lengthOf(segment.length, static_cast<int>(ctx.board.hexCount())));
          for (HexSearch::NodeIndex reached : field.reached()) {
            if (sourceP[reached]) {
              ++found;
            }
          }
        }
        // A "region" segment gates the trace on region occupancy, which no game in this milestone
        // supplies; it passes through untouched rather than refusing the trace.
        if (spec.threshold <= found) {
          break;
        }
      }

      if (spec.threshold <= found) {
        report.supplied.push_back(unit);
      } else {
        report.unsupplied.push_back(unit);
      }
    }
    return report;
  }

}  // namespace HexEngine
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------

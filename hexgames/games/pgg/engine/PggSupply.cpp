// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
#include "PggSupply.h"

#include "PggState.h"
#include "PggTerrain.h"
#include "PggUnits.h"

#include <algorithm>
#include <array>

namespace Pgg {

  namespace {

    constexpr std::array<std::string_view, 3> kClaims{"supply-never-fatal", "supply-first-turn",
                                                      "soviet-interdiction-effect"};

    constexpr int kGermanHexes = 20;   // 11.11
    constexpr int kGermanHalves = 40;  // 11.12: twenty movement points

    using HexSearch::NodeIndex;

  }  // namespace

  PggSupply::PggSupply(const PggFacts& facts, const PggZoc& zoc) : facts_(facts), zoc_(zoc)
  {
  }

  std::span<const std::string_view>
  PggSupply::claims() const
  {
    return kClaims;
  }

  bool
  PggSupply::passableP(const Ctx& ctx, HexIndex hex, SideId side) const
  {
    if (facts_.lakeP(hex) || Units::enemyAtP(ctx, hex, side) ||
        zoc_.blockedForP(ctx, hex, side, HexRules::Purpose::Supply)) {
      return false;
    }
    return facts_.german() != side || hex != stateOf(ctx.position).sovietInterdiction;  // 13.44
  }

  bool
  PggSupply::throughP(const Ctx& ctx, HexIndex hex, SideId side) const
  {
    return passableP(ctx, hex, side) && !facts_.swampP(hex);  // 6.7: into a swamp, not through it
  }

  std::vector<bool>
  PggSupply::roadSupplied(const Ctx& ctx, HexSearch::SearchScratch& scratch) const
  {
    const SideId german = facts_.german();
    std::vector<bool> out(ctx.board.hexCount(), false);
    const HexIndex source = facts_.supplyRoadHex();
    if (!passableP(ctx, source, german)) {
      return out;
    }
    const HexModel::LinkNetwork& roads = ctx.board.network(facts_.roadNetwork());
    const HexSearch::NetworkGraph graph(ctx.board, facts_.roadNetwork(), [](std::size_t) { return true; });
    const std::function<bool(NodeIndex, std::size_t)> openP = [&](NodeIndex node, std::size_t link) {
      const HexModel::LinkNetwork::Link& arc = roads.links()[link];
      const HexIndex to = arc.a.value == node ? arc.b : arc.a;
      return throughP(ctx, HexIndex{node}, german) && passableP(ctx, to, german);
    };
    const NodeIndex seed = source.value;
    const HexSearch::Field<HexSearch::CostHalves> field = HexSearch::Algorithms::bfsFlood(
        graph, scratch, std::span<const NodeIndex>(&seed, 1), openP, static_cast<int>(ctx.board.hexCount()));
    for (NodeIndex node : field.reached()) {
      out[node] = true;
    }
    return out;
  }

  bool
  PggSupply::germanSuppliedP(const Ctx& ctx, HexSearch::SearchScratch& scratch, UnitId unit,
                             const std::vector<bool>& roads) const
  {
    const SideId german = facts_.german();
    const HexIndex start = *Units::hexOf(ctx.position, unit);
    const auto stepP = [&](NodeIndex node, std::size_t direction) {
      const std::optional<HexIndex> to = ctx.board.neighbour(HexIndex{node}, static_cast<Direction>(direction));
      return to && !facts_.lakeHexsideP(HexIndex{node}, static_cast<Direction>(direction)) &&
             (node == start.value || throughP(ctx, HexIndex{node}, german)) && passableP(ctx, *to, german);
    };
    const HexSearch::HexAdjacencyGraph graph(ctx.board, [](HexIndex, Direction) { return true; });
    const NodeIndex seed = start.value;
    {
      const HexSearch::Field<HexSearch::CostHalves> near = HexSearch::Algorithms::bfsFlood(
          graph, scratch, std::span<const NodeIndex>(&seed, 1), stepP, kGermanHexes);
      for (NodeIndex node : near.reached()) {
        if (roads[node]) {
          return true;  // 11.11
        }
      }
    }
    const MoveClass mover = facts_.moveClass(unit);
    const std::function<std::optional<HexSearch::CostHalves>(NodeIndex, std::size_t)> cost =
        [&](NodeIndex node, std::size_t direction) -> std::optional<HexSearch::CostHalves> {
      if (!stepP(node, direction)) {
        return std::nullopt;
      }
      const std::optional<int> halves =
          terrainHalves(facts_, mover, german, HexIndex{node}, static_cast<Direction>(direction));
      return halves ? std::optional<HexSearch::CostHalves>(HexSearch::CostHalves{*halves}) : std::nullopt;
    };
    const HexSearch::Source source{seed, HexSearch::CostHalves::zero()};
    const HexSearch::Field<HexSearch::CostHalves> direct = HexSearch::Algorithms::dijkstraBounded(
        graph, scratch, std::span<const HexSearch::Source>(&source, 1), cost, HexSearch::CostHalves{kGermanHalves},
        [](NodeIndex) { return false; });
    for (NodeIndex node : direct.reached()) {
      if (facts_.westEdgeP(HexIndex{node})) {
        return true;  // 11.12
      }
    }
    return false;
  }

  bool
  PggSupply::leaderSuppliedP(const Ctx& ctx, HexSearch::SearchScratch& scratch, UnitId leader) const
  {
    const SideId soviet = facts_.soviet();
    const HexIndex start = *Units::hexOf(ctx.position, leader);
    const auto stepP = [&](NodeIndex node, std::size_t direction) {
      const std::optional<HexIndex> to = ctx.board.neighbour(HexIndex{node}, static_cast<Direction>(direction));
      return to && !facts_.lakeHexsideP(HexIndex{node}, static_cast<Direction>(direction)) &&
             (node == start.value || throughP(ctx, HexIndex{node}, soviet)) && passableP(ctx, *to, soviet);
    };
    const HexSearch::HexAdjacencyGraph graph(ctx.board, [](HexIndex, Direction) { return true; });
    const NodeIndex seed = start.value;
    const HexSearch::Field<HexSearch::CostHalves> field = HexSearch::Algorithms::bfsFlood(
        graph, scratch, std::span<const NodeIndex>(&seed, 1), stepP, static_cast<int>(ctx.board.hexCount()));
    for (NodeIndex node : field.reached()) {
      if (facts_.eastEdgeP(HexIndex{node})) {
        return true;  // 10.34, 11.24
      }
    }
    return false;
  }

  std::vector<UnitId>
  PggSupply::workingLeaders(const Ctx& ctx, HexSearch::SearchScratch& scratch, bool suppliedOnlyP) const
  {
    const SideId soviet = facts_.soviet();
    const PggSideState& mine = stateOf(ctx.position).side(soviet);
    std::vector<UnitId> out;
    for (UnitId unit : Units::onMap(ctx, facts_, soviet)) {
      if (!facts_.leaderP(unit) || mine.disrupted.contains(unit)) {
        continue;  // 6.64: a Disrupted Leader coordinates nothing
      }
      if (!suppliedOnlyP || mine.entered.contains(unit) || leaderSuppliedP(ctx, scratch, unit)) {
        out.push_back(unit);
      }
    }
    return out;
  }

  bool
  PggSupply::communicatesP(const Ctx& ctx, HexSearch::SearchScratch& scratch, UnitId unit,
                           const std::vector<UnitId>& leaders) const
  {
    const SideId soviet = facts_.soviet();
    const HexIndex start = *Units::hexOf(ctx.position, unit);
    int reach = 0;
    for (UnitId leader : leaders) {
      reach = std::max(reach, facts_.leaderRating(leader));
    }
    const auto stepP = [&](NodeIndex node, std::size_t direction) {
      const std::optional<HexIndex> to = ctx.board.neighbour(HexIndex{node}, static_cast<Direction>(direction));
      return to && !facts_.lakeHexsideP(HexIndex{node}, static_cast<Direction>(direction)) &&
             (node == start.value || throughP(ctx, HexIndex{node}, soviet)) && passableP(ctx, *to, soviet);
    };
    const HexSearch::HexAdjacencyGraph graph(ctx.board, [](HexIndex, Direction) { return true; });
    const NodeIndex seed = start.value;
    const HexSearch::Field<HexSearch::CostHalves> field =
        HexSearch::Algorithms::bfsFlood(graph, scratch, std::span<const NodeIndex>(&seed, 1), stepP, reach);
    for (UnitId leader : leaders) {
      const HexIndex at = *Units::hexOf(ctx.position, leader);
      if (field.reachedP(at.value) && field.distance(at.value).halves <= facts_.leaderRating(leader)) {
        return true;  // 11.22
      }
    }
    return false;
  }

  bool
  PggSupply::suppliedP(const Ctx& ctx, HexSearch::SearchScratch& scratch, UnitId unit) const
  {
    const SideId side = ctx.roster.unit(unit).side;
    if (stateOf(ctx.position).side(side).entered.contains(unit)) {
      return true;  // 11.33
    }
    if (facts_.german() == side) {
      const std::vector<bool> roads = roadSupplied(ctx, scratch);
      return germanSuppliedP(ctx, scratch, unit, roads);
    }
    if (facts_.leaderP(unit)) {
      return leaderSuppliedP(ctx, scratch, unit);
    }
    const std::vector<UnitId> leaders = workingLeaders(ctx, scratch, true);
    return communicatesP(ctx, scratch, unit, leaders);
  }

  bool
  PggSupply::withinRadiusP(const Ctx& ctx, HexSearch::SearchScratch& scratch, UnitId unit) const
  {
    const std::vector<UnitId> leaders = workingLeaders(ctx, scratch, false);
    return communicatesP(ctx, scratch, unit, leaders);
  }

  HexEngine::SupplyReport
  PggSupply::trace(const Ctx& ctx, HexSearch::SearchScratch& scratch, SideId side) const
  {
    HexEngine::SupplyReport report;
    const PggSideState& mine = stateOf(ctx.position).side(side);
    const std::vector<UnitId> units = Units::onMap(ctx, facts_, side);
    std::vector<bool> roads;
    std::vector<UnitId> leaders;
    if (facts_.german() == side) {
      roads = roadSupplied(ctx, scratch);
    } else {
      leaders = workingLeaders(ctx, scratch, true);
    }
    for (UnitId unit : units) {
      bool suppliedP = mine.entered.contains(unit);
      if (!suppliedP && facts_.german() == side) {
        suppliedP = germanSuppliedP(ctx, scratch, unit, roads);
      } else if (!suppliedP && facts_.leaderP(unit)) {
        suppliedP = leaderSuppliedP(ctx, scratch, unit);
      } else if (!suppliedP) {
        suppliedP = communicatesP(ctx, scratch, unit, leaders);
      }
      (suppliedP ? report.supplied : report.unsupplied).push_back(unit);
    }
    return report;
  }

  bool
  PggSupply::lineWestP(const Ctx& ctx, HexSearch::SearchScratch& scratch, HexIndex hex) const
  {
    const SideId german = facts_.german();
    const auto openP = [&](HexIndex at) {
      return !facts_.lakeP(at) && !Units::enemyAtP(ctx, at, german) &&
             !zoc_.blockedForP(ctx, at, german, HexRules::Purpose::Supply);
    };
    if (!openP(hex)) {
      return false;
    }
    const auto stepP = [&](NodeIndex node, std::size_t direction) {
      const std::optional<HexIndex> to = ctx.board.neighbour(HexIndex{node}, static_cast<Direction>(direction));
      return to && !facts_.lakeHexsideP(HexIndex{node}, static_cast<Direction>(direction)) && openP(*to);
    };
    const HexSearch::HexAdjacencyGraph graph(ctx.board, [](HexIndex, Direction) { return true; });
    const NodeIndex seed = hex.value;
    const HexSearch::Field<HexSearch::CostHalves> field = HexSearch::Algorithms::bfsFlood(
        graph, scratch, std::span<const NodeIndex>(&seed, 1), stepP, static_cast<int>(ctx.board.hexCount()));
    for (NodeIndex node : field.reached()) {
      if (facts_.westEdgeP(HexIndex{node})) {
        return true;
      }
    }
    return false;
  }

}  // namespace Pgg
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------

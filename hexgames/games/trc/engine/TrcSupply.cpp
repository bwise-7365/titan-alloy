// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
#include "TrcSupply.h"

#include "TrcMarkers.h"
#include "TrcState.h"

#include <array>

namespace Trc {

  namespace {

    constexpr std::array<std::string_view, 2> kClaims{"supply-exempt", "supply-partisan-city"};

    bool
    enemyUnitAtP(const Ctx& ctx, HexIndex hex, SideId side)
    {
      for (UnitId occupant : ctx.position.unitsAt(hex)) {
        if (side != ctx.roster.unit(occupant).side) {
          return true;
        }
      }
      return false;
    }

  }  // namespace

  TrcSupply::TrcSupply(const TrcFacts& facts, const TrcZoc& zoc, const TrcWeather& weather)
    : facts_(facts), zoc_(zoc), weather_(weather)
  {
  }

  std::span<const std::string_view>
  TrcSupply::claims() const
  {
    return kClaims;
  }

  // Friendly cities (a partisan in one changes nothing: control is not the partisan's, 19.2), and
  // every friendly rail hex a chain of friendly rail links joins to one or to the owner's edge.
  std::vector<bool>
  TrcSupply::sources(const Ctx& ctx, HexSearch::SearchScratch& scratch, SideId side) const
  {
    const Board& board = ctx.board;
    const NetworkId rail = facts_.railNetwork();
    std::vector<bool> out(board.hexCount(), false);
    std::vector<HexSearch::NodeIndex> seeds;
    for (std::size_t h = 0; h < board.hexCount(); ++h) {
      const HexIndex hex{static_cast<std::uint32_t>(h)};
      const bool cityP = facts_.cityP(hex) && side == ctx.position.control(hex);
      out[h] = cityP;
      if ((cityP || facts_.ownEdgeP(side, hex)) && facts_.railHexP(hex)) {
        seeds.push_back(hex.value);
      }
    }
    const HexSearch::NetworkGraph friendly(board, rail, [&](std::size_t link) {
      return side == ctx.position.linkOwner(rail, link);
    });
    const std::function<bool(HexSearch::NodeIndex, std::size_t)> openP = [&](HexSearch::NodeIndex, std::size_t link) {
      const LinkNetwork::Link& arc = board.network(rail).links()[link];
      return !enemyUnitAtP(ctx, arc.a, side) && !enemyUnitAtP(ctx, arc.b, side);
    };
    const HexSearch::Field<HexSearch::CostHalves> field = HexSearch::Algorithms::bfsFlood(
        friendly, scratch, std::span<const HexSearch::NodeIndex>(seeds), openP, static_cast<int>(board.hexCount()));
    for (HexSearch::NodeIndex node : field.reached()) {
      bool ownedLinkP = false;
      for (std::size_t link : board.network(rail).linksAt(HexIndex{node})) {
        ownedLinkP = ownedLinkP || side == ctx.position.linkOwner(rail, link);
      }
      out[node] = out[node] || ownedLinkP;
    }
    return out;
  }

  bool
  TrcSupply::exemptP(const Ctx& ctx, UnitId unit) const
  {
    return facts_.typeP(unit, "paratroop") || facts_.typeP(unit, "partisan") ||
           Markers::markedP(ctx.position, unit, Markers::kInvaded);
  }

  bool
  TrcSupply::tracesP(const Ctx& ctx, HexSearch::SearchScratch& scratch, UnitId unit,
                      const std::vector<bool>& sources) const
  {
    const SideId side = ctx.roster.unit(unit).side;
    const HexIndex start = std::get<HexIndex>(*ctx.position.unit(unit).where);
    const auto enemyCityP = [&](HexIndex hex) {
      const std::optional<SideId> owner = ctx.position.control(hex);
      return facts_.cityP(hex) && owner && side != *owner;
    };
    const auto partisanAtP = [&](HexIndex hex) {
      for (UnitId occupant : ctx.position.unitsAt(hex)) {
        if (facts_.typeP(occupant, "partisan")) {
          return true;
        }
      }
      return false;
    };
    // A hex the line may leave again: the start, or one outside enemy zones, enemy cities and partisans.
    const auto throughP = [&](HexIndex hex) {
      return hex == start ||
             (!zoc_.blockedForP(ctx, hex, side, HexRules::Purpose::Supply) && !enemyCityP(hex) && !partisanAtP(hex));
    };
    const HexSearch::HexAdjacencyGraph graph(ctx.board, [&](HexIndex from, Direction direction) {
      const std::optional<HexIndex> to = ctx.board.neighbour(from, direction);
      return to && !facts_.waterP(*to) && !facts_.blockedHexsideP(from, direction) && !enemyCityP(*to);
    });
    const std::function<bool(HexSearch::NodeIndex, std::size_t)> fromP = [&](HexSearch::NodeIndex node, std::size_t) {
      return throughP(HexIndex{node});
    };
    const int range = Weather::Snow == weather_.current(ctx.position) ? 4 : 8;
    const HexSearch::NodeIndex seed = start.value;
    const HexSearch::Field<HexSearch::CostHalves> field = HexSearch::Algorithms::bfsFlood(
        graph, scratch, std::span<const HexSearch::NodeIndex>(&seed, 1), fromP, range);
    for (HexSearch::NodeIndex node : field.reached()) {
      if (sources[node]) {
        return true;
      }
    }
    return false;
  }

  HexEngine::SupplyReport
  TrcSupply::trace(const Ctx& ctx, HexSearch::SearchScratch& scratch, SideId side) const
  {
    const std::vector<bool> found = sources(ctx, scratch, side);
    HexEngine::SupplyReport report;
    for (UnitId unit : ctx.roster.ofSide(side)) {
      const HexModel::UnitState& state = ctx.position.unit(unit);
      if (!state.where || !std::holds_alternative<HexIndex>(*state.where)) {
        continue;
      }
      if (exemptP(ctx, unit) || tracesP(ctx, scratch, unit, found)) {
        report.supplied.push_back(unit);
      } else {
        report.unsupplied.push_back(unit);
      }
    }
    return report;
  }

  bool
  TrcSupply::axisCityNearP(const Ctx& ctx, HexIndex hex) const
  {
    const auto axisCityP = [&](HexIndex at) {
      return facts_.cityP(at) && facts_.axis() == ctx.position.control(at);
    };
    if (axisCityP(hex)) {
      return true;
    }
    for (int d = 0; d < HexCoord::kDirections; ++d) {
      const std::optional<HexIndex> near = ctx.board.neighbour(hex, static_cast<Direction>(d));
      if (near && axisCityP(*near)) {
        return true;
      }
    }
    return false;
  }

  bool
  TrcSupply::combatSuppliedP(const Ctx& ctx, UnitId unit) const
  {
    const Nation nation = facts_.nation(unit);
    if (Nation::Russian == nation || Nation::Finnish == nation) {
      return true;
    }
    const int turn = ctx.position.clock().turn;
    const bool firstWinterP = 3 <= turn && 8 >= turn;
    const bool secondWinterP = 9 <= turn && 14 >= turn;
    if ((!firstWinterP && !secondWinterP) || Weather::Snow != weather_.current(ctx.position)) {
      return true;
    }
    const HexIndex hex = std::get<HexIndex>(*ctx.position.unit(unit).where);
    if (!facts_.inCountryP(hex, "russia") || axisCityNearP(ctx, hex)) {
      return true;
    }
    if (firstWinterP) {
      return false;
    }
    for (int d = 0; d < HexCoord::kDirections; ++d) {
      const std::optional<HexIndex> near = ctx.board.neighbour(hex, static_cast<Direction>(d));
      if (near && !facts_.waterP(*near) && !zoc_.enemyZocP(ctx, *near, facts_.axis(), false) &&
          axisCityNearP(ctx, *near)) {
        return true;  // 17.3.3
      }
    }
    return false;
  }

}  // namespace Trc
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------

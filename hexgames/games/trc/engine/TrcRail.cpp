// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
#include "TrcRail.h"

#include "TrcFlags.h"

#include <array>

namespace Trc {

  namespace {

    constexpr std::array<std::string_view, 4> kClaims{"rail-railhead-advance", "rail-railhead-pushback",
                                                      "rail-prior-turn", "rail-city-conversion"};

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

  TrcRail::TrcRail(const TrcFacts& facts, const TrcZoc& zoc) : facts_(facts), zoc_(zoc)
  {
  }

  std::span<const std::string_view>
  TrcRail::claims()
  {
    return kClaims;
  }

  // 9.4.1: conversion never runs in or through an enemy zone, except in a friendly city; nor
  // through an enemy city. A partisan's zone counts (it inhibits Axis conversion; for the Russians
  // it is their own and never an enemy zone).
  bool
  TrcRail::openP(const Ctx& ctx, HexIndex hex, SideId side) const
  {
    const std::optional<SideId> owner = ctx.position.control(hex);
    if (facts_.cityP(hex) && owner) {
      return side == *owner;
    }
    return !zoc_.blockedForP(ctx, hex, side, HexRules::Purpose::Network);
  }

  std::vector<std::size_t>
  TrcRail::linksOf(const std::vector<HexSearch::NodeIndex>& path) const
  {
    std::vector<std::size_t> out;
    for (std::size_t i = 1; i < path.size(); ++i) {
      out.push_back(*facts_.railLink(HexIndex{path[i - 1]}, HexIndex{path[i]}));
    }
    return out;
  }

  std::optional<std::vector<std::size_t>>
  TrcRail::traceBack(const Ctx& ctx, HexSearch::SearchScratch& scratch, HexIndex hex, SideId side) const
  {
    const Board& board = ctx.board;
    const NetworkId rail = facts_.railNetwork();
    const auto goalP = [&](HexIndex at) {
      return (facts_.cityP(at) && side == ctx.position.control(at)) || facts_.ownEdgeP(side, at);
    };
    if (!openP(ctx, hex, side)) {
      return std::nullopt;
    }
    const HexSearch::NetworkGraph graph(board, rail, [](std::size_t) { return true; });
    const std::function<bool(HexSearch::NodeIndex, std::size_t)> stepP = [&](HexSearch::NodeIndex node, std::size_t link) {
      const LinkNetwork::Link& arc = board.network(rail).links()[link];
      const HexIndex other = arc.a.value == node ? arc.b : arc.a;
      return (HexIndex{node} == hex || !goalP(HexIndex{node})) && openP(ctx, other, side);
    };
    const HexSearch::NodeIndex seed = hex.value;
    const HexSearch::Field<HexSearch::CostHalves> field = HexSearch::Algorithms::bfsFlood(
        graph, scratch, std::span<const HexSearch::NodeIndex>(&seed, 1), stepP, static_cast<int>(board.hexCount()));
    for (HexSearch::NodeIndex node : field.reached()) {
      if (goalP(HexIndex{node})) {
        return linksOf(field.pathTo(node));
      }
    }
    return std::nullopt;
  }

  Position
  TrcRail::noteTouched(const Ctx& ctx, const HexEngine::MoveUnit& move) const
  {
    Position next = ctx.position;
    const SideId side = ctx.roster.unit(move.units.front()).side;
    HexSearch::SearchScratch scratch;
    for (std::size_t i = 1; i < move.path.size(); ++i) {
      const HexIndex hex = move.path[i];
      if (facts_.railHexP(hex) && traceBack(ctx, scratch, hex, side)) {
        Flags::add(next, side, Flags::kRailTouched, ctx.board.id(hex).text);
      }
    }
    return next;
  }

  Position
  TrcRail::own(const Ctx& ctx, const std::vector<std::size_t>& links, SideId side, HexEngine::EventSink& sink) const
  {
    Position next = ctx.position;
    const LinkNetwork& network = ctx.board.network(facts_.railNetwork());
    std::string converted;
    for (std::size_t link : links) {
      if (side == next.linkOwner(facts_.railNetwork(), link)) {
        continue;
      }
      next.setLinkOwner(facts_.railNetwork(), link, side);
      converted += (converted.empty() ? "" : " ") + ctx.board.id(network.links()[link].a).text + "-" +
                   ctx.board.id(network.links()[link].b).text;
    }
    if (!converted.empty()) {
      sink.onEvent(HexEngine::GameEvent{"rail-converted", ctx.rules.sides()[side.value].id + " " + converted});
    }
    return next;
  }

  Position
  TrcRail::convert(const Ctx& ctx, SideId side, HexSearch::SearchScratch& scratch, HexEngine::EventSink& sink) const
  {
    Position next = ctx.position;
    for (const std::string& id : Flags::list(ctx.position, side, Flags::kRailTouched)) {
      const Ctx now{ctx.board, ctx.rules, ctx.roster, next};
      if (const std::optional<std::vector<std::size_t>> path =
              traceBack(now, scratch, ctx.board.indexOf(HexCoord::HexId{id}), side)) {
        next = own(now, *path, side, sink);
      }
    }
    next.setFlag(side, Flags::kRailTouched, std::nullopt);

    const Board& board = ctx.board;
    const NetworkId rail = facts_.railNetwork();
    for (std::size_t h = 0; h < board.hexCount(); ++h) {
      const HexIndex city{static_cast<std::uint32_t>(h)};
      if (!facts_.cityP(city) || side != next.control(city) || !facts_.railHexP(city)) {
        continue;
      }
      next = own(Ctx{ctx.board, ctx.rules, ctx.roster, next}, board.network(rail).linksAt(city), side, sink);
      const Ctx now{ctx.board, ctx.rules, ctx.roster, next};
      const auto clearP = [&](HexIndex at) {
        return !facts_.cityP(at) && !enemyUnitAtP(now, at, side) &&
               !zoc_.blockedForP(now, at, side, HexRules::Purpose::Network);
      };
      const HexSearch::NetworkGraph graph(board, rail, [](std::size_t) { return true; });
      const std::function<bool(HexSearch::NodeIndex, std::size_t)> stepP = [&](HexSearch::NodeIndex node, std::size_t link) {
        const LinkNetwork::Link& arc = board.network(rail).links()[link];
        const HexIndex other = arc.a.value == node ? arc.b : arc.a;
        const bool friendlyCityP = facts_.cityP(other) && side == now.position.control(other);
        return (HexIndex{node} == city || clearP(HexIndex{node})) && (friendlyCityP || clearP(other));
      };
      const HexSearch::NodeIndex seed = city.value;
      const HexSearch::Field<HexSearch::CostHalves> field = HexSearch::Algorithms::bfsFlood(
          graph, scratch, std::span<const HexSearch::NodeIndex>(&seed, 1), stepP, static_cast<int>(board.hexCount()));
      std::vector<std::size_t> lines;
      for (HexSearch::NodeIndex node : field.reached()) {
        if (node != city.value && facts_.cityP(HexIndex{node})) {
          const std::vector<std::size_t> path = linksOf(field.pathTo(node));
          lines.insert(lines.end(), path.begin(), path.end());
        }
      }
      next = own(now, lines, side, sink);
    }
    return next;
  }

}  // namespace Trc
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------

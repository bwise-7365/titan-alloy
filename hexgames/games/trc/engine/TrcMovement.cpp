// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
#include "TrcMovement.h"

#include "TrcMarkers.h"
#include "TrcState.h"

#include <array>
#include <stdexcept>

namespace Trc {

  namespace {

    constexpr std::array<std::string_view, 6> kClaims{"second-impulse", "zoc-pinning", "hq-movement",
                                                      "leader-movement", "leader-loss", "weather-effects"};

    const HexEngine::EntryVerdict kProhibited{HexModel::Prohibited{}, false};

    HexIndex
    hexOf(const Position& position, UnitId unit)
    {
      return std::get<HexIndex>(*position.unit(unit).where);
    }

  }  // namespace

  TrcMovement::TrcMovement(const TrcFacts& facts, const TrcZoc& zoc, const TrcWeather& weather)
    : facts_(facts), zoc_(zoc), weather_(weather)
  {
  }

  std::span<const std::string_view>
  TrcMovement::claims() const
  {
    return kClaims;
  }

  int
  TrcMovement::railCapacity(bool axisP, Weather weather)
  {
    if (!axisP) {
      return 5;
    }
    return Weather::Snow == weather ? 3 : 6;
  }

  int
  TrcMovement::railMovesUsed(const Position& position, SideId side) const
  {
    return stateOf(position).side(side).railMoves.value_or(0);
  }

  std::optional<Impulse>
  TrcMovement::impulseNow(const Ctx& ctx) const
  {
    const PhaseInfo info = facts_.phase(ctx.position.clock().phase);
    if (PhaseKind::Move != info.kind) {
      return std::nullopt;
    }
    return info.impulse;
  }

  Budget
  TrcMovement::allowance(const Ctx& ctx, UnitId unit, ModeId mode) const
  {
    if (facts_.normalMode() == mode) {
      return HexCount{normalHexes(ctx, unit)};
    }
    if (facts_.railMode() == mode) {
      return HexCount{railHexes(ctx, unit)};
    }
    throw std::invalid_argument("TrcMovement: mode '" + ctx.rules.movement().modes[mode.value].id +
                                 "' is a command of its own (sea-move, paradrop), not a move");
  }

  int
  TrcMovement::normalHexes(const Ctx& ctx, UnitId unit) const
  {
    const std::optional<Impulse> impulse = impulseNow(ctx);
    const HexModel::UnitState& state = ctx.position.unit(unit);
    if (!impulse || state.flags.movedP || Markers::markedP(ctx.position, unit, Markers::kAv) ||
        Markers::markedP(ctx.position, unit, Markers::kDropped)) {
      return 0;
    }
    if (facts_.typeP(unit, "worker") || facts_.typeP(unit, "partisan") || facts_.typeP(unit, "leader")) {
      return 0;  // workers never move once placed; partisans relocate; leaders go by rail or sea (11.3)
    }
    const UnitSpec& spec = ctx.roster.unit(unit);
    if (!spec.front.allowance) {
      throw std::invalid_argument("TrcMovement: counter '" + spec.counter.text + "' prints no movement factor");
    }
    const int printed = std::get<MovementPoints>(*spec.front.allowance).halves / 2;
    if (facts_.typeP(unit, "hq")) {
      return Impulse::Second == *impulse ? printed : 0;  // 11.1
    }
    if (Impulse::Second == *impulse && zoc_.enemyZocP(ctx, hexOf(ctx.position, unit), spec.side, true)) {
      return 0;  // 8.4: pinned
    }
    const bool leaderNationP = facts_.axis() == spec.side ? Nation::German == facts_.nation(unit) : true;
    if (leaderNationP && LeaderLost::Active == stateOf(ctx.position).side(spec.side).leaderLost) {
      return 0;  // 11.3
    }
    return chartAllowance(facts_, unit, weather_.current(ctx.position), *impulse, printed);
  }

  int
  TrcMovement::railHexes(const Ctx& ctx, UnitId unit) const
  {
    const std::optional<Impulse> impulse = impulseNow(ctx);
    const HexModel::UnitState& state = ctx.position.unit(unit);
    if (Impulse::First != impulse || state.flags.movedP || Markers::markedP(ctx.position, unit, Markers::kAv)) {
      return 0;
    }
    if (facts_.typeP(unit, "hq") || facts_.typeP(unit, "worker") || facts_.typeP(unit, "partisan")) {
      return 0;
    }
    const SideId side = ctx.roster.unit(unit).side;
    const HexIndex hex = hexOf(ctx.position, unit);
    bool friendlyRailP = false;
    for (std::size_t link : ctx.board.network(facts_.railNetwork()).linksAt(hex)) {
      friendlyRailP = friendlyRailP || side == ctx.position.linkOwner(facts_.railNetwork(), link);
    }
    if (!friendlyRailP || zoc_.blockedForP(ctx, hex, side, HexRules::Purpose::Network)) {
      return 0;  // 9.3.2
    }
    const int capacity = railCapacity(facts_.axis() == side, weather_.current(ctx.position));
    if (capacity <= railMovesUsed(ctx.position, side)) {
      return 0;
    }
    return static_cast<int>(ctx.board.hexCount());
  }

  HexEngine::EntryVerdict
  TrcMovement::enter(const Ctx& ctx, UnitId unit, HexIndex from, Direction direction, ModeId mode) const
  {
    const std::optional<HexIndex> to = ctx.board.neighbour(from, direction);
    if (!to) {
      return kProhibited;
    }
    if (facts_.railMode() == mode) {
      return enterByRail(ctx, unit, from, *to);
    }
    if (facts_.normalMode() == mode) {
      return enterNormally(ctx, unit, from, *to, direction);
    }
    return kProhibited;
  }

  HexEngine::EntryVerdict
  TrcMovement::enterNormally(const Ctx& ctx, UnitId unit, HexIndex from, HexIndex to, Direction direction) const
  {
    if (facts_.waterP(to) || facts_.blockedHexsideP(from, direction)) {
      return kProhibited;
    }
    const SideId side = ctx.roster.unit(unit).side;
    const bool intoZocP = zoc_.enemyZocP(ctx, to, side, true);
    if (intoZocP && zoc_.enemyZocP(ctx, from, side, true)) {
      return kProhibited;  // 8.3: no zone to zone
    }
    if (intoZocP && Markers::markedP(ctx.position, unit, Markers::kAvFirst)) {
      return kProhibited;  // 16.2
    }
    if (intoZocP && facts_.typeP(unit, "hq")) {
      bool friendlyP = false;
      for (UnitId occupant : ctx.position.unitsAt(to)) {
        friendlyP = friendlyP || (side == ctx.roster.unit(occupant).side && !facts_.typeP(occupant, "hq"));
      }
      if (!friendlyP) {
        return kProhibited;  // 11.1
      }
    }
    if (stateOf(ctx.position).helsinkiRussianP && facts_.inCountryP(to, "finland")) {
      return kProhibited;  // 24.0: nobody enters Finland after it surrenders
    }
    return HexEngine::EntryVerdict{MovementPoints::whole(1), stopsInP(ctx, unit, to, facts_.normalMode())};
  }

  HexEngine::EntryVerdict
  TrcMovement::enterByRail(const Ctx& ctx, UnitId unit, HexIndex from, HexIndex to) const
  {
    const std::optional<std::size_t> link = facts_.railLink(from, to);
    const SideId side = ctx.roster.unit(unit).side;
    if (!link || side != ctx.position.linkOwner(facts_.railNetwork(), *link)) {
      return kProhibited;
    }
    if (zoc_.blockedForP(ctx, to, side, HexRules::Purpose::Network)) {
      return kProhibited;
    }
    return HexEngine::EntryVerdict{MovementPoints::whole(1), false};
  }

  bool
  TrcMovement::stopsInP(const Ctx& ctx, UnitId unit, HexIndex hex, ModeId mode) const
  {
    if (facts_.railMode() == mode) {
      return false;  // 9.3: terrain is ignored
    }
    for (int d = 0; d < HexCoord::kDirections; ++d) {
      const std::optional<HexIndex> near = ctx.board.neighbour(hex, static_cast<Direction>(d));
      if (near && facts_.kerchP(hex, *near)) {
        return true;  // 8.5; see the task file: every entry into KK19 or KK20 stops
      }
    }
    const HexRules::Terrain& terrain = ctx.rules.hexTerrain()[ctx.board.terrain(hex).value];
    if ("swamp" == terrain.id && Weather::Snow == weather_.current(ctx.position)) {
      return false;  // treated as clear during snow
    }
    const UnitTypeId type = ctx.roster.unit(unit).type;
    return terrain.stopP && terrain.stopExcept.end() == std::find(terrain.stopExcept.begin(), terrain.stopExcept.end(), type);
  }

}  // namespace Trc
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------

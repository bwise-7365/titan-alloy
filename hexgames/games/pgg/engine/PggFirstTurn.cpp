// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
#include "PggFirstTurn.h"

#include "PggState.h"
#include "PggUnits.h"

#include <algorithm>
#include <array>
#include <set>
#include <stdexcept>

namespace Pgg {

  namespace {

    constexpr std::array<std::string_view, 2> kClaims{"soviet-first-turn-die", "soviet-first-turn-mandatory"};

    [[noreturn]] void
    refuse(const Ctx& ctx, UnitId unit, const std::string& why)
    {
      throw std::invalid_argument("PggFirstTurn: counter '" + ctx.roster.unit(unit).counter.text + "' " + why);
    }

    bool
    marchingP(Army army)
    {
      return Army::Thirteenth == army || Army::Twentieth == army;
    }

  }  // namespace

  PggFirstTurn::PggFirstTurn(const PggFacts& facts, const PggMovement& movement, const PggStacking& stacking)
    : facts_(facts), movement_(movement), stacking_(stacking)
  {
  }

  std::span<const std::string_view>
  PggFirstTurn::claims()
  {
    return kClaims;
  }

  Position
  PggFirstTurn::rollArmies(const Ctx& ctx, HexEngine::PrngStreams& streams, HexEngine::EventSink& sink) const
  {
    Position next = ctx.position;
    for (Army army : {Army::Sixteenth, Army::Nineteenth}) {
      const int die = HexEngine::rollDie(streams, HexEngine::StreamTag::Setup, 6);
      sink.onEvent(HexEngine::DieRolled{HexEngine::StreamTag::Setup, die});
      const bool standsP = 3 < die;
      if (standsP) {
        stateOf(next).frozen.insert(army);
      }
      sink.onEvent(HexEngine::GameEvent{"first-turn-army", std::to_string(armyNumber(army)) + (standsP ? " stands" : " moves")});
    }
    return next;
  }

  bool
  PggFirstTurn::mayGoOnP(const Ctx& ctx, UnitId unit, HexIndex at, int halvesLeft,
                         const std::vector<HexIndex>& visited) const
  {
    const SideId side = ctx.roster.unit(unit).side;
    for (int d = 0; d < HexCoord::kDirections; ++d) {
      const Direction direction = static_cast<Direction>(d);
      const std::optional<HexIndex> to = ctx.board.neighbour(at, direction);
      if (!to || facts_.column(*to) < facts_.column(at) || Units::enemyAtP(ctx, *to, side) ||
          visited.end() != std::find(visited.begin(), visited.end(), *to)) {
        continue;
      }
      const HexEngine::EntryVerdict verdict = movement_.enter(ctx, unit, at, direction, facts_.normalMode());
      if (std::holds_alternative<MovementPoints>(verdict.cost) &&
          std::get<MovementPoints>(verdict.cost).halves <= halvesLeft) {
        return true;
      }
    }
    return false;
  }

  void
  PggFirstTurn::checkMove(const Ctx& ctx, const HexEngine::MoveUnit& move) const
  {
    const PggState& state = stateOf(ctx.position);
    for (UnitId unit : move.units) {
      const auto found = state.armies.find(unit);
      if (state.armies.end() == found) {
        continue;
      }
      const Army army = found->second;
      if (state.frozen.contains(army)) {
        if (facts_.normalMode() != move.mode || 2 != move.path.size() ||
            stacking_.excess(ctx, move.path.front()).empty()) {
          refuse(ctx, unit, "belongs to an army that stands this turn; it may move one hex off an overstacked hex only (5.21, 5.22)");
        }
        continue;
      }
      if (!marchingP(army)) {
        continue;
      }
      if (facts_.normalMode() != move.mode) {
        refuse(ctx, unit, "belongs to the 13th or 20th Army, which may not use Rail Movement on turn 1 (5.23)");
      }
      const std::set<HexIndex> distinct(move.path.begin(), move.path.end());
      if (distinct.size() != move.path.size() || facts_.westmostColumnsP(move.path.back())) {
        refuse(ctx, unit, "may not re-enter a hex or end in hex-columns 01 or 02 on turn 1 (5.23)");
      }
      for (std::size_t i = 1; i < move.path.size(); ++i) {
        if (facts_.column(move.path[i]) < facts_.column(move.path[i - 1])) {
          refuse(ctx, unit, "may move only north, south or east on turn 1 (5.23)");
        }
      }
      const int allowance = std::get<MovementPoints>(movement_.allowance(ctx, unit, move.mode)).halves;
      const std::optional<int> cost = movement_.pathHalves(ctx, unit, move.path, move.mode);
      if (cost && mayGoOnP(ctx, unit, move.path.back(), allowance - *cost, move.path)) {
        refuse(ctx, unit, "must expend its full Movement Allowance on turn 1 (5.23)");
      }
    }
    return;
  }

  void
  PggFirstTurn::checkEnd(const Ctx& ctx) const
  {
    const PggState& state = stateOf(ctx.position);
    for (const auto& [unit, army] : state.armies) {
      const std::optional<HexIndex> hex = Units::hexOf(ctx.position, unit);
      if (!marchingP(army) || !hex || ctx.position.unit(unit).flags.movedP) {
        continue;
      }
      const int allowance = std::get<MovementPoints>(movement_.allowance(ctx, unit, facts_.normalMode())).halves;
      if (mayGoOnP(ctx, unit, *hex, allowance, {*hex})) {
        refuse(ctx, unit, "has not moved, and the 13th and 20th Armies must expend their full allowance on turn 1 (5.23)");
      }
    }
    return;
  }

  void
  PggFirstTurn::check(const Ctx& ctx, const HexEngine::Command& command) const
  {
    if (const HexEngine::MoveUnit* move = std::get_if<HexEngine::MoveUnit>(&command)) {
      checkMove(ctx, *move);
    } else if (std::holds_alternative<HexEngine::EndPhase>(command)) {
      checkEnd(ctx);
    } else if (const HexEngine::GameCommand* game = std::get_if<HexEngine::GameCommand>(&command)) {
      const PggState& state = stateOf(ctx.position);
      if ("overrun" == game->verb && !game->args.empty()) {
        const UnitId first = facts_.counter(game->args[0].substr(0, game->args[0].find(' ')));
        const auto found = state.armies.find(first);
        if (state.armies.end() != found && state.frozen.contains(found->second)) {
          refuse(ctx, first, "belongs to an army that stands this turn (5.21)");
        }
      }
    }
    return;
  }

}  // namespace Pgg
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------

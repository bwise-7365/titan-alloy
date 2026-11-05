// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
#include "PggAgent.h"

#include "PggSchedule.h"
#include "PggUnits.h"

#include <algorithm>
#include <functional>
#include <tuple>

namespace Pgg {

  namespace {

    constexpr int kLeaderRally = 8;  // hexes: a Leader follows the divisions this near (no HexKrieg counterpart)

    bool
    marchingP(Army army)
    {
      return Army::Thirteenth == army || Army::Twentieth == army;
    }

    // The search for a 5.23 march tries every order of steps, so it is bounded; it can only fail to find a
    // hex with room, and then the march ends where the first path does.
    constexpr int kMarchNodes = 4000;

  }  // namespace

  // The area's hexes a unit may enter, those with the fewest units first, then those nearest the enemy. A
  // seizing division comes in together: a hex holding its own regiments comes before the rest.
  std::vector<HexIndex>
  PggAgent::entryHexes(const Ctx& ctx, Area area, std::optional<UnitId> unit) const
  {
    std::vector<HexIndex> out;
    for (HexIndex hex : set_.facts().area(area)) {
      if (!set_.facts().lakeP(hex) && !Units::enemyAtP(ctx, hex, side_)) {
        out.push_back(hex);
      }
    }
    const std::optional<Division> division = unit && seizingP() ? set_.facts().division(*unit) : std::nullopt;
    const auto key = [&](HexIndex hex) {
      const std::vector<UnitId>& there = ctx.position.unitsAt(hex);
      const bool strangerP = !division || std::none_of(there.begin(), there.end(), [&](UnitId occupant) { return division == set_.facts().division(occupant); });
      return std::make_tuple(strangerP, there.size(), field_->steps(hex).value_or(ctx.board.hexCount()), hex.value);
    };
    std::sort(out.begin(), out.end(), [&](HexIndex l, HexIndex r) { return key(l) < key(r); });
    return out;
  }

  std::optional<HexEngine::Command>
  PggAgent::nextArrival(const HexEngine::Session& session)
  {
    const Ctx ctx = session.context();
    const PggFacts& facts = set_.facts();
    const SpaceId box = facts.german() == side_ ? facts.germanArrivals() : facts.sovietArrivals();
    for (UnitId unit : ctx.position.unitsIn(box)) {
      if (0 < ctx.position.unit(unit).delay.value_or(0) || memory_.arrivals.contains(unit)) {
        continue;
      }
      std::optional<Area> area;
      if (const std::optional<GermanArrival> german = germanArrivalOf(facts, unit)) {
        area = german->area;
      } else if (const std::optional<SovietArrival> leader = leaderArrivalOf(facts, unit)) {
        area = leader->area;
      }
      const std::vector<HexIndex> hexes = area ? entryHexes(ctx, *area, unit) : std::vector<HexIndex>{};
      if (!hexes.empty()) {
        return HexEngine::Place{unit, hexes.front()};
      }
    }
    return std::nullopt;
  }

  std::optional<HexEngine::Command>
  PggAgent::nextReinforcement(const HexEngine::Session& session)
  {
    const Ctx ctx = session.context();
    if (set_.facts().soviet() != side_) {
      return std::nullopt;
    }
    for (const auto& [area, owe] : stateOf(ctx.position).owed) {
      for (const auto& [type, count] : {std::make_pair(std::string("soviet-rifle"), owe.rifles),
                                        std::make_pair(std::string("soviet-armour"), owe.armour)}) {
        if (0 >= count) {
          continue;
        }
        for (HexIndex hex : entryHexes(ctx, area, std::nullopt)) {
          const std::string id = ctx.board.id(hex).text;
          if (!memory_.entries.contains(type + " " + id)) {
            return HexEngine::GameCommand{"reinforce", {type, id, "scheduled"}};
          }
        }
      }
    }
    return std::nullopt;
  }

  // 5.22 and 5.23. An army that stands moves one hex, off an overstacked hex only. A marching unit never
  // goes west or back on its path and goes on until it can afford no further step: east where it can,
  // ending clear of hex-columns 01 and 02 and in a hex with room.
  std::optional<std::vector<HexIndex>>
  PggAgent::firstTurnPath(const Ctx& ctx, UnitId unit, Army army) const
  {
    const PggFacts& facts = set_.facts();
    const HexIndex from = *Units::hexOf(ctx.position, unit);
    const ModeId mode = facts.normalMode();
    const auto stepsFrom = [&](HexIndex at, int left, const std::vector<HexIndex>& path) {
      std::vector<std::tuple<int, int, std::uint32_t, HexIndex>> steps;
      for (int d = 0; d < HexCoord::kDirections; ++d) {
        const HexModel::Direction direction = static_cast<HexModel::Direction>(d);
        const std::optional<HexIndex> to = ctx.board.neighbour(at, direction);
        if (!to || facts.column(*to) < facts.column(at) || Units::enemyAtP(ctx, *to, side_) ||
            path.end() != std::find(path.begin(), path.end(), *to)) {
          continue;
        }
        const HexEngine::EntryVerdict verdict = set_.movement().enter(ctx, unit, at, direction, mode);
        if (std::holds_alternative<MovementPoints>(verdict.cost) && std::get<MovementPoints>(verdict.cost).halves <= left) {
          const int cost = std::get<MovementPoints>(verdict.cost).halves;
          steps.emplace_back(-facts.column(*to), cost, to->value, *to);
        }
      }
      std::sort(steps.begin(), steps.end());
      return steps;
    };
    if (!marchingP(army)) {
      if (set_.stacking().excess(ctx, from).empty()) {
        return std::nullopt;
      }
      for (int d = 0; d < HexCoord::kDirections; ++d) {
        const HexModel::Direction direction = static_cast<HexModel::Direction>(d);
        const std::optional<HexIndex> to = ctx.board.neighbour(from, direction);
        if (to && !Units::enemyAtP(ctx, *to, side_) && roomP(ctx, *to, unit) &&
            std::holds_alternative<MovementPoints>(set_.movement().enter(ctx, unit, from, direction, mode).cost)) {
          return std::vector<HexIndex>{from, *to};
        }
      }
      return std::nullopt;
    }
    const int allowance = std::get<MovementPoints>(set_.movement().allowance(ctx, unit, mode)).halves;
    std::vector<HexIndex> path{from};
    std::optional<std::vector<HexIndex>> fallback;
    int nodes = 0;
    const std::function<bool(HexIndex, int)> march = [&](HexIndex at, int left) {
      nodes += 1;
      const auto steps = stepsFrom(at, left, path);
      if (steps.empty()) {
        if (!facts.westmostColumnsP(at) && 2 <= path.size()) {
          fallback = fallback ? fallback : std::optional<std::vector<HexIndex>>(path);
          return roomP(ctx, at, unit);
        }
        return false;
      }
      for (const auto& step : steps) {
        path.push_back(std::get<HexIndex>(step));
        if (march(std::get<HexIndex>(step), left - std::get<1>(step))) {
          return true;
        }
        path.pop_back();
        if (kMarchNodes < nodes && fallback) {
          return false;
        }
      }
      return false;
    };
    if (march(from, allowance)) {
      return path;
    }
    return fallback;
  }

  std::optional<HexEngine::Command>
  PggAgent::nextMove(const HexEngine::Session& session, const PggSeen& seen)
  {
    const Ctx ctx = session.context();
    const PggFacts& facts = set_.facts();
    const ModeId mode = facts.normalMode();
    const std::vector<HexAi::Piece> friends = seen.pieces(side_);
    const std::vector<HexAi::Piece> enemies = seen.pieces(facts.enemy(side_));
    const PggState& state = stateOf(ctx.position);
    if (seizingP()) {
      planSeizure(session, seen);
    }
    const auto restrictedP = [&](UnitId unit) {
      const auto army = state.armies.find(unit);
      return 1 == ctx.position.clock().turn && state.armies.end() != army &&
             (marchingP(army->second) || state.frozen.contains(army->second));
    };
    for (UnitId unit : Units::onMap(ctx, facts, side_)) {
      if ((leadersCoverP() && facts.leaderP(unit) && !restrictedP(unit)) || !memory_.considered.insert(unit).second) {
        continue;
      }
      const HexIndex from = *Units::hexOf(ctx.position, unit);
      const auto army = state.armies.find(unit);
      if (restrictedP(unit)) {
        const std::optional<std::vector<HexIndex>> path = firstTurnPath(ctx, unit, army->second);
        if (path) {
          return HexEngine::MoveUnit{{unit}, mode, *path};
        }
        continue;
      }
      if (seizingP() && !facts.leaderP(unit)) {
        if (const std::optional<HexEngine::Command> move = seizeMove(session, ctx, unit)) {
          return move;
        }
        continue;
      }
      const UnitId units[] = {unit};
      const HexEngine::Reachability reach = session.reachable(units, mode);
      const std::set<HexIndex>& refusedHexes = memory_.hexes[unit];
      const bool leaderP = facts.leaderP(unit);
      const auto allowedP = [&](HexIndex hex) {
        return !refusedHexes.contains(hex) && roomP(ctx, hex, unit) && (!leaderP || !set_.zoc().enemyZocP(ctx, hex, side_));
      };
      const std::vector<UnitId> excess = set_.stacking().excess(ctx, from);
      const bool mustLeaveP = excess.end() != std::find(excess.begin(), excess.end(), unit);
      const HexAi::Piece me{from, seen.attack(unit)};
      const HexAi::Goal goal = leaderP ? HexAi::Goal{HexAi::rallyOf(ctx.board, from, kLeaderRally, friends)}
                                       : HexAi::goalFor(ctx.board, tactic_, me, friends, enemies);
      const std::optional<std::size_t> best = HexAi::chooseDestination(ctx.board, *field_, goal, reach, from, mustLeaveP, allowedP);
      if (best) {
        return HexEngine::MoveUnit{{unit}, mode, reach.paths[*best]};
      }
    }
    if (leadersCoverP()) {
      for (UnitId unit : Units::onMap(ctx, facts, side_)) {
        if (facts.leaderP(unit) && memory_.considered.insert(unit).second) {
          if (const std::optional<HexEngine::Command> move = coverMove(session, ctx, unit)) {
            return move;
          }
        }
      }
    }
    return std::nullopt;
  }

}  // namespace Pgg
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------

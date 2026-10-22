// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
#include "PggVictory.h"

#include "PggState.h"
#include "PggUnits.h"

#include <algorithm>
#include <array>
#include <stdexcept>

namespace Pgg {

  namespace {

    constexpr std::array<std::string_view, 4> kClaims{"german-vp-occupation", "german-vp-swf",
                                                      "soviet-vp-eliminated-division", "soviet-vp-recapture"};

  }  // namespace

  PggVictory::PggVictory(const PggFacts& facts, const PggSupply& supply) : facts_(facts), supply_(supply)
  {
  }

  std::span<const std::string_view>
  PggVictory::claims() const
  {
    return kClaims;
  }

  std::optional<HexEngine::Outcome>
  PggVictory::check(const Ctx& ctx) const
  {
    const std::optional<std::size_t> outcome = stateOf(ctx.position).outcome;
    if (!outcome) {
      return std::nullopt;
    }
    const VictoryLevel& level = facts_.victoryLevels().at(*outcome);
    return HexEngine::Outcome{level.winner, level.name};
  }

  int
  PggVictory::hexPoints(const Ctx& ctx, const VictoryHex& vp) const
  {
    if (vp.hex != facts_.smolensk()) {
      return vp.points;
    }
    const int taken = stateOf(ctx.position).smolenskTaken.value_or(ctx.position.clock().turn);
    return std::max(0, vp.points - 2 * std::max(0, taken - 4));  // amendments 15.11
  }

  int
  PggVictory::germanPoints(const Ctx& ctx, HexSearch::SearchScratch& scratch) const
  {
    int points = 0;
    for (const VictoryHex& vp : facts_.victoryHexes()) {
      if (facts_.german() == ctx.position.control(vp.hex) && supply_.lineWestP(ctx, scratch, vp.hex)) {
        points += hexPoints(ctx, vp);
      }
    }
    const int used = stateOf(ctx.position).swfUsed;
    return points + std::min(used, 5) + 2 * std::max(0, used - 5);  // 14.23
  }

  bool
  PggVictory::divisionEliminatedP(const Ctx& ctx, Division division) const
  {
    const std::set<UnitId>& gone = stateOf(ctx.position).germanEliminated;
    const std::vector<UnitId>& members = facts_.members(division);
    return std::all_of(members.begin(), members.end(), [&](UnitId unit) { return gone.contains(unit); });
  }

  int
  PggVictory::sovietPoints(const Ctx& ctx) const
  {
    int points = stateOf(ctx.position).recaptureVp;
    for (Division division : facts_.divisions()) {
      if (DivisionKind::Cavalry != division.kind && DivisionKind::Independent != division.kind &&
          divisionEliminatedP(ctx, division)) {
        points += 1;
      }
    }
    return points;
  }

  Position
  PggVictory::declare(const Ctx& ctx, HexSearch::SearchScratch& scratch, HexEngine::EventSink& sink) const
  {
    const int german = germanPoints(ctx, scratch);
    const int soviet = sovietPoints(ctx);
    const int margin = german - soviet;
    const std::vector<VictoryLevel>& levels = facts_.victoryLevels();
    for (std::size_t i = 0; i < levels.size(); ++i) {
      const bool aboveP = !levels[i].lowest || *levels[i].lowest <= margin;
      const bool belowP = !levels[i].highest || margin <= *levels[i].highest;
      if (aboveP && belowP) {
        Position next = ctx.position;
        stateOf(next).outcome = i;
        sink.onEvent(HexEngine::GameEvent{"victory-points", "german " + std::to_string(german) + ", soviet " +
                                                                 std::to_string(soviet)});
        sink.onEvent(HexEngine::VictoryDeclared{levels[i].winner, levels[i].name});
        return next;
      }
    }
    throw std::invalid_argument("PggVictory: no level of victory covers a margin of " + std::to_string(margin));
  }

  Position
  PggVictory::recordGermanCities(const Ctx& ctx, HexSearch::SearchScratch& scratch) const
  {
    Position next = ctx.position;
    PggState& state = stateOf(next);
    state.germanHeld.clear();
    for (const VictoryHex& vp : facts_.victoryHexes()) {
      if (facts_.german() == ctx.position.control(vp.hex) && supply_.lineWestP(ctx, scratch, vp.hex)) {
        state.germanHeld.insert(vp.hex);
      }
    }
    if (facts_.german() != ctx.position.control(facts_.smolensk())) {
      state.smolenskTaken.reset();
    } else if (!state.smolenskTaken) {
      state.smolenskTaken = ctx.position.clock().turn;
    }
    return next;
  }

  Position
  PggVictory::scoreRecaptures(const Ctx& ctx, HexEngine::EventSink& sink) const
  {
    Position next = ctx.position;
    PggState& state = stateOf(next);
    for (HexIndex hex : state.germanHeld) {
      if (Units::friendlyAtP(ctx, hex, facts_.soviet())) {
        state.recaptureVp += 2;  // 15.12
        sink.onEvent(HexEngine::GameEvent{"recaptured", ctx.board.id(hex).text});
      }
    }
    return next;
  }

  Position
  PggVictory::noteControl(const Ctx& ctx, const HexEngine::Command& command, HexEngine::EventSink& sink) const
  {
    Position next = ctx.position;
    const auto take = [&](HexIndex hex, SideId side) {
      if (side != next.control(hex)) {
        next.setControl(hex, side);
        sink.onEvent(HexEngine::ControlChanged{hex, side});
      }
      return;
    };
    if (const HexEngine::MoveUnit* move = std::get_if<HexEngine::MoveUnit>(&command)) {
      const SideId side = ctx.roster.unit(move->units.front()).side;
      for (std::size_t i = 1; i < move->path.size(); ++i) {
        for (const VictoryHex& vp : facts_.victoryHexes()) {
          if (vp.hex == move->path[i]) {
            take(vp.hex, side);
          }
        }
      }
    }
    for (const VictoryHex& vp : facts_.victoryHexes()) {
      for (UnitId unit : ctx.position.unitsAt(vp.hex)) {
        if (!facts_.markerP(unit)) {
          take(vp.hex, ctx.roster.unit(unit).side);
        }
      }
    }
    return next;
  }

}  // namespace Pgg
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------

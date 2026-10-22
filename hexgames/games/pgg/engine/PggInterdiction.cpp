// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
#include "PggInterdiction.h"

#include "PggState.h"
#include "PggUnits.h"

#include <algorithm>
#include <array>
#include <stdexcept>

namespace Pgg {

  namespace {

    constexpr std::array<std::string_view, 3> kClaims{"german-interdiction-placement", "rail-cut-place",
                                                      "rail-cut-repair"};

    [[noreturn]] void
    refuse(const std::string& why)
    {
      throw std::invalid_argument("PggInterdiction: " + why);
    }

  }  // namespace

  PggInterdiction::PggInterdiction(const PggFacts& facts, const PggSupply& supply) : facts_(facts), supply_(supply)
  {
  }

  std::span<const std::string_view>
  PggInterdiction::claims()
  {
    return kClaims;
  }

  HexIndex
  PggInterdiction::targetOf(const HexEngine::GameCommand& command) const
  {
    if (1 != command.args.size()) {
      refuse("'" + command.verb + "' needs one target hex");
    }
    return facts_.hex(command.args[0]);
  }

  Position
  PggInterdiction::placeAir(const Ctx& ctx, HexIndex hex, HexEngine::EventSink& sink) const
  {
    const PggState& state = stateOf(ctx.position);
    if (kAirMarkers <= state.airInterdiction.size()) {
      refuse("the German Player has three Air Interdiction Markers (13.1)");
    }
    if (state.airInterdiction.end() != std::find(state.airInterdiction.begin(), state.airInterdiction.end(), hex)) {
      refuse("hex '" + ctx.board.id(hex).text + "' already holds an Air Interdiction Marker (13.33)");
    }
    if (kEastmostColumn < facts_.column(hex)) {
      HexSearch::SearchScratch scratch;
      const bool heldP = state.smolenskTaken && *state.smolenskTaken < ctx.position.clock().turn &&
                         facts_.german() == ctx.position.control(facts_.smolensk()) &&
                         supply_.lineWestP(ctx, scratch, facts_.smolensk());
      if (!heldP) {
        refuse("hex '" + ctx.board.id(hex).text + "' lies east of hex-row 4000 and Smolensk is not held (13.2)");
      }
    }
    Position next = ctx.position;
    stateOf(next).airInterdiction.push_back(hex);
    sink.onEvent(HexEngine::GameEvent{"air-interdiction", ctx.board.id(hex).text});
    return next;
  }

  Position
  PggInterdiction::placeSoviet(const Ctx& ctx, HexIndex hex, HexEngine::EventSink& sink) const
  {
    const PggState& state = stateOf(ctx.position);
    if (12 <= ctx.position.clock().turn || kSovietTurns <= state.sovietInterdictionTurns) {
      refuse("the Soviet Player interdicts on three turns at most, never turn 12 (13.41)");
    }
    if (state.sovietInterdiction) {
      refuse("the Soviet Interdiction Marker is already on the map");
    }
    if (Units::enemyAtP(ctx, hex, facts_.soviet())) {
      refuse("hex '" + ctx.board.id(hex).text + "' holds a German unit (13.45)");
    }
    Position next = ctx.position;
    stateOf(next).sovietInterdiction = hex;
    stateOf(next).sovietInterdictionTurns += 1;
    sink.onEvent(HexEngine::GameEvent{"soviet-interdiction", ctx.board.id(hex).text});
    return next;
  }

  Position
  PggInterdiction::interdict(const Ctx& ctx, const HexEngine::GameCommand& command, HexEngine::EventSink& sink) const
  {
    const HexIndex hex = targetOf(command);
    const PhaseInfo phase = facts_.phase(ctx.position.clock().phase);
    if (facts_.lakeP(hex)) {
      refuse("hex '" + ctx.board.id(hex).text + "' is a Lake");
    }
    if (PhaseKind::SetUp == phase.kind || PhaseKind::AirInterdiction == phase.kind) {
      return placeAir(ctx, hex, sink);
    }
    if (PhaseKind::SovietInterdiction == phase.kind) {
      return placeSoviet(ctx, hex, sink);
    }
    refuse("markers are placed only before the game and in the two interdiction phases (13.2, 13.42)");
  }

  void
  PggInterdiction::checkGermanMovement(const Ctx& ctx, const HexEngine::GameCommand& command) const
  {
    const PhaseInfo phase = facts_.phase(ctx.position.clock().phase);
    const bool movingP = PhaseKind::Move == phase.kind || PhaseKind::MechanizedMove == phase.kind;
    if (!movingP || facts_.german() != phase.side) {
      refuse("'" + command.verb + "' is a German movement phase action (errata 6.37)");
    }
    const HexIndex hex = targetOf(command);
    if (!stateOf(ctx.position).passedByGermans.contains(hex)) {
      refuse("no German combat unit has moved through hex '" + ctx.board.id(hex).text + "' this phase (errata 6.37)");
    }
    return;
  }

  Position
  PggInterdiction::cutRail(const Ctx& ctx, const HexEngine::GameCommand& command, HexEngine::EventSink& sink) const
  {
    checkGermanMovement(ctx, command);
    const HexIndex hex = targetOf(command);
    const PggState& state = stateOf(ctx.position);
    if (!facts_.railHexP(hex) || state.railCuts.contains(hex) || kRailCuts <= state.railCuts.size()) {
      refuse("hex '" + ctx.board.id(hex).text + "' is not an uncut Railroad hex, or all six markers are in use");
    }
    Position next = ctx.position;
    stateOf(next).railCuts.insert(hex);
    sink.onEvent(HexEngine::GameEvent{"rail-cut", ctx.board.id(hex).text});
    return next;
  }

  Position
  PggInterdiction::liftCut(const Ctx& ctx, const HexEngine::GameCommand& command, HexEngine::EventSink& sink) const
  {
    checkGermanMovement(ctx, command);
    const HexIndex hex = targetOf(command);
    if (!stateOf(ctx.position).railCuts.contains(hex)) {
      refuse("hex '" + ctx.board.id(hex).text + "' holds no Rail Cut Marker");
    }
    Position next = ctx.position;
    stateOf(next).railCuts.erase(hex);
    sink.onEvent(HexEngine::GameEvent{"rail-cut-lifted", ctx.board.id(hex).text});
    return next;
  }

  Position
  PggInterdiction::removeAir(const Ctx& ctx, HexEngine::EventSink& sink) const
  {
    Position next = ctx.position;
    for (HexIndex hex : stateOf(next).airInterdiction) {
      sink.onEvent(HexEngine::GameEvent{"air-interdiction-removed", ctx.board.id(hex).text});
    }
    stateOf(next).airInterdiction.clear();
    return next;
  }

  Position
  PggInterdiction::removeSoviet(const Ctx& ctx, HexEngine::EventSink& sink) const
  {
    Position next = ctx.position;
    if (const std::optional<HexIndex> hex = stateOf(next).sovietInterdiction) {
      sink.onEvent(HexEngine::GameEvent{"soviet-interdiction-removed", ctx.board.id(*hex).text});
    }
    stateOf(next).sovietInterdiction.reset();
    return next;
  }

  Position
  PggInterdiction::notePassage(const Ctx& ctx, const HexEngine::MoveUnit& move, HexEngine::EventSink& sink) const
  {
    Position next = ctx.position;
    PggState& state = stateOf(next);
    const SideId side = ctx.roster.unit(move.units.front()).side;
    for (std::size_t i = 1; i < move.path.size(); ++i) {
      const HexIndex hex = move.path[i];
      if (facts_.german() == side && facts_.railHexP(hex)) {
        state.passedByGermans.insert(hex);
      }
      if (facts_.soviet() == side && facts_.normalMode() == move.mode && state.railCuts.contains(hex)) {
        state.railCuts.erase(hex);
        state.railRepaired[hex] = ctx.position.clock().turn;
        sink.onEvent(HexEngine::GameEvent{"rail-repaired", ctx.board.id(hex).text});
      }
    }
    return next;
  }

}  // namespace Pgg
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------

// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
#include "PggArrivals.h"

#include "PggSchedule.h"
#include "PggState.h"
#include "PggUnits.h"

#include <algorithm>
#include <array>
#include <stdexcept>

namespace Pgg {

  namespace {

    constexpr std::array<std::string_view, 7> kClaims{
        "soviet-provisional-reinforcement", "soviet-swf-reinforcement", "reinforcement-turn-12",
        "german-reinforcement-26th",        "german-reinforcement-268th", "untried-placement", "untried-dead-pile"};

    [[noreturn]] void
    refuse(const Ctx& ctx, UnitId unit, const std::string& why)
    {
      throw std::invalid_argument("PggArrivals: counter '" + ctx.roster.unit(unit).counter.text + "' " + why);
    }

    [[noreturn]] void
    refuse(const std::string& why)
    {
      throw std::invalid_argument("PggArrivals: " + why);
    }

    const std::array<Area, 6> kProvisional{Area::P1, Area::P2, Area::P3, Area::P4, Area::P5, Area::P6};

  }  // namespace

  PggArrivals::PggArrivals(const PggFacts& facts, const PggMovement& movement, const PggZoc& zoc)
    : facts_(facts), movement_(movement), zoc_(zoc)
  {
  }

  std::span<const std::string_view>
  PggArrivals::claims()
  {
    return kClaims;
  }

  void
  PggArrivals::enter(const Ctx& ctx, Position& next, UnitId unit, HexIndex hex, HexEngine::EventSink& sink) const
  {
    const SideId side = ctx.roster.unit(unit).side;
    next.state(unit).delay = std::nullopt;
    Units::place(next, unit, hex, sink);
    PggSideState& mine = stateOf(next).side(side);
    mine.entered.insert(unit);
    const Ctx there{ctx.board, ctx.rules, ctx.roster, next};
    mine.spent[unit] = movement_.entryHalves(there, unit, hex);  // 14.0
    return;
  }

  Position
  PggArrivals::place(const Ctx& ctx, const HexEngine::Place& command, HexEngine::EventSink& sink) const
  {
    const UnitId unit = command.unit;
    const SideId side = ctx.roster.unit(unit).side;
    if (!std::holds_alternative<HexIndex>(command.where)) {
      refuse(ctx, unit, "enters on a hex of its entrance area (14.3)");
    }
    const HexIndex hex = std::get<HexIndex>(command.where);
    const SpaceId box = facts_.german() == side ? facts_.germanArrivals() : facts_.sovietArrivals();
    if (!Units::inSpaceP(ctx.position, unit, box)) {
      refuse(ctx, unit, "is not waiting to enter play");
    }
    if (0 < ctx.position.unit(unit).delay.value_or(0)) {
      refuse(ctx, unit, "has not arrived yet: it enters in " + std::to_string(*ctx.position.unit(unit).delay) +
                            " turn(s) (16.0)");
    }
    const PhaseInfo phase = facts_.phase(ctx.position.clock().phase);
    if (PhaseKind::Move != phase.kind || side != phase.side) {
      refuse(ctx, unit, "enters only in its own side's first movement phase (4.2)");
    }
    std::optional<Area> area;
    if (const std::optional<GermanArrival> german = germanArrivalOf(facts_, unit)) {
      area = german->area;
    } else if (const std::optional<SovietArrival> leader = leaderArrivalOf(facts_, unit)) {
      area = leader->area;
    }
    if (!area || !facts_.inAreaP(*area, hex)) {
      refuse(ctx, unit, "enters in its own entrance area, and hex '" + ctx.board.id(hex).text + "' is not in it (14.3)");
    }
    if (facts_.lakeP(hex) || Units::enemyAtP(ctx, hex, side)) {
      refuse(ctx, unit, "cannot enter hex '" + ctx.board.id(hex).text + "' (6.21)");
    }
    Position next = ctx.position;
    enter(ctx, next, unit, hex, sink);
    return next;
  }

  std::optional<UnitId>
  PggArrivals::draw(const Ctx& ctx, Arm arm, HexEngine::PrngStreams& streams, HexEngine::EventSink& sink) const
  {
    for (SpaceId space : {facts_.sovietPool(), facts_.deadPile()}) {
      std::vector<UnitId> candidates;
      for (UnitId unit : ctx.position.unitsIn(space)) {
        if (arm == facts_.arm(unit)) {
          candidates.push_back(unit);
        }
      }
      if (candidates.empty()) {
        continue;
      }
      // TODO(decide): no stream tag names the pool draw, the Provisional Reinforcement die or the
      // first-turn army dice; all three use the Setup stream.
      const int die = HexEngine::rollDie(streams, HexEngine::StreamTag::Setup, static_cast<int>(candidates.size()));
      sink.onEvent(HexEngine::DieRolled{HexEngine::StreamTag::Setup, die});
      return candidates[static_cast<std::size_t>(die - 1)];
    }
    return std::nullopt;
  }

  Position
  PggArrivals::reinforce(const Ctx& ctx, const HexEngine::GameCommand& command, HexEngine::PrngStreams& streams,
                         HexEngine::EventSink& sink) const
  {
    if (3 != command.args.size()) {
      refuse("'reinforce' needs a type, a hex and a source");
    }
    const PhaseInfo phase = facts_.phase(ctx.position.clock().phase);
    if (PhaseKind::Move != phase.kind || facts_.soviet() != phase.side) {
      refuse("Soviet divisions enter only in the Soviet Movement Phase (4.2)");
    }
    Arm arm = Arm::SovietRifle;
    if ("soviet-armour" == command.args[0]) {
      arm = Arm::SovietArmour;
    } else if ("soviet-rifle" != command.args[0]) {
      refuse("'" + command.args[0] + "' is not a Soviet division type");
    }
    const HexIndex hex = facts_.hex(command.args[1]);
    if (facts_.lakeP(hex) || Units::enemyAtP(ctx, hex, facts_.soviet())) {
      refuse("a division cannot enter hex '" + command.args[1] + "' (6.21)");
    }
    Position next = ctx.position;
    PggState& state = stateOf(next);
    const int turn = ctx.position.clock().turn;
    if ("scheduled" == command.args[2]) {
      bool owedP = false;
      for (auto& [area, owe] : state.owed) {
        int& count = Arm::SovietArmour == arm ? owe.armour : owe.rifles;
        if (!owedP && 0 < count && facts_.inAreaP(area, hex)) {
          count -= 1;
          owedP = true;
        }
      }
      if (!owedP) {
        refuse("no scheduled division of that type is owed at hex '" + command.args[1] + "' (16.1)");
      }
    } else if ("south-western" == command.args[2]) {
      const bool edgeP = facts_.southEdgeP(hex) && facts_.railHexP(hex) && facts_.southWesternFrontColumn() <= facts_.column(hex);
      if (2 > turn || Arm::SovietRifle != arm || !edgeP || kSwfPerTurn <= state.swfThisTurn || kSwfPerGame <= state.swfUsed) {
        refuse("a South-Western Front rifle division enters from turn 2, on a south-edge Railroad hex at or east of Z, "
               "five a turn and ten a game (14.21, 14.22)");
      }
      state.swfThisTurn += 1;
      state.swfUsed += 1;
    } else {
      refuse("'" + command.args[2] + "' is not scheduled or south-western");
    }
    const std::optional<UnitId> unit = draw(ctx, arm, streams, sink);
    if (!unit) {
      refuse("no counter of that type is left in the pool or the dead pile (14.14)");
    }
    enter(ctx, next, *unit, hex, sink);
    next.state(*unit).flags.revealedP = false;
    return next;
  }

  Position
  PggArrivals::scheduleOwed(const Ctx& ctx) const
  {
    Position next = ctx.position;
    for (const SovietArrival& arrival : sovietSchedule()) {
      if (ctx.position.clock().turn == arrival.turn) {
        Owed& owe = stateOf(next).owed[arrival.area];
        owe.rifles += arrival.rifles;
        owe.armour += arrival.armour;
      }
    }
    return next;
  }

  std::optional<HexIndex>
  PggArrivals::provisionalHex(const Ctx& ctx, Area area) const
  {
    const SideId soviet = facts_.soviet();
    const auto openP = [&](HexIndex hex) {
      return !facts_.lakeP(hex) && !Units::enemyAtP(ctx, hex, soviet) && !zoc_.enemyZocP(ctx, hex, soviet);
    };
    const HexIndex entrance = facts_.area(area).front();
    if (openP(entrance)) {
      return entrance;
    }
    std::optional<HexIndex> best;
    for (std::size_t h = 0; h < ctx.board.hexCount(); ++h) {
      const HexIndex hex{static_cast<std::uint32_t>(h)};
      if (facts_.column(hex) <= facts_.column(entrance) || !openP(hex)) {
        continue;
      }
      if (!best || ctx.board.distance(entrance, hex) < ctx.board.distance(entrance, *best)) {
        best = hex;  // 14.12: the nearest open hex toward the east edge
      }
    }
    return best;
  }

  Position
  PggArrivals::rollProvisional(const Ctx& ctx, HexEngine::PrngStreams& streams, HexEngine::EventSink& sink) const
  {
    const int die = HexEngine::rollDie(streams, HexEngine::StreamTag::Setup, 6);
    sink.onEvent(HexEngine::DieRolled{HexEngine::StreamTag::Setup, die});
    const Area area = kProvisional[static_cast<std::size_t>(die - 1)];
    Position next = ctx.position;
    const std::optional<HexIndex> hex = provisionalHex(ctx, area);
    const std::optional<UnitId> unit = hex ? draw(ctx, Arm::SovietRifle, streams, sink) : std::nullopt;
    if (!unit) {
      sink.onEvent(HexEngine::GameEvent{"provisional-none", "area " + std::string(areaName(area)) + ": no open hex or counter"});
      return next;
    }
    enter(ctx, next, *unit, *hex, sink);
    next.state(*unit).flags.revealedP = false;
    return next;
  }

  Position
  PggArrivals::ageArrivals(const Ctx& ctx) const
  {
    Position next = ctx.position;
    for (SpaceId box : {facts_.germanArrivals(), facts_.sovietArrivals()}) {
      for (UnitId unit : ctx.position.unitsIn(box)) {
        std::optional<int>& delay = next.state(unit).delay;
        if (delay && 0 < *delay) {
          *delay -= 1;
        }
      }
    }
    return next;
  }

}  // namespace Pgg
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------

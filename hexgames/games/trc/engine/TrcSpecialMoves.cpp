// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
#include "TrcSpecialMoves.h"

#include "TrcFlags.h"
#include "TrcUnits.h"

#include <array>
#include <stdexcept>

namespace Trc {

  namespace {

    constexpr std::array<std::string_view, 2> kClaims{"zoc-av-exception", "av-russian-date"};

    const std::array<std::string_view, 4> kBalticPorts{"Riga", "Tallinn", "Helsinki", "Leningrad"};
    const std::array<std::string_view, 3> kBlackSeaPorts{"Odessa", "Sevastopol", "Rostov"};

    [[noreturn]] void
    refuse(const std::string& why)
    {
      throw std::invalid_argument("TrcSpecialMoves: " + why);
    }

    std::string_view
    areaName(SeaArea area)
    {
      switch (area) {
        case SeaArea::Baltic:
          return "baltic";
        case SeaArea::BlackSea:
          return "black-sea";
        case SeaArea::Caspian:
          return "caspian";
      }
      throw std::invalid_argument("Trc::areaName: sea area outside the three");
    }

  }  // namespace

  TrcSpecialMoves::TrcSpecialMoves(const TrcFacts& facts, const TrcZoc& zoc, const TrcWeather& weather,
                                   const TrcCombat& combat)
    : facts_(facts), zoc_(zoc), weather_(weather), combat_(combat)
  {
  }

  std::span<const std::string_view>
  TrcSpecialMoves::claims()
  {
    return kClaims;
  }

  std::vector<UnitId>
  TrcSpecialMoves::unitsOf(const Ctx& ctx, const std::string& list) const
  {
    std::vector<UnitId> out;
    std::string current;
    for (char c : list + " ") {
      if (' ' == c) {
        if (!current.empty()) {
          out.push_back(facts_.counter(current));
        }
        current.clear();
      } else {
        current += c;
      }
    }
    const std::optional<SideId> acting = ctx.position.clock().actingSide;
    for (UnitId unit : out) {
      if (acting != ctx.roster.unit(unit).side) {
        refuse("counter '" + ctx.roster.unit(unit).counter.text + "' does not belong to the acting side");
      }
    }
    if (out.empty()) {
      refuse("the command names no unit");
    }
    return out;
  }

  int
  TrcSpecialMoves::portsHeld(const Ctx& ctx, SideId side, SeaArea area) const
  {
    int held = 0;
    const auto count = [&](std::string_view port) {
      held += side == ctx.position.control(facts_.named(port)) ? 1 : 0;
      return;
    };
    if (SeaArea::Baltic == area) {
      for (std::string_view port : kBalticPorts) {
        count(port);
      }
    } else {
      for (std::string_view port : kBlackSeaPorts) {
        count(port);
      }
    }
    return held;
  }

  Position
  TrcSpecialMoves::seaMove(const Ctx& ctx, const HexEngine::GameCommand& command, HexEngine::PrngStreams& streams,
                           HexEngine::EventSink& sink) const
  {
    if (2 != command.args.size()) {
      refuse("sea-move takes units and to");
    }
    const std::vector<UnitId> units = unitsOf(ctx, command.args[0]);
    if (1 != units.size()) {
      refuse("sea movement carries one unit per side per sea area per turn (10.0)");
    }
    const UnitId unit = units.front();
    const SideId side = ctx.roster.unit(unit).side;
    const PhaseInfo phase = facts_.phase(ctx.position.clock().phase);
    if (PhaseKind::Move != phase.kind || Impulse::First != phase.impulse || ctx.position.unit(unit).flags.movedP) {
      refuse("sea movement is a first-impulse move of an unmoved unit (10.0)");
    }
    const bool fromBoxP = Units::inSpaceP(ctx.position, unit, facts_.omb());
    const std::optional<HexIndex> from = Units::hexOf(ctx.position, unit);
    const bool toBoxP = "omb" == command.args[1];
    const std::optional<HexIndex> to =
        toBoxP ? std::nullopt : std::optional<HexIndex>(ctx.board.indexOf(HexCoord::HexId{command.args[1]}));
    if (!fromBoxP && !from) {
      refuse("the unit is neither on the map nor in the Off-Map Units Box");
    }
    const auto friendlyPortP = [&](HexIndex hex) {
      return facts_.seaAreaOfPort(hex).has_value() && side == ctx.position.control(hex);
    };

    // The sea area, and whether this is a transfer, an invasion or an evacuation.
    std::optional<SeaArea> area;
    bool evacuationP = false;
    bool invasionP = false;
    if (to) {
      std::vector<SeaArea> seas = facts_.seasTouching(*to);
      if (seas.empty()) {
        refuse("hex '" + command.args[1] + "' borders no sea");
      }
      area = seas.front();
      invasionP = !friendlyPortP(*to);
      if (invasionP && Units::enemyAtP(ctx, *to, side)) {
        refuse("an invasion may not land on an enemy-occupied hex (10.0)");
      }
    }
    if (from) {
      const std::vector<SeaArea> seas = facts_.seasTouching(*from);
      if (seas.empty() || (area && seas.end() == std::find(seas.begin(), seas.end(), *area))) {
        refuse("the unit's hex does not border the destination's sea (10.0)");
      }
      area = area.value_or(seas.front());
      evacuationP = !friendlyPortP(*from);
      if (evacuationP && invasionP) {
        refuse("a sea move is port to port, a landing from a port, or an evacuation to a port (10.0)");
      }
    }
    if (toBoxP && !evacuationP && fromBoxP) {
      refuse("the unit is already in the Off-Map Units Box");
    }
    if (SeaArea::Caspian == *area) {
      refuse("there is no sea movement on the Caspian (10.0)");
    }
    if (SeaArea::Baltic == *area && invasionP) {
      refuse("there are no invasions in the Baltic (the sheet's Baltic panel)");
    }
    const std::string areaText(areaName(*area));
    if (Flags::listedP(ctx.position, side, Flags::kSeaUsed, areaText)) {
      refuse("the side has already moved by sea in the " + areaText + " this turn (10.0)");
    }

    const int die = HexEngine::rollDie(streams, HexEngine::StreamTag::SeaMove, 6);
    const bool russianBalticP = facts_.russian() == side && SeaArea::Baltic == *area &&
                                facts_.axis() == ctx.position.control(facts_.named("Leningrad"));
    const int modified = die - portsHeld(ctx, side, *area) + (evacuationP ? 1 : 0) + (russianBalticP ? 2 : 0);
    const int needed = SeaArea::Baltic == *area ? 2 : 3;
    Position next = ctx.position;
    Flags::add(next, side, Flags::kSeaUsed, areaText);
    sink.onEvent(HexEngine::DieRolled{HexEngine::StreamTag::SeaMove, die});
    const bool successP = needed >= modified;
    sink.onEvent(HexEngine::GameEvent{"sea-move", areaText + " roll " + std::to_string(modified) + (successP ? " succeeds" : " fails")});
    if (!successP) {
      Units::remove(next, unit, facts_.pool(side), sink);
      return next;
    }
    if (to) {
      Units::place(next, unit, *to, sink);
    } else {
      next.place(unit, facts_.omb());
      sink.onEvent(HexEngine::UnitPlaced{unit, facts_.omb()});
    }
    next.state(unit).flags.movedP = true;
    if (invasionP) {
      Flags::mark(next, unit, Flags::kInvaded);
    }
    return next;
  }

  Position
  TrcSpecialMoves::paradrop(const Ctx& ctx, const HexEngine::GameCommand& command, HexEngine::EventSink& sink) const
  {
    if (2 != command.args.size()) {
      refuse("paradrop takes units and to");
    }
    const std::vector<UnitId> units = unitsOf(ctx, command.args[0]);
    const HexIndex to = ctx.board.indexOf(HexCoord::HexId{command.args[1]});
    const PhaseInfo phase = facts_.phase(ctx.position.clock().phase);
    if (facts_.russian() != phase.side || PhaseKind::Move != phase.kind || Impulse::First != phase.impulse ||
        Weather::Snow != weather_.current(ctx.position)) {
      refuse("paradrops are made in the Russian first-impulse movement phase of snow turns (18.0)");
    }
    const std::optional<HexIndex> stavka = Units::hexOf(ctx.position, facts_.stavka());
    if (!stavka || !Units::withinP(ctx, *stavka, to, 8)) {
      refuse("the drop hex is not within eight hexes of Stavka (18.0)");
    }
    const std::string& terrain = ctx.rules.hexTerrain()[ctx.board.terrain(to).value].id;
    if (facts_.waterP(to) || "woods" == terrain || "mountain-hex" == terrain || Units::enemyAtP(ctx, to, facts_.russian()) ||
        zoc_.enemyZocP(ctx, to, facts_.russian(), true)) {
      refuse("paratroops may not drop into an enemy zone, woods, mountains or water (18.0)");
    }
    Position next = ctx.position;
    for (UnitId unit : units) {
      if (!facts_.typeP(unit, "paratroop") || Flags::markedP(ctx.position, unit, Flags::kDropped)) {
        refuse("counter '" + ctx.roster.unit(unit).counter.text + "' is not a paratroop corps that has yet to drop");
      }
      Units::place(next, unit, to, sink);
      next.state(unit).flags.movedP = true;
      Flags::mark(next, unit, Flags::kDropped);
    }
    return next;
  }

  // 16.1: the stop on entering an enemy zone does not apply "during" an automatic victory; in this
  // engine a zone of control is always read from the live position, so the defenders' zones are
  // gone the moment they are, and the vacated hex and the hexes around it are open to every other
  // unit that has yet to move this impulse.
  Position
  TrcSpecialMoves::automaticVictory(const Ctx& ctx, const HexEngine::GameCommand& command,
                                    HexEngine::EventSink& sink) const
  {
    if (2 != command.args.size()) {
      refuse("av-attack takes units and target");
    }
    const std::vector<UnitId> attackers = unitsOf(ctx, command.args[0]);
    const HexIndex target = ctx.board.indexOf(HexCoord::HexId{command.args[1]});
    const SideId side = ctx.roster.unit(attackers.front()).side;
    const PhaseInfo phase = facts_.phase(ctx.position.clock().phase);
    if (PhaseKind::Move != phase.kind) {
      refuse("an automatic victory is declared in a movement phase (16.1)");
    }
    if (facts_.russian() == side && 10 > ctx.position.clock().turn) {
      refuse("the Russians may not make automatic-victory attacks before November/December 1942 (16.4)");
    }
    std::vector<UnitId> defenders;
    for (UnitId unit : ctx.position.unitsAt(target)) {
      if (side != ctx.roster.unit(unit).side) {
        defenders.push_back(unit);
      }
    }
    if (defenders.empty()) {
      refuse("hex '" + command.args[1] + "' holds no enemy unit");
    }
    for (UnitId unit : attackers) {
      const std::optional<HexIndex> hex = Units::hexOf(ctx.position, unit);
      if (!hex || 1 != ctx.board.distance(*hex, target) || Flags::markedP(ctx.position, unit, Flags::kAv)) {
        refuse("counter '" + ctx.roster.unit(unit).counter.text + "' is not adjacent to the target or has fought an automatic victory this impulse");
      }
    }
    const TrcCombat::Factors power = combat_.factors(ctx, attackers, target, defenders);
    if (power.attack.value < 10 * power.defence.value) {
      refuse(std::to_string(power.attack.value) + " against " + std::to_string(power.defence.value) +
             " is short of the 10-1 an automatic victory needs (16.1)");
    }
    Position next = ctx.position;
    sink.onEvent(HexEngine::GameEvent{"automatic-victory", ctx.board.id(target).text + " " +
                                                                std::to_string(power.attack.value) + "-" +
                                                                std::to_string(power.defence.value)});
    for (UnitId unit : defenders) {
      Units::remove(next, unit, facts_.pool(ctx.roster.unit(unit).side), sink);
    }
    for (UnitId unit : attackers) {
      Flags::mark(next, unit, Flags::kAv);
      if (Impulse::First == phase.impulse) {
        Flags::mark(next, unit, Flags::kAvFirst);
      }
    }
    return next;
  }

}  // namespace Trc
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------

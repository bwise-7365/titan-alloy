// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
#include "TrcArrivals.h"

#include "TrcFlags.h"
#include "TrcUnits.h"

#include <array>
#include <stdexcept>

namespace Trc {

  namespace {

    constexpr std::array<std::string_view, 6> kClaims{"reinforcement-schedule", "omb-diversion", "south-edge-entry",
                                                      "axis-replacements", "russian-replacements", "partisan-placement"};

    const std::string kReplacedArmour = "replaced-armour";
    const std::string kReplacedGuards = "replaced-guards";

    [[noreturn]] void
    refuse(const Ctx& ctx, UnitId unit, const std::string& why)
    {
      throw std::invalid_argument("TrcArrivals: counter '" + ctx.roster.unit(unit).counter.text + "' " + why);
    }

  }  // namespace

  TrcArrivals::TrcArrivals(const TrcFacts& facts, const TrcZoc& zoc, const TrcWeather& weather)
    : facts_(facts), zoc_(zoc), weather_(weather)
  {
  }

  std::span<const std::string_view>
  TrcArrivals::claims()
  {
    return kClaims;
  }

  Position
  TrcArrivals::place(const Ctx& ctx, const HexEngine::Place& command, HexEngine::EventSink& sink) const
  {
    const UnitId unit = command.unit;
    const SideId side = ctx.roster.unit(unit).side;
    const PhaseInfo phase = facts_.phase(ctx.position.clock().phase);
    if (facts_.typeP(unit, "partisan")) {
      if (!std::holds_alternative<HexIndex>(command.where)) {
        refuse(ctx, unit, "is a partisan and is placed on a hex (19.1)");
      }
      return placePartisan(ctx, unit, std::get<HexIndex>(command.where), sink);
    }
    if (side != ctx.position.clock().actingSide || PhaseKind::Move != phase.kind) {
      refuse(ctx, unit, "may enter or leave the map only in its own side's movement phase (20.0)");
    }
    if (std::holds_alternative<SpaceId>(command.where)) {
      if (facts_.omb() != std::get<SpaceId>(command.where)) {
        refuse(ctx, unit, "may be moved only into the Off-Map Units Box (20.1)");
      }
      return divert(ctx, unit, sink);
    }
    const HexIndex hex = std::get<HexIndex>(command.where);
    if (Units::inSpaceP(ctx.position, unit, facts_.omb())) {
      return enter(ctx, unit, hex, sink);
    }
    if (Units::inSpaceP(ctx.position, unit, facts_.pool(side))) {
      return replace(ctx, unit, hex, sink);
    }
    refuse(ctx, unit, "is neither waiting in the Off-Map Units Box nor in its replacement pool");
  }

  void
  TrcArrivals::checkEntryHex(const Ctx& ctx, SideId side, HexIndex hex) const
  {
    const bool cityP = facts_.cityP(hex) && side == ctx.position.control(hex);
    if (!cityP && !facts_.ownEdgeP(side, hex)) {
      throw std::invalid_argument("TrcArrivals: hex '" + ctx.board.id(hex).text +
                                   "' is neither a friendly city nor on the side's own board edge (20.4)");
    }
    if (facts_.waterP(hex) || Units::enemyAtP(ctx, hex, side)) {
      throw std::invalid_argument("TrcArrivals: hex '" + ctx.board.id(hex).text + "' cannot be entered");
    }
    return;
  }

  Position
  TrcArrivals::enter(const Ctx& ctx, UnitId unit, HexIndex hex, HexEngine::EventSink& sink) const
  {
    const SideId side = ctx.roster.unit(unit).side;
    if (0 < ctx.position.unit(unit).delay.value_or(0)) {
      refuse(ctx, unit, "has not arrived yet (its delay is " + std::to_string(*ctx.position.unit(unit).delay) + ")");
    }
    checkEntryHex(ctx, side, hex);
    Position next = ctx.position;
    const bool southP = facts_.russian() == side && facts_.southEntryP(hex) &&
                        !(facts_.cityP(hex) && side == ctx.position.control(hex));
    if (southP) {
      if (0 < Flags::counter(ctx.position, side, Flags::kSouthEntry)) {
        refuse(ctx, unit, "cannot enter from the south edge: one Russian unit per turn does (20.5)");
      }
      Flags::bump(next, side, Flags::kSouthEntry, 1);
    } else if (facts_.ownEdgeP(side, hex) && facts_.russian() == side && ctx.board.id(hex).text.starts_with("QQ")) {
      refuse(ctx, unit, "may enter the south edge only between QQ5 and QQ16 (20.5)");
    }
    next.state(unit).delay = std::nullopt;
    Units::place(next, unit, hex, sink);
    return next;
  }

  Position
  TrcArrivals::divert(const Ctx& ctx, UnitId unit, HexEngine::EventSink& sink) const
  {
    const SideId side = ctx.roster.unit(unit).side;
    const std::optional<HexIndex> hex = Units::hexOf(ctx.position, unit);
    if (!hex || !facts_.railHexP(*hex) || !facts_.ownEdgeP(side, *hex)) {
      refuse(ctx, unit, "is not on a rail hex of its own board edge, so cannot rail into the box (20.1)");
    }
    if (Impulse::First != facts_.phase(ctx.position.clock().phase).impulse || ctx.position.unit(unit).flags.movedP) {
      refuse(ctx, unit, "can rail into the box only as its first-impulse move (9.1)");
    }
    const int capacity = TrcMovement::railCapacity(facts_.axis() == side, weather_.current(ctx.position));
    if (capacity <= Flags::counter(ctx.position, side, Flags::kRailMoves)) {
      refuse(ctx, unit, "cannot rail: the side's rail capacity is spent (9.2)");
    }
    Position next = ctx.position;
    Flags::bump(next, side, Flags::kRailMoves, 1);
    next.place(unit, facts_.omb());
    next.state(unit).flags.movedP = true;
    sink.onEvent(HexEngine::UnitPlaced{unit, facts_.omb()});
    return next;
  }

  std::string
  TrcArrivals::axisCategory(const Ctx& ctx, UnitId unit) const
  {
    const UnitSpec& spec = ctx.roster.unit(unit);
    const Nation nation = facts_.nation(unit);
    if (Nation::German != nation) {
      return std::string(nationName(nation));  // one of each minor ally
    }
    if ("ss" == spec.nationality || "luftwaffe" == spec.nationality || facts_.typeP(unit, "hq")) {
      return "";  // every one of them
    }
    if (facts_.typeP(unit, "armour")) {
      return "armour";
    }
    if (facts_.typeP(unit, "mountain") || facts_.typeP(unit, "motorized")) {
      return facts_.typeOf(unit);
    }
    const std::string factors = std::to_string(spec.front.attack->value) + "-" +
                                std::to_string(std::get<MovementPoints>(*spec.front.allowance).halves / 2);
    if (facts_.typeP(unit, "infantry") && ("3-4" == factors || "4-4" == factors || "5-4" == factors)) {
      return "infantry-" + factors;
    }
    refuse(ctx, unit, "is not on the Axis replacement list (21.0)");
  }

  Position
  TrcArrivals::replace(const Ctx& ctx, UnitId unit, HexIndex hex, HexEngine::EventSink& sink) const
  {
    const SideId side = ctx.roster.unit(unit).side;
    const int turn = ctx.position.clock().turn;
    const bool secondP = Impulse::Second == facts_.phase(ctx.position.clock().phase).impulse;
    checkEntryHex(ctx, side, hex);
    Position next = ctx.position;
    if (facts_.axis() == side) {
      if (7 != turn && 13 != turn && 19 != turn) {
        refuse(ctx, unit, "cannot be replaced: Axis replacements come on turns 7, 13 and 19 only (21.0)");
      }
      if (secondP && !facts_.typeP(unit, "hq")) {
        refuse(ctx, unit, "cannot be replaced in the second impulse, which takes HQs only (8.4)");
      }
      const std::string category = axisCategory(ctx, unit);
      if ("armour" == category) {
        int wells = 0;
        for (HexIndex well : facts_.oilWells()) {
          wells += facts_.axis() == ctx.position.control(well) ? 1 : 0;
        }
        if (wells <= Flags::counter(ctx.position, side, kReplacedArmour)) {
          refuse(ctx, unit, "exceeds one German armour corps per oil well held (21.0)");
        }
        Flags::bump(next, side, kReplacedArmour, 1);
      } else if (!category.empty()) {
        if (Flags::listedP(ctx.position, side, Flags::kReplaced, category)) {
          refuse(ctx, unit, "repeats the replacement category '" + category + "' (21.0)");
        }
        Flags::add(next, side, Flags::kReplaced, category);
      }
    } else {
      if (secondP && facts_.stavka() != unit) {
        refuse(ctx, unit, "cannot be replaced in the second impulse, which takes Stavka only (8.4)");
      }
      const int cost = ctx.roster.unit(unit).front.attack->value;
      const int points = Flags::counter(ctx.position, side, Flags::kReplacementPoints);
      if (points < cost) {
        refuse(ctx, unit, "costs " + std::to_string(cost) + " replacement points and " + std::to_string(points) +
                              " are left (22.0)");
      }
      const int cap = 17 <= turn ? 2 : 1;
      if (facts_.typeP(unit, "armour")) {
        if (cap <= Flags::counter(ctx.position, side, kReplacedArmour)) {
          refuse(ctx, unit, "exceeds the armour replacements allowed this turn (22.0)");
        }
        Flags::bump(next, side, kReplacedArmour, 1);
      }
      if ("guards" == ctx.roster.unit(unit).nationality) {
        if (cap <= Flags::counter(ctx.position, side, kReplacedGuards)) {
          refuse(ctx, unit, "exceeds the Guards replacements allowed this turn (22.0)");
        }
        Flags::bump(next, side, kReplacedGuards, 1);
      }
      Flags::bump(next, side, Flags::kReplacementPoints, -cost);
    }
    Units::place(next, unit, hex, sink);
    return next;
  }

  Position
  TrcArrivals::grantReplacementPoints(const Ctx& ctx, HexEngine::PrngStreams& streams, HexEngine::EventSink& sink) const
  {
    const int turn = ctx.position.clock().turn;
    int workers = 0;
    for (UnitId unit : Units::onMap(ctx, facts_.russian())) {
      if (facts_.typeP(unit, "worker")) {
        workers += ctx.roster.unit(unit).front.attack->value;
      }
    }
    int points = 13 <= turn ? 2 * workers : workers;
    Position next = ctx.position;
    if (5 <= turn) {
      // TODO(decide): no stream tag names Archangel's die; it rolls on the Setup stream.
      const int archangel = HexEngine::rollDie(streams, HexEngine::StreamTag::Setup, 6);
      sink.onEvent(HexEngine::DieRolled{HexEngine::StreamTag::Setup, archangel});
      points += archangel;
    }
    next.setFlag(facts_.russian(), Flags::kReplacementPoints, std::to_string(points));
    sink.onEvent(HexEngine::GameEvent{"replacement-points", std::to_string(points)});
    return next;
  }

  Position
  TrcArrivals::placePartisan(const Ctx& ctx, UnitId unit, HexIndex hex, HexEngine::EventSink& sink) const
  {
    const PhaseInfo phase = facts_.phase(ctx.position.clock().phase);
    if (PhaseKind::End != phase.kind || facts_.russian() != phase.side) {
      refuse(ctx, unit, "is placed in the Russian end phase, after the second impulse (19.4)");
    }
    if (ctx.position.unit(unit).where.has_value()) {
      refuse(ctx, unit, "is already in play; partisans are placed only once removed (19.4)");
    }
    const bool axisCityP = facts_.cityP(hex) && facts_.axis() == ctx.position.control(hex);
    bool axisRailP = false;
    for (std::size_t link : ctx.board.network(facts_.railNetwork()).linksAt(hex)) {
      axisRailP = axisRailP || facts_.axis() == ctx.position.linkOwner(facts_.railNetwork(), link);
    }
    if (!facts_.inCountryP(hex, "russia") || (!axisCityP && !axisRailP)) {
      refuse(ctx, unit, "must go in Russia on an Axis city or Axis rail hex (19.1)");
    }
    if (zoc_.enemyZocP(ctx, hex, facts_.russian(), false)) {
      refuse(ctx, unit, "may not be placed in an Axis zone of control (19.1)");
    }
    for (UnitId axisUnit : Units::onMap(ctx, facts_.axis())) {
      if ("ss" == ctx.roster.unit(axisUnit).nationality &&
          Units::withinP(ctx, *Units::hexOf(ctx.position, axisUnit), hex, 5)) {
        refuse(ctx, unit, "may not be placed within five hexes of an SS unit (19.1)");
      }
    }
    Position next = ctx.position;
    Units::place(next, unit, hex, sink);
    return next;
  }

}  // namespace Trc
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------

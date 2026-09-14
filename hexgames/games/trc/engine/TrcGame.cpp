// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
// TrcGame: check, apply and settle. The phase boundaries are in TrcGamePhases.cpp.
// ----------------------------------------------
#include "TrcGame.h"

#include "TrcFlags.h"
#include "TrcUnits.h"

#include <array>
#include <stdexcept>

namespace Trc {

  namespace {

    constexpr std::array<std::string_view, 1> kClaims{"order-railheads-before-supply"};

    Ctx
    at(const Ctx& ctx, const Position& position)
    {
      return Ctx{ctx.board, ctx.rules, ctx.roster, position};
    }

  }  // namespace

  TrcGame::TrcGame(TrcParts parts) : parts_(parts)
  {
  }

  std::span<const std::string_view>
  TrcGame::claims() const
  {
    return kClaims;
  }

  void
  TrcGame::checkMove(const Ctx& ctx, const HexEngine::MoveUnit& move) const
  {
    if (parts_.facts.railMode() == move.mode &&
        Impulse::First != parts_.facts.phase(ctx.position.clock().phase).impulse) {
      throw std::invalid_argument("TrcGame: rail movement is made in the first impulse only (9.1)");
    }
    if (parts_.facts.railMode() != move.mode && parts_.facts.normalMode() != move.mode) {
      throw std::invalid_argument("TrcGame: sea movement and paradrops are the sea-move and paradrop commands");
    }
    return;
  }

  void
  TrcGame::checkAttack(const Ctx& ctx, const HexEngine::DeclareAttack& attack) const
  {
    for (UnitId unit : attack.attackers) {
      if (!parts_.mandatory.mayAttackP(ctx, unit)) {
        throw std::invalid_argument("TrcGame: counter '" + ctx.roster.unit(unit).counter.text +
                                     "' has attacked, moved by rail or won an automatic victory this impulse "
                                     "(9.1, 12.4, 16.2)");
      }
    }
    const SideId side = ctx.roster.unit(attack.attackers.front()).side;
    std::vector<UnitId> defenders;
    for (UnitId unit : ctx.position.unitsAt(attack.target)) {
      if (side == ctx.roster.unit(unit).side) {
        continue;
      }
      if (ctx.position.unit(unit).flags.defendedP) {
        throw std::invalid_argument("TrcGame: counter '" + ctx.roster.unit(unit).counter.text +
                                     "' has already been attacked this phase (12.4)");
      }
      defenders.push_back(unit);
    }
    parts_.air.check(ctx, attack);
    if (defenders.empty()) {
      return;  // the engine refuses an attack on an empty hex with its own reason
    }
    const TrcCombat::Factors power = parts_.combat.factors(ctx, attack.attackers, attack.target, defenders);
    if (std::holds_alternative<HexModel::BelowMinimum>(parts_.combat.odds(power))) {
      throw std::invalid_argument("TrcGame: " + std::to_string(power.attack.value) + " against " +
                                   std::to_string(power.defence.value) + " is worse than 1-6 (12.5)");
    }
    return;
  }

  void
  TrcGame::check(const Ctx& ctx, const HexEngine::Command& command) const
  {
    if (const HexEngine::MoveUnit* move = std::get_if<HexEngine::MoveUnit>(&command)) {
      checkMove(ctx, *move);
    } else if (const HexEngine::DeclareAttack* attack = std::get_if<HexEngine::DeclareAttack>(&command)) {
      checkAttack(ctx, *attack);
    } else if (std::holds_alternative<HexEngine::EndPhase>(command) &&
               PhaseKind::Combat == parts_.facts.phase(ctx.position.clock().phase).kind) {
      parts_.mandatory.check(ctx);
    }
    return;
  }

  Position
  TrcGame::apply(const Ctx& ctx, const HexEngine::Command& command, HexEngine::PrngStreams& streams,
                 HexEngine::EventSink& sink) const
  {
    if (const HexEngine::Place* place = std::get_if<HexEngine::Place>(&command)) {
      return parts_.arrivals.place(ctx, *place, sink);
    }
    if (const HexEngine::GameCommand* game = std::get_if<HexEngine::GameCommand>(&command)) {
      if ("sea-move" == game->verb) {
        return parts_.special.seaMove(ctx, *game, streams, sink);
      }
      if ("paradrop" == game->verb) {
        return parts_.special.paradrop(ctx, *game, sink);
      }
      if ("av-attack" == game->verb) {
        return parts_.special.automaticVictory(ctx, *game, sink);
      }
      throw std::invalid_argument("TrcGame: TRC has no command '" + game->verb + "'");
    }
    throw std::invalid_argument("TrcGame: the engine adjudicates this command itself");
  }

  Position
  TrcGame::settleMove(const Ctx& ctx, const HexEngine::MoveUnit& move) const
  {
    Position next = ctx.position;
    const SideId side = ctx.roster.unit(move.units.front()).side;
    if (parts_.facts.railMode() == move.mode) {
      Flags::bump(next, side, Flags::kRailMoves, static_cast<int>(move.units.size()));
      for (UnitId unit : move.units) {
        Flags::mark(next, unit, Flags::kRailed);
      }
    }
    next = parts_.rail.noteTouched(at(ctx, next), move);
    return parts_.politics.noteWarsaw(at(ctx, next), move);
  }

  Position
  TrcGame::repairStacking(const Ctx& ctx, HexEngine::EventSink& sink) const
  {
    Position next = ctx.position;
    for (std::size_t h = 0; h < ctx.board.hexCount(); ++h) {
      const HexIndex hex{static_cast<std::uint32_t>(h)};
      if (next.unitsAt(hex).empty()) {
        continue;
      }
      for (UnitId unit : parts_.stacking.excess(at(ctx, next), hex)) {
        Units::remove(next, unit, parts_.facts.pool(ctx.roster.unit(unit).side), sink);
      }
    }
    return next;
  }

  // 11.3: the impulse after a leader is lost. A leader is lost when he lands in his side's pool or
  // surrendered box (a leader never placed is not lost). "pending" becomes "active" when the
  // country's next movement phase opens and "spent" when it closes; a leader is lost once.
  Position
  TrcGame::noteLeaders(const Ctx& ctx) const
  {
    Position next = ctx.position;
    for (UnitId leader : {parts_.facts.hitler(), parts_.facts.stalin()}) {
      const SideId side = ctx.roster.unit(leader).side;
      const bool lostP = Units::inSpaceP(next, leader, parts_.facts.pool(side)) ||
                         Units::inSpaceP(next, leader, parts_.facts.surrendered(side));
      if (lostP && !next.flag(side, Flags::kLeaderLost)) {
        next.setFlag(side, Flags::kLeaderLost, "pending");
      }
    }
    return next;
  }

  // 22.2: a worker always surrenders rather than being eliminated.
  Position
  TrcGame::workersSurrender(const Ctx& ctx, HexEngine::EventSink& sink) const
  {
    Position next = ctx.position;
    const SideId russian = parts_.facts.russian();
    for (UnitId unit : ctx.position.unitsIn(parts_.facts.pool(russian))) {
      if (parts_.facts.typeP(unit, "worker")) {
        Units::remove(next, unit, parts_.facts.surrendered(russian), sink);
      }
    }
    return next;
  }

  Position
  TrcGame::settle(const Ctx& ctx, const HexEngine::Command& command, HexEngine::EventSink& sink) const
  {
    Position next = ctx.position;
    if (const HexEngine::MoveUnit* move = std::get_if<HexEngine::MoveUnit>(&command)) {
      next = settleMove(ctx, *move);
    }
    const HexEngine::DeclareAttack* attack = std::get_if<HexEngine::DeclareAttack>(&command);
    if (nullptr != attack && !attack->modifiers.empty()) {
      const SideId side = ctx.roster.unit(attack->attackers.front()).side;
      Flags::bump(next, side, Flags::kAirUsed, static_cast<int>(attack->modifiers.size()));
    }
    next = workersSurrender(at(ctx, next), sink);
    next = noteLeaders(at(ctx, next));
    if (next.combatPlan()) {
      return next;  // 13.3: nothing is re-evaluated until the battle's last answer
    }
    const bool battleOverP = std::holds_alternative<HexEngine::DeclareAttack>(command) ||
                             std::holds_alternative<HexEngine::DecisionAnswer>(command);
    if (battleOverP) {
      next = repairStacking(at(ctx, next), sink);  // 6.2: after each combat
    }
    return parts_.control.update(at(ctx, next), sink);
  }

}  // namespace Trc
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------

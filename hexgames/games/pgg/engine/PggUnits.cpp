// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
#include "PggUnits.h"

#include <stdexcept>

namespace Pgg::Units {

  namespace {

    void
    forget(PggState& state, SideId side, UnitId unit)
    {
      PggSideState& one = state.side(side);
      one.disrupted.erase(unit);
      one.unsupplied.erase(unit);
      one.beyondRadius.erase(unit);
      one.entered.erase(unit);
      one.spent.erase(unit);
      one.halted.erase(unit);
      one.continuing.erase(unit);
      one.retreatedOnto.erase(unit);
      one.zocEntry.erase(unit);
      return;
    }

    void
    renameIn(std::set<UnitId>& set, UnitId from, UnitId to)
    {
      if (0 != set.erase(from)) {
        set.insert(to);
      }
      return;
    }

    template <class V>
    void
    renameIn(std::map<UnitId, V>& map, UnitId from, UnitId to)
    {
      const auto found = map.find(from);
      if (map.end() != found) {
        map[to] = found->second;
        map.erase(from);
      }
      return;
    }

    // A German infantry division's second counter takes over the first's marks (9.61).
    void
    rename(PggState& state, SideId side, UnitId from, UnitId to)
    {
      PggSideState& one = state.side(side);
      for (std::set<UnitId>* set : {&one.disrupted, &one.unsupplied, &one.beyondRadius, &one.entered, &one.halted,
                                    &one.continuing, &one.retreatedOnto}) {
        renameIn(*set, from, to);
      }
      renameIn(one.spent, from, to);
      renameIn(one.zocEntry, from, to);
      return;
    }

  }  // namespace

  std::optional<HexIndex>
  hexOf(const Position& position, UnitId unit)
  {
    const std::optional<Location>& where = position.unit(unit).where;
    if (!where || !std::holds_alternative<HexIndex>(*where)) {
      return std::nullopt;
    }
    return std::get<HexIndex>(*where);
  }

  bool
  inSpaceP(const Position& position, UnitId unit, SpaceId space)
  {
    const std::optional<Location>& where = position.unit(unit).where;
    return where && std::holds_alternative<SpaceId>(*where) && space == std::get<SpaceId>(*where);
  }

  std::vector<UnitId>
  onMap(const Ctx& ctx, const PggFacts& facts, SideId side)
  {
    std::vector<UnitId> out;
    for (UnitId unit : ctx.roster.ofSide(side)) {
      if (!facts.markerP(unit) && hexOf(ctx.position, unit)) {
        out.push_back(unit);
      }
    }
    return out;
  }

  bool
  enemyAtP(const Ctx& ctx, HexIndex hex, SideId side)
  {
    for (UnitId occupant : ctx.position.unitsAt(hex)) {
      if (side != ctx.roster.unit(occupant).side) {
        return true;
      }
    }
    return false;
  }

  bool
  friendlyAtP(const Ctx& ctx, HexIndex hex, SideId side)
  {
    for (UnitId occupant : ctx.position.unitsAt(hex)) {
      if (side == ctx.roster.unit(occupant).side) {
        return true;
      }
    }
    return false;
  }

  const Strengths&
  face(const Ctx& ctx, UnitId unit)
  {
    const UnitSpec& spec = ctx.roster.unit(unit);
    if (HexModel::Face::Back == ctx.position.unit(unit).face) {
      if (!spec.back) {
        throw std::invalid_argument("Pgg::Units: counter '" + spec.counter.text + "' shows a back that prints no values");
      }
      return *spec.back;
    }
    return spec.front;
  }

  int
  attackOf(const Ctx& ctx, UnitId unit)
  {
    return face(ctx, unit).attack.value_or(Strength{0}).value;
  }

  int
  defenceOf(const Ctx& ctx, UnitId unit)
  {
    return face(ctx, unit).defence.value_or(Strength{0}).value;
  }

  int
  allowanceOf(const Ctx& ctx, UnitId unit)
  {
    const Strengths& shown = face(ctx, unit);
    if (!shown.allowance) {
      throw std::invalid_argument("Pgg::Units: counter '" + ctx.roster.unit(unit).counter.text +
                                  "' prints no movement allowance");
    }
    return std::get<MovementPoints>(*shown.allowance).halves / 2;
  }

  void
  place(Position& position, UnitId unit, HexIndex hex, HexEngine::EventSink& sink)
  {
    position.place(unit, hex);
    sink.onEvent(HexEngine::UnitPlaced{unit, hex});
    return;
  }

  void
  reveal(Position& position, UnitId unit, HexEngine::EventSink& sink)
  {
    position.state(unit).flags.revealedP = true;
    sink.onEvent(HexEngine::UnitRevealed{unit});
    return;
  }

  void
  eliminate(const PggFacts& facts, Position& position, UnitId unit, HexEngine::EventSink& sink)
  {
    const SideId side = facts.definition().roster->unit(unit).side;
    PggState& state = stateOf(position);
    forget(state, side, unit);
    if (facts.sovietDivisionP(unit)) {
      position.place(unit, facts.deadPile());
      position.state(unit).flags.revealedP = false;  // 12.4: the dead pile keeps them untried
      sink.onEvent(HexEngine::UnitEliminated{unit, facts.deadPile()});
      return;
    }
    position.remove(unit);
    if (facts.german() == side) {
      state.germanEliminated.insert(unit);
      if (const std::optional<UnitId> second = facts.successor(unit)) {
        state.germanEliminated.insert(*second);  // the division's last two steps go with it
      }
    }
    sink.onEvent(HexEngine::UnitEliminated{unit, std::nullopt});
    return;
  }

  void
  loseStep(const PggFacts& facts, Position& position, UnitId unit, HexEngine::EventSink& sink)
  {
    const SideId side = facts.definition().roster->unit(unit).side;
    if (facts.soviet() == side) {
      eliminate(facts, position, unit, sink);  // 9.62, 10.24: every Soviet unit is one step
      return;
    }
    HexModel::UnitState& state = position.state(unit);
    if (HexModel::Face::Front == state.face) {
      state.face = HexModel::Face::Back;
      sink.onEvent(HexEngine::UnitReduced{unit, facts.successor(unit) ? 3 : 1});
      return;
    }
    const std::optional<UnitId> second = facts.successor(unit);
    const std::optional<HexIndex> hex = hexOf(position, unit);
    if (!second || !hex) {
      eliminate(facts, position, unit, sink);
      return;
    }
    if (position.unit(*second).where) {
      throw std::invalid_argument("Pgg::Units: counter '" + facts.definition().roster->unit(*second).counter.text +
                                  "' is already in play, so it cannot replace its division's first counter");
    }
    const HexModel::UnitFlags flags = state.flags;
    position.remove(unit);
    position.place(*second, *hex);
    position.state(*second).flags = flags;
    position.state(*second).face = HexModel::Face::Front;
    rename(stateOf(position), side, unit, *second);
    sink.onEvent(HexEngine::UnitPlaced{*second, *hex});
    sink.onEvent(HexEngine::UnitReduced{*second, 2});
    return;
  }

}  // namespace Pgg::Units
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------

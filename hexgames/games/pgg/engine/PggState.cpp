// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
#include "PggState.h"

#include <stdexcept>

namespace Pgg {

  namespace {

    void
    appendUnits(std::string& s, const std::set<UnitId>& units)
    {
      for (UnitId unit : units) {
        s += std::to_string(unit.value) + ",";
      }
      s += ';';
      return;
    }

    void
    appendHexes(std::string& s, const auto& hexes)
    {
      for (HexIndex hex : hexes) {
        s += std::to_string(hex.value) + ",";
      }
      s += ';';
      return;
    }

    void
    appendSide(std::string& s, const PggSideState& side)
    {
      appendUnits(s, side.disrupted);
      appendUnits(s, side.unsupplied);
      appendUnits(s, side.beyondRadius);
      appendUnits(s, side.entered);
      for (const auto& [unit, halves] : side.spent) {
        s += std::to_string(unit.value) + ":" + std::to_string(halves) + ",";
      }
      s += ';';
      appendUnits(s, side.halted);
      appendUnits(s, side.continuing);
      appendUnits(s, side.retreatedOnto);
      for (const auto& [unit, hex] : side.zocEntry) {
        s += std::to_string(unit.value) + ":" + std::to_string(hex.value) + ",";
      }
      s += '|';
      return;
    }

  }  // namespace

  int
  armyNumber(Army army)
  {
    switch (army) {
      case Army::Thirteenth:
        return 13;
      case Army::Sixteenth:
        return 16;
      case Army::Nineteenth:
        return 19;
      case Army::Twentieth:
        return 20;
    }
    throw std::invalid_argument("Pgg::armyNumber: army outside 5.2");
  }

  Army
  armyNumbered(int number)
  {
    for (Army army : {Army::Thirteenth, Army::Sixteenth, Army::Nineteenth, Army::Twentieth}) {
      if (number == armyNumber(army)) {
        return army;
      }
    }
    throw std::invalid_argument("Pgg::armyNumbered: 5.2 names no army " + std::to_string(number));
  }

  PggState::PggState(std::size_t sides) : sides_(sides)
  {
  }

  const PggSideState&
  PggState::side(SideId id) const
  {
    if (sides_.size() <= id.value) {
      throw std::invalid_argument("PggState: side index " + std::to_string(id.value) + " outside the rules");
    }
    return sides_[id.value];
  }

  PggSideState&
  PggState::side(SideId id)
  {
    if (sides_.size() <= id.value) {
      throw std::invalid_argument("PggState: side index " + std::to_string(id.value) + " outside the rules");
    }
    return sides_[id.value];
  }

  void
  PggState::appendDigest(std::string& s) const
  {
    appendHexes(s, airInterdiction);
    appendHexes(s, railCuts);
    for (const auto& [hex, turn] : railRepaired) {
      s += std::to_string(hex.value) + ":" + std::to_string(turn) + ",";
    }
    s += ';';
    appendHexes(s, passedByGermans);
    s += (smolenskTaken ? std::to_string(*smolenskTaken) : std::string("-")) + ";";
    appendHexes(s, germanHeld);
    appendUnits(s, germanEliminated);
    s += (outcome ? std::to_string(*outcome) : std::string("-")) + ";";
    s += (sovietInterdiction ? std::to_string(sovietInterdiction->value) : std::string("-")) + ";";
    s += std::to_string(sovietInterdictionTurns) + ";" + std::to_string(swfUsed) + ";" + std::to_string(swfThisTurn) +
         ";" + std::to_string(railUnits) + ";" + std::to_string(recaptureVp) + ";";
    for (const auto& [area, owe] : owed) {
      s += std::string(areaName(area)) + ":" + std::to_string(owe.rifles) + ":" + std::to_string(owe.armour) + ",";
    }
    s += ';';
    for (const auto& [unit, army] : armies) {
      s += std::to_string(unit.value) + ":" + std::to_string(armyNumber(army)) + ",";
    }
    s += ';';
    for (Army army : frozen) {
      s += std::to_string(armyNumber(army)) + ",";
    }
    s += ';';
    for (const PggSideState& one : sides_) {
      appendSide(s, one);
    }
    return;
  }

  const PggState&
  stateOf(const Position& position)
  {
    return position.gameState<PggState>();
  }

  PggState&
  stateOf(Position& position)
  {
    return position.gameState<PggState>();
  }

}  // namespace Pgg
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------

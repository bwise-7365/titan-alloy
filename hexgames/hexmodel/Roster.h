// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
// The Roster: every counter that can be in a game, with its typed strengths. Immutable; built from
// the counters document through the package bindings and a game-supplied value-line reader.
// ----------------------------------------------
#pragma once
#include "hexmodel/Ids.h"
#include "hexmodel/Quantities.h"

#include <functional>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace HexModel {

  enum class UnitKind : std::uint8_t { Ground, Air, Naval, Hq, Leader, Marker };

  // Strengths read from one printed value line ("8-7", "3-3-1", "4-U", "Hero RD").
  struct Strengths {
    std::optional<Strength> attack;
    std::optional<Strength> defence;
    std::optional<Budget> allowance;  // movement, in the game's budget unit
    std::optional<int> range;         // nullopt: not printed; a large sentinel is never used for "U"
    bool unlimitedRangeP = false;
  };

  // Turns a counter's value-line text into Strengths; supplied per game. Throws
  // std::invalid_argument for text it does not understand, so every counter of a set is checked once.
  using ValueLineReader = std::function<Strengths(std::string_view valueLine, UnitKind)>;

  struct UnitSpec {
    UnitId id;
    CounterId counter;   // "g-ge-41-armour", with "#k" for the k-th copy
    UnitTypeId type;
    SideId side;
    UnitKind kind = UnitKind::Ground;
    std::string nationality;
    Strengths front;
    std::optional<Strengths> back;  // nullopt: the back is not a weaker face of this unit
    int maxSteps = 1;
    bool hiddenP = false;           // placed face down (Tarawa's Japanese)
  };

  class Roster {
  public:
    const std::vector<UnitSpec>& units() const { return units_; }  // insertion order == UnitId order
    const UnitSpec& unit(UnitId) const;
    std::optional<UnitId> find(const CounterId&) const;
    std::vector<UnitId> ofSide(SideId) const;

  private:
    friend class RosterBuilder;
    std::vector<UnitSpec> units_;
  };

}  // namespace HexModel
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------

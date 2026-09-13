// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
// Walking the rules document's phase tree. A stop is one leaf phase acting for one side; a node
// that names several sides repeats its whole subtree once per side, in the sides' declaration
// order (Dai Senso's faction turn). Nodes outside the turn's TurnSelector, and nodes the game's
// PhaseGate calls inactive, are skipped whole.
// ----------------------------------------------
#pragma once
#include "hexmodel/Ids.h"
#include "hexrules/RuleSet.h"

#include <functional>
#include <optional>
#include <vector>

namespace HexEngine {

  class PhaseCursor {
  public:
    struct Stop {
      int turn = 1;
      HexModel::PhaseId phase;
      std::optional<HexModel::SideId> side;
      bool operator==(const Stop&) const = default;
    };

    // The PhaseGate reduced to what the walk needs, so that the cursor itself needs no Position.
    using ActiveFilter = std::function<bool(HexModel::PhaseId)>;

    explicit PhaseCursor(const HexRules::RuleSet&);

    // Every stop of one turn, in document order. Empty when the turn has no active phase.
    std::vector<Stop> turnStops(int turn, const ActiveFilter&) const;
    // The first stop of `turn`, or of the first later turn that has one; throws after kTurnSearch
    // barren turns, which is a rules document with no reachable phase at all.
    Stop first(int turn, const ActiveFilter&) const;
    // The stop after `current`; walks into later turns. Throws when `current` is not a stop of its
    // own turn, which means the position and the rules document disagree.
    Stop next(const Stop& current, const ActiveFilter&) const;

    static constexpr int kTurnSearch = 64;

  private:
    const HexRules::RuleSet& rules_;
  };

}  // namespace HexEngine
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------

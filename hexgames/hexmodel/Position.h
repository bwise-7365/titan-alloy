// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
// The Position: everything that changes during play. A value type, cheap to copy, the only thing a
// rollout forks. All mutation goes through the few named mutators so the per-hex stacks, the control
// table and the unit table can never disagree.
// ----------------------------------------------
#pragma once
#include "hexmodel/GameState.h"
#include "hexmodel/Ids.h"
#include "hexmodel/Quantities.h"
#include "hexmodel/Resolution.h"

#include <cstdint>
#include <map>
#include <optional>
#include <string>
#include <variant>
#include <vector>

namespace HexModel {

  // A unit is always somewhere: on a hex or in an off-map space.
  using Location = std::variant<HexIndex, SpaceId>;

  enum class Face : std::uint8_t { Front, Back };

  struct UnitFlags {
    bool movedP = false;
    bool attackedP = false;
    bool defendedP = false;  // added in M6: was the target of a battle this phase (TRC 12.4)
    bool revealedP = true;   // false only for hidden units still face down
    bool disruptedP = false;
    bool isolatedP = false;  // out of supply at the last check
  };

  struct UnitState {
    std::optional<Location> where;  // nullopt: permanently out of the game
    Steps steps;
    Face face = Face::Front;
    UnitFlags flags;
    std::optional<int> delay;       // turns until a delayed pool returns it
    std::vector<std::string> markers;  // marker tokens as in hexsave unit/@status
  };

  struct TurnClock {
    int turn = 1;
    PhaseId phase;
    std::optional<SideId> actingSide;
  };

  // Mutable state of a region in a mutable layer (Dai Senso countries).
  struct RegionState {
    std::optional<std::string> status;
    std::optional<SideId> alignment;
    std::optional<std::string> posture;
    std::optional<SideId> owner;
  };

  class Position {
  public:
    // ---- units ---------------------------------------------------------------------------------
    const UnitState& unit(UnitId) const;
    std::size_t unitCount() const { return units_.size(); }
    // The stack on a hex, in insertion order (the order matters wherever a PRNG follows it).
    const std::vector<UnitId>& unitsAt(HexIndex) const;
    const std::vector<UnitId>& unitsIn(SpaceId) const;

    // The only mutators of location; they keep the stacks in step with the units.
    void place(UnitId, Location);
    void remove(UnitId);                 // out of the game for good
    UnitState& state(UnitId);            // steps, face, flags, delay, markers -- never `where`

    // ---- control and network / region / track state -----------------------------------------
    std::optional<SideId> control(HexIndex) const;   // last-toucher ownership, stored not derived
    void setControl(HexIndex, std::optional<SideId>);
    std::optional<SideId> linkOwner(NetworkId, std::size_t link) const;
    void setLinkOwner(NetworkId, std::size_t link, std::optional<SideId>);
    const RegionState& region(LayerId, RegionId) const;
    RegionState& region(LayerId, RegionId);
    int track(TrackId) const;
    void setTrack(TrackId, int);

    // ---- clock and decisions ----------------------------------------------------------------
    const TurnClock& clock() const { return clock_; }
    TurnClock& clock() { return clock_; }

    // ---- the resolution stack (M6b; see Resolution.h) ------------------------------------------
    // Bottom first, top last. top() and pop() throw std::invalid_argument when nothing is owed.
    const std::vector<Resolution>& resolution() const { return resolution_; }
    const Resolution& top() const;
    Resolution& top();
    void push(Obligation);  // the new top, asking nothing yet
    void pop();
    // The top entry's decision; NoDecision when nothing is owed or the top asks nothing.
    const PendingDecision& pending() const;
    // Sets the top entry's decision (NoDecision once it is answered); throws when nothing is owed.
    void ask(PendingDecision);

    // ---- the game's own state (M6b; see GameState.h) --------------------------------------------
    // gameState<T>() throws std::invalid_argument when no state is held or it is not a T.
    template <class T>
    const T&
    gameState() const
    {
      return game_.template as<T>("Position: the game state");
    }
    template <class T>
    T&
    gameState()
    {
      return game_.template as<T>("Position: the game state");
    }
    const Polymorphic<GameState>& heldGameState() const { return game_; }  // for a codec
    void setGameState(Polymorphic<GameState>);

    // A stable hash of the canonical serialisation, for determinism tests.
    std::uint64_t digest() const;

  private:
    friend class PositionBuilder;
    void removeFromStack(Location, UnitId);
    void addToStack(Location, UnitId);

    std::vector<UnitState> units_;
    std::vector<std::vector<UnitId>> byHex_;
    std::vector<std::vector<UnitId>> bySpace_;
    std::vector<std::optional<SideId>> control_;
    std::vector<std::vector<std::optional<SideId>>> linkOwners_;
    std::vector<std::vector<RegionState>> regions_;
    std::vector<int> tracks_;
    TurnClock clock_;
    std::vector<Resolution> resolution_;
    Polymorphic<GameState> game_;
  };

}  // namespace HexModel
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------

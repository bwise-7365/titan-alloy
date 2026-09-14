// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
// The Position: everything that changes during play. A value type, cheap to copy, the only thing a
// rollout forks. All mutation goes through the few named mutators so the per-hex stacks, the control
// table and the unit table can never disagree.
// ----------------------------------------------
#pragma once
#include "hexmodel/Ids.h"
#include "hexmodel/Quantities.h"

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

  // A choice the rules require before adjudication can continue; the engine accepts only the
  // matching answer next. Games extend GameChoice with their own vocabularies. Since M6 a combat
  // decision names the side that answers it, which is not always the side whose turn it is (TRC
  // 13.3: the defender picks his own loss, the attacker routes every retreat).
  struct NoDecision {};
  struct ChooseLoss { SideId side; std::vector<UnitId> candidates; int count; };
  struct ChooseRetreat { SideId side; UnitId unit; std::vector<HexIndex> candidates; bool mayStopP = false; };
  struct ChooseCard { RandomizerId deck; std::vector<std::string> candidates; };
  struct GameChoice { std::string verb; std::vector<std::string> options; };
  using PendingDecision = std::variant<NoDecision, ChooseLoss, ChooseRetreat, ChooseCard, GameChoice>;

  // Added in M6: the rest of a battle, in the order the rules settle it. The head of `steps` is
  // what the current PendingDecision is about; when it is answered the adjudicator works down the
  // list until it needs another answer or the list is empty.
  struct OwedLoss { SideId side; int count; };                // steps a side still has to lose
  struct OwedRetreat { SideId side; int fewest; int most; };  // every unit of a side retreats
  struct UnitRetreat {                                        // one unit's walk, part-way through
    UnitId unit;
    HexIndex from;
    int fewest;
    int most;
    std::vector<HexIndex> path;
  };
  using CombatStep = std::variant<OwedLoss, OwedRetreat, UnitRetreat>;
  struct CombatPlan {
    std::optional<SideId> router;  // who routes retreats; nullopt: each unit's owner
    std::vector<UnitId> involved;
    std::vector<CombatStep> steps;
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
    const PendingDecision& pending() const { return pending_; }
    void setPending(PendingDecision);
    const std::optional<CombatPlan>& combatPlan() const { return plan_; }
    void setCombatPlan(std::optional<CombatPlan>);

    // ---- side flags (hexsave sides/side/flag), added in M6 ---------------------------------------
    // Named per-side values a game keeps between commands (TRC's weather and weather DRM). nullopt:
    // the flag is not set. Throws for a side outside the position.
    std::optional<std::string> flag(SideId, const std::string& name) const;
    void setFlag(SideId, const std::string& name, std::optional<std::string> value);
    const std::map<std::string, std::string>& flags(SideId) const;

    // A stable hash of the canonical serialisation, for determinism tests.
    std::uint64_t digest() const;

  private:
    friend class PositionBuilder;
    void removeFromStack(Location, UnitId);
    void addToStack(Location, UnitId);
    std::map<std::string, std::string>& flagsOf(SideId);

    std::vector<UnitState> units_;
    std::vector<std::vector<UnitId>> byHex_;
    std::vector<std::vector<UnitId>> bySpace_;
    std::vector<std::optional<SideId>> control_;
    std::vector<std::vector<std::optional<SideId>>> linkOwners_;
    std::vector<std::vector<RegionState>> regions_;
    std::vector<int> tracks_;
    TurnClock clock_;
    PendingDecision pending_;
    std::optional<CombatPlan> plan_;
    std::vector<std::map<std::string, std::string>> flags_;  // one ordered map per side
  };

}  // namespace HexModel
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------

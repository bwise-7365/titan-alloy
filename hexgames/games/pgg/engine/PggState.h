// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
// PGG's own state between commands, as real types: each side's per-unit marks (disrupted, out of
// supply at the start of the movement phase, entered this player-turn, movement points spent, halted
// or free to go on after an overrun, retreated onto a friendly stack, the hex a unit stood on before
// it entered an enemy zone of control), the interdiction and rail-cut markers, the Soviet schedule
// still owed, the South-Western Front and rail counts, the first-turn armies, and the Victory Point
// records. Markers live here and not as counters on the map, because the engine reads every enemy
// counter on a hex as a combat unit (see the task file). PggStateCodec is the only place these become
// hexsave <flag name value/> strings.
// ----------------------------------------------
#pragma once
#include "PggFacts.h"

#include "hexengine/GameNames.h"
#include "hexmodel/GameState.h"

#include <map>
#include <optional>
#include <set>
#include <string>
#include <vector>

namespace Pgg {

  enum class Army : std::uint8_t { Thirteenth, Sixteenth, Nineteenth, Twentieth };  // 5.2
  int armyNumber(Army);
  Army armyNumbered(int);  // throws for an army 5.2 does not name

  struct Owed {  // scheduled Soviet divisions still to enter at one entrance (16.1, 14.3)
    int rifles = 0;
    int armour = 0;
    auto operator<=>(const Owed&) const = default;
  };

  struct PggSideState {
    std::set<UnitId> disrupted;           // 6.6
    std::set<UnitId> unsupplied;          // at the start of the current movement phase (11.0, 11.31)
    std::set<UnitId> beyondRadius;        // Soviet, beyond every Leader's radius at that start (6.55)
    std::set<UnitId> entered;             // entered play this player-turn: in supply (11.33)
    std::map<UnitId, int> spent;          // movement points spent this phase, in halves (6.23)
    std::set<UnitId> halted;              // an overrun stopped them for the phase (6.52)
    std::set<UnitId> continuing;          // a successful overrun lets them move on (6.51)
    std::set<UnitId> retreatedOnto;       // retreated onto a friendly stack this phase (9.75)
    std::map<UnitId, HexIndex> zocEntry;  // the hex a unit left to enter an enemy zone of control (10.23)
    auto operator<=>(const PggSideState&) const = default;
  };

  class PggState : public HexModel::Cloneable<PggState, HexModel::GameState> {
  public:
    explicit PggState(std::size_t sides);

    // Throws std::invalid_argument for a side outside the rules.
    const PggSideState& side(SideId) const;
    PggSideState& side(SideId);
    std::size_t sideCount() const { return sides_.size(); }

    void appendDigest(std::string&) const override;

    // ---- German ----
    std::vector<HexIndex> airInterdiction;  // at most three, in the order placed (13.1)
    std::set<HexIndex> railCuts;             // at most six (errata 6.37)
    std::map<HexIndex, int> railRepaired;    // hex -> the turn a Soviet unit repaired it (6.37)
    std::set<HexIndex> passedByGermans;      // Railroad hexes a German combat unit entered this phase
    std::optional<int> smolenskTaken;        // the turn the German Player took Smolensk (13.2, amendments 15.11)
    std::set<HexIndex> germanHeld;           // cities held, with a line west, as the last German player-turn ended
    std::set<UnitId> germanEliminated;       // counters eliminated, for 15.12
    std::optional<std::size_t> outcome;      // the level of victory, once the game is over (15.2)

    // ---- Soviet ----
    std::optional<HexIndex> sovietInterdiction;  // 13.4
    int sovietInterdictionTurns = 0;
    int swfUsed = 0;       // South-Western Front divisions brought in this game (14.21)
    int swfThisTurn = 0;
    int railUnits = 0;     // combat units moved by rail this turn, armour counting three (6.31)
    int recaptureVp = 0;   // 15.12
    std::map<Area, Owed> owed;
    std::map<UnitId, Army> armies;  // the first-turn armies of 5.2
    std::set<Army> frozen;          // 5.21: may not move on Game-Turn One

  private:
    std::vector<PggSideState> sides_;
  };

  // The position's PGG state; throws std::invalid_argument when it holds none.
  const PggState& stateOf(const Position&);
  PggState& stateOf(Position&);

  class PggStateCodec : public HexModel::GameStateCodec {
  public:
    PggStateCodec(const PggFacts&, const HexEngine::GameNames&);
    HexModel::Polymorphic<HexModel::GameState> decode(const HexModel::SideFlags&) const override;
    HexModel::SideFlags encode(const HexModel::GameState&) const override;

  private:
    void decodeGerman(PggState&, const HexModel::SideFlag&) const;
    void decodeSoviet(PggState&, const HexModel::SideFlag&) const;
    void decodeSide(PggSideState&, SideId, const HexModel::SideFlag&) const;

    const PggFacts& facts_;
    const HexEngine::GameNames& names_;
  };

}  // namespace Pgg
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------

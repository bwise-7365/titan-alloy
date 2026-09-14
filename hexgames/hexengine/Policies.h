// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
// The plug-in interfaces a game implements where its prose changes a default. All are const,
// stateless and shared across threads; the engine ships a default for each (see defaults in
// hexengine/Defaults.h once implemented), mirroring common-abstractions.md section 3.
// ----------------------------------------------
#pragma once
#include "hexengine/Command.h"
#include "hexengine/Event.h"
#include "hexengine/PrngStreams.h"
#include "hexmodel/Board.h"
#include "hexmodel/Position.h"
#include "hexmodel/Quantities.h"
#include "hexmodel/Roster.h"
#include "hexrules/RuleSet.h"
#include "hexsearch/Search.h"

#include <bitset>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

namespace HexEngine {

  using HexRules::Purpose;
  using HexRules::RuleSet;

  // What every policy sees: the immutable definition and the current position.
  struct Ctx {
    const Board& board;
    const RuleSet& rules;
    const Roster& roster;
    const Position& position;
  };

  // A policy names the prose rules it implements, so the ledger test can check the claim.
  class Policy {
  public:
    virtual ~Policy() = default;
    virtual std::span<const std::string_view> claims() const = 0;
  };

  class ZocPolicy : public Policy {
  public:
    virtual bool projectsIntoP(const Ctx&, UnitId, HexIndex from, HexIndex to) const = 0;
    virtual bool blockedForP(const Ctx&, HexIndex, SideId mover, Purpose) const = 0;
  };

  struct EntryVerdict {
    std::variant<MovementPoints, HexModel::Prohibited> cost;
    bool mustStopP = false;
  };
  class MovementPolicy : public Policy {
  public:
    virtual EntryVerdict enter(const Ctx&, UnitId, HexIndex from, Direction, ModeId) const = 0;
    virtual Budget allowance(const Ctx&, UnitId, ModeId) const = 0;
    // Added in M6: whether a unit moving in `mode` has to stop on entering `hex`. Session::reachable
    // settles such a hex without expanding it; before M6 it read the terrain table itself, which
    // left a game no way to say "swamp is clear in snow" or "stop after the Kerch Strait".
    virtual bool stopsInP(const Ctx&, UnitId, HexIndex, ModeId) const = 0;
  };

  struct SupplyReport { std::vector<UnitId> supplied; std::vector<UnitId> unsupplied; };
  class SupplyTrace : public Policy {
  public:
    virtual SupplyReport trace(const Ctx&, HexSearch::SearchScratch&, SideId) const = 0;
  };

  struct CombatContext {
    std::vector<UnitId> attackers;
    HexIndex target;
    std::vector<UnitId> defenders;
    std::vector<Direction> attackDirections;  // hexsides crossed, from the defender's side
    std::vector<ModifierId> declared;
  };
  struct StepLoss { SideId side; int steps; };
  // Changed in M6: a range, because TRC's attackers retreat "one or two hexes" at the router's
  // choice while its defenders retreat exactly two.
  struct RetreatEffect { SideId side; int fewest; int most; };
  struct Eliminate { SideId side; };
  struct Surrender { SideId side; };
  struct NoEffect {};
  struct RevealAndReconsult {};
  struct GameEffect { std::string code; };
  using CombatEffect = std::variant<StepLoss, RetreatEffect, Eliminate, Surrender, NoEffect, RevealAndReconsult, GameEffect>;
  // Added in M4: the same resolution with the text the event log and a golden record need -- the
  // odds as printed, the table's own result code and every die value drawn, in order. The engine's
  // adjudicators call report(); resolve() is the shorthand for a caller that wants only the
  // effects, and an implementation writes it as report(...).effects.
  struct CombatReport {
    std::string odds;       // "3-1", or "below-minimum" / "above-maximum"
    std::string outcome;    // the resolver table's own result code, e.g. TRC's "EX"
    std::vector<int> dice;  // every die drawn while resolving, in the order drawn
    std::vector<CombatEffect> effects;
  };

  class CombatResolver : public Policy {
  public:
    virtual std::vector<CombatEffect> resolve(const Ctx&, const CombatContext&, PrngStreams&) const = 0;
    virtual CombatReport report(const Ctx&, const CombatContext&, PrngStreams&) const = 0;
  };

  // Added in M6: what a unit ordered to retreat does before any hex is chosen. Walk is the ordinary
  // case; Stay is a unit the rules hold in place (TRC woods); Surrender is a unit that leaves play
  // for good instead of retreating (TRC leaders and workers).
  enum class RetreatFate : std::uint8_t { Walk, Stay, Surrender };

  class RetreatPolicy : public Policy {
  public:
    // Hexes the unit may retreat into for one step, in a stable order; empty means it cannot.
    virtual std::vector<HexIndex> candidates(const Ctx&, UnitId, HexIndex origin) const = 0;
    virtual RetreatFate fate(const Ctx&, UnitId) const = 0;
  };

  class StackingPolicy : public Policy {
  public:
    // Units that exceed the limit on a hex, in the order the owner must remove them.
    virtual std::vector<UnitId> excess(const Ctx&, HexIndex) const = 0;
  };

  struct Outcome { std::optional<SideId> winner; std::string condition; };
  class VictoryCheck : public Policy {
  public:
    virtual std::optional<Outcome> check(const Ctx&) const = 0;
  };

  // Capabilities a phase grants, so legality is a mask test and not scattered branching.
  enum class Cap : std::uint8_t { Move, RailMove, SeaMove, Combat, Reinforce, Replace, Supply, Weather, Admin, Decision };
  using PhaseCaps = std::bitset<16>;
  class PhaseGate : public Policy {
  public:
    virtual bool activeP(const Ctx&, PhaseId) const = 0;
    virtual PhaseCaps capsFor(PhaseId) const = 0;
  };

  // The three randomiser types, deliberately not unified.
  class Deck {
  public:
    virtual ~Deck() = default;
    virtual std::string draw(PrngStreams&) = 0;
    virtual void discard(const std::string& card) = 0;
    virtual std::vector<std::string> order() const = 0;  // for hexsave piles
  };

  class StepRegistry;  // Steps.h: the behaviours the rules document's phase steps name (M6b)

  // Added in M6b: settles the obligations a game pushes on the resolution stack (Resolution.h). The
  // engine calls settle() when one of them is the top entry and asks nothing, and answer() when the
  // decision it asked is answered; the engine has already checked a GameChoice answer against the
  // verb and options. Each returns the next Position: the obligation popped or replaced, or a
  // decision asked on it. Both are pure and write what happened into the sink.
  class ObligationPolicy : public Policy {
  public:
    virtual Position settle(const Ctx&, PrngStreams&, EventSink&) const = 0;
    virtual Position answer(const Ctx&, const DecisionAnswer&, PrngStreams&, EventSink&) const = 0;
  };

  // The set a game hands the engine; unset members use the engine defaults.
  struct Policies {
    const ZocPolicy* zoc = nullptr;
    const MovementPolicy* movement = nullptr;
    const SupplyTrace* supply = nullptr;
    const CombatResolver* combat = nullptr;
    const RetreatPolicy* retreat = nullptr;
    const StackingPolicy* stacking = nullptr;
    const VictoryCheck* victory = nullptr;
    const PhaseGate* phases = nullptr;
    const CommandGrammar* grammar = nullptr;
    const StepRegistry* steps = nullptr;            // M6b: required; a Session verifies it against the rules
    const ObligationPolicy* obligations = nullptr;  // nullptr: a game obligation on the stack throws
    const HexModel::GameStateCodec* state = nullptr;  // M6b: hexsave flags <-> the game's own state
    const HexModel::ObligationCodec* obligationCodec = nullptr;  // hexsave <owe kind="game"> <-> a game obligation
  };

}  // namespace HexEngine
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------

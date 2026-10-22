// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
// The engine's own implementation of every policy interface, read out of the rules document rather
// than written per game. Each is const, stateless and shares the immutable trio, so one instance
// serves every thread. A game replaces the members of Policies it disagrees with; a default that
// guesses where the rules document holds only prose says so in its comment, and claims() is empty
// for all of them because a default implements no numbered rule of any game.
// ----------------------------------------------
#pragma once
#include "hexengine/Command.h"
#include "hexengine/GameNames.h"
#include "hexengine/Policies.h"
#include "hexengine/Steps.h"
#include "hexrules/Package.h"

#include <memory>
#include <string>
#include <string_view>
#include <vector>

namespace HexEngine {

  // The engine's reading of a dash-separated printed value line: "8-7" is one combat factor used
  // in attack and defence and a movement allowance (the commonest two-number counter), "3-3-1" is
  // attack, defence and allowance, "4-U" is a strength with unlimited range, "(2)" is a bracketed
  // strength. A game whose counters print something else supplies its own ValueLineReader.
  HexModel::Strengths defaultValueLine(std::string_view line, HexModel::UnitKind);

  // Who a side is fighting. A rules document that declares no <side @hostile-to> anywhere is the
  // plain case every two-player game is in -- every other side is an enemy -- and one that declares
  // any is taken at its word, since a three-way game has to say who is at war with whom.
  SideMask enemiesOf(const HexRules::RuleSet&, SideId);

  // ---- zones of control -------------------------------------------------------------------------
  // The first <zoc> of the rules document: its range, its blocked-by hexside terrains, its
  // negated-by-friendly flag and its projected-by unit types, with stops-movement answering
  // Purpose::Movement and the blocks mask answering the rest.
  class SixHexZoc : public ZocPolicy {
  public:
    explicit SixHexZoc(const HexRules::RuleSet&);
    std::span<const std::string_view> claims() const override;
    bool projectsIntoP(const Ctx&, UnitId, HexIndex from, HexIndex to) const override;
    bool blockedForP(const Ctx&, HexIndex, SideId mover, Purpose) const override;

  private:
    bool blockedEdgeP(const Ctx&, HexIndex, Direction) const;
    bool reachesP(const Ctx&, HexIndex from, HexIndex to) const;

    const HexRules::ZocSpec& spec_;
    int range_ = 1;
  };

  // ---- movement ---------------------------------------------------------------------------------
  // Shared by the three budgets: the entry cost of a hex is its terrain's move-cost plus the
  // move-cost of every hexside terrain crossed, refused when either prohibits or blocks movement or
  // when the hex's enter-only list excludes the unit, and stopping when the terrain says stop and
  // the unit's type is not excepted.
  class TerrainMovement : public MovementPolicy {
  public:
    explicit TerrainMovement(const HexRules::RuleSet&);
    std::span<const std::string_view> claims() const override;
    EntryVerdict enter(const Ctx&, UnitId, HexIndex from, Direction, ModeId) const override;
    // The hex terrain's stop, unless the unit's type is excepted (what Session read before M6).
    bool stopsInP(const Ctx&, UnitId, HexIndex, ModeId) const override;

  protected:
    // The printed allowance of a unit, as MovementPoints; throws when the counter prints none.
    static MovementPoints printedAllowance(const Ctx&, UnitId);
    const HexRules::RuleSet& rules_;
  };

  class PointCostMovement : public TerrainMovement {
  public:
    using TerrainMovement::TerrainMovement;
    Budget allowance(const Ctx&, UnitId, ModeId) const override;
  };

  class HexCountMovement : public TerrainMovement {
  public:
    using TerrainMovement::TerrainMovement;
    Budget allowance(const Ctx&, UnitId, ModeId) const override;
  };

  class ActionMovement : public TerrainMovement {
  public:
    using TerrainMovement::TerrainMovement;
    Budget allowance(const Ctx&, UnitId, ModeId) const override;
  };

  // movement/@budget picks one of the three.
  std::unique_ptr<MovementPolicy> movementFor(const HexRules::RuleSet&);

  // ---- supply -----------------------------------------------------------------------------------
  // The first <trace> of the rules document, segment by segment: a free walk of the segment's
  // length, then a flood along the segment's network, then the segment's region layer, stopping
  // when `threshold` sources are reached. A source is a hex the side controls that carries a drawn
  // feature -- the rules document holds its real source set as prose, so this is the engine's
  // reading of "a friendly-controlled city", and a game that means something else says so.
  // Never removes a unit, whatever @fatal says: it marks UnitFlags::isolatedP and reports.
  class BoundedTrace : public SupplyTrace {
  public:
    explicit BoundedTrace(const HexRules::RuleSet&, const ZocPolicy&);
    std::span<const std::string_view> claims() const override;
    SupplyReport trace(const Ctx&, HexSearch::SearchScratch&, SideId) const override;

  private:
    const HexRules::RuleSet& rules_;
    const ZocPolicy& zoc_;
  };

  // ---- combat -----------------------------------------------------------------------------------
  // The rules document's first odds-table resolver: makeOdds with its rounding, minimum and
  // maximum; declared modifiers of kind "shift" summed and capped; of kind "multiplier" applied to
  // the side they name and capped; the target hex's own defence-multiplier folded in; then the cell
  // at (die row, odds column) read as a result code and mapped to effects.
  class OddsTableResolver : public CombatResolver {
  public:
    explicit OddsTableResolver(const HexRules::RuleSet&);
    std::span<const std::string_view> claims() const override;
    std::vector<CombatEffect> resolve(const Ctx&, const CombatContext&, PrngStreams&) const override;
    CombatReport report(const Ctx&, const CombatContext&, PrngStreams&) const override;

    // The code map: TRC's AE A1 AR C EX DR D1 DE DS AS by default, anything else a GameEffect.
    static std::vector<CombatEffect> effectsOf(const std::string& code, SideId attacker, SideId defender);

  private:
    const HexRules::RuleSet& rules_;
    const HexRules::Resolver& resolver_;
    const HexRules::Table& table_;
    int faces_ = 6;
  };

  // ---- retreat ----------------------------------------------------------------------------------
  // Any adjacent hex that is not enemy-occupied, not in an enemy zone of control unless the rules
  // allow it, and not across a hexside the retreat spec's blocked-by list names; in direction order,
  // which is the same on every run.
  class AdjacentRetreat : public RetreatPolicy {
  public:
    AdjacentRetreat(const HexRules::RuleSet&, const ZocPolicy&);
    std::span<const std::string_view> claims() const override;
    std::vector<HexIndex> candidates(const Ctx&, UnitId, HexIndex origin) const override;
    RetreatFate fate(const Ctx&, UnitId) const override;  // always Walk

  private:
    const HexRules::RuleSet& rules_;
    const ZocPolicy& zoc_;
  };

  // ---- stacking ---------------------------------------------------------------------------------
  // stacking/@units as a plain count, with the exempt unit types not counted; the excess is the
  // units most recently placed on the hex, which is the order the owner must remove them in.
  class CountStacking : public StackingPolicy {
  public:
    explicit CountStacking(const HexRules::RuleSet&);
    std::span<const std::string_view> claims() const override;
    std::vector<UnitId> excess(const Ctx&, HexIndex) const override;

  private:
    const HexRules::RuleSet& rules_;
  };

  // ---- victory ----------------------------------------------------------------------------------
  // A hook: the rules document holds every victory condition as prose, so the engine declares none.
  class DefaultVictory : public VictoryCheck {
  public:
    std::span<const std::string_view> claims() const override;
    std::optional<Outcome> check(const Ctx&) const override;
  };

  // ---- phases -----------------------------------------------------------------------------------
  // Every phase is active, and its capabilities come from its printed name: "Movement" grants Move,
  // "Combat" grants Combat, "End" grants Supply and Admin, "Weather" grants Weather. That is a
  // default a game overrides, not a rule: a phase whose name says nothing grants Admin alone.
  class DefaultPhaseGate : public PhaseGate {
  public:
    explicit DefaultPhaseGate(const HexRules::RuleSet&);
    std::span<const std::string_view> claims() const override;
    bool activeP(const Ctx&, PhaseId) const override;
    PhaseCaps capsFor(PhaseId) const override;

  private:
    std::vector<PhaseCaps> capsByPhase_;
  };

  // ---- commands ---------------------------------------------------------------------------------
  // move, attack, resolve, answer, place, end-phase, and game:<verb> for anything a game adds.
  class DefaultCommandGrammar : public CommandGrammar {
  public:
    explicit DefaultCommandGrammar(const GameNames&);
    std::string verb(const Command&) const override;
    Command parse(const std::string& verb,
                   const std::vector<std::pair<std::string, std::string>>& args) const override;
    std::vector<std::pair<std::string, std::string>> arguments(const Command&) const override;
    // move, attack, resolve, answer, place, end-phase. A game's own "game:<verb>" is open-ended and
    // so is not listed: a rules step cannot name one.
    std::vector<std::string> verbs() const override;

  private:
    const GameNames& names_;
  };

  // ---- codecs for a game with no module (M6b review) -------------------------------------------
  // No game module, no game state: a document carrying any side flag is refused, naming every flag
  // and its side. The position then holds no game state and writes no flags.
  class NoGameStateCodec : public HexModel::GameStateCodec {
  public:
    explicit NoGameStateCodec(const HexRules::RuleSet&);
    HexModel::Polymorphic<HexModel::GameState> decode(const HexModel::SideFlags&) const override;
    HexModel::SideFlags encode(const HexModel::GameState&) const override;  // throws: nothing to encode

  private:
    const HexRules::RuleSet& rules_;
  };

  // No game module, no game obligations: reading or writing one throws, naming it.
  class NoObligationCodec : public HexModel::ObligationCodec {
  public:
    HexModel::Polymorphic<HexModel::GameObligation> decode(const std::string& name,
                                                           const std::vector<HexModel::ObligationArg>&) const override;
    std::vector<HexModel::ObligationArg> encode(const HexModel::GameObligation&) const override;
  };

  // ---- the set ----------------------------------------------------------------------------------
  // What the engine's own set does with rules steps whose behaviour only a game module registers
  // (M6b). Required: building a Session on such a document throws, naming the step. Withheld: each
  // such behaviour is registered as withheld -- its steps do nothing -- and steps().withheld() lists
  // it with the reason, so a run of a game's document on engine defaults says what it left out.
  enum class GameSteps : std::uint8_t { Required, Withheld };

  // Owns one of each and hands out the Policies a Session wants. A game builds one, then replaces
  // the members it disagrees with before handing the set to a Session. There is no Place and no
  // game verb: only a game knows where a counter may arrive or what its own verbs mean.
  class DefaultPolicySet {
  public:
    DefaultPolicySet(const HexRules::GameDefinition&, GameSteps);
    const Policies& policies() const { return policies_; }
    const GameNames& names() const { return names_; }
    const StepRegistry& steps() const { return steps_; }

  private:
    GameNames names_;
    SixHexZoc zoc_;
    std::unique_ptr<MovementPolicy> movement_;
    BoundedTrace supply_;
    OddsTableResolver combat_;
    AdjacentRetreat retreat_;
    CountStacking stacking_;
    DefaultVictory victory_;
    DefaultPhaseGate phases_;
    DefaultCommandGrammar grammar_;
    StepRegistry steps_;
    NoGameStateCodec state_;
    NoObligationCodec obligationCodec_;
    Policies policies_;
  };

}  // namespace HexEngine
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------

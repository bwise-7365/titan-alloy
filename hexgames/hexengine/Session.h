// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
// A Session is one game instance: the shared immutable definition plus this instance's Position,
// random streams, search scratch and event log. Nothing is shared mutably, so N sessions run on
// N threads; fork() is the only way to copy one.
// ----------------------------------------------
#pragma once
#include "hexengine/Command.h"
#include "hexengine/Event.h"
#include "hexengine/Policies.h"
#include "hexengine/PrngStreams.h"
#include "hexrules/Package.h"
#include "hexsearch/Search.h"

#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace HexEngine {

  namespace Sequence {
    struct Env;
  }

  struct Prompt {
    int turn = 1;
    PhaseId phase;
    std::optional<SideId> side;   // who acts; nullopt when the game is over
    bool decisionPendingP = false;
    bool overP = false;
  };

  struct Applied {
    std::size_t firstEvent;  // index into the log of this command's first event
    std::size_t eventCount;
  };

  // What a mover may do with one unit or stack: the reachable field with a path to each hex.
  struct Reachability {
    std::vector<HexIndex> hexes;
    std::vector<std::vector<HexIndex>> paths;  // parallel to hexes
    std::vector<MovementPoints> costs;
  };

  class Session {
  public:
    // Throws std::invalid_argument when the policy set has no StepRegistry or CommandGrammar, when
    // rules steps name command verbs the grammar lacks (all of them, each with its step), or when a
    // step names a behaviour the registry does not hold for its kind (naming the step and its does).
    Session(std::shared_ptr<const HexRules::GameDefinition>, const Policies&, Position start, std::uint64_t seed);

    const HexRules::GameDefinition& definition() const { return *definition_; }
    const Position& position() const { return position_; }
    const EventLog& events() const { return log_; }
    Prompt prompt() const;
    // Added in M4: what a save has to write back -- the seed and each stream's draw count -- and the
    // policy set a fork or a record writer needs to hand on.
    const PrngStreams& streams() const { return streams_; }
    const Policies& policies() const { return policies_; }
    // The immutable definition plus this session's position, as every policy and adjudicator sees it.
    struct Ctx context() const;

    // Finite choices: decisions, phase ends, placements, game verbs. Unit moves come from reachable().
    std::vector<Command> legalCommands() const;
    Reachability reachable(std::span<const UnitId> units, ModeId) const;
    std::vector<HexIndex> attackTargets(std::span<const UnitId> attackers) const;

    // Validates, adjudicates, appends events, notifies sinks. Throws std::invalid_argument, naming
    // the reason, on an illegal command; the Position is untouched in that case.
    Applied apply(const Command&);

    // A deep copy for rollouts: Position, streams and scratch; the definition is shared.
    Session fork() const;

    void attach(EventSink&);
    void detach(EventSink&);

  private:
    // Added in M4: validate the command, then build the next Position through the adjudicators,
    // writing every event into `sink`. Throws before touching anything on an illegal command, which
    // is what leaves position_ untouched. Since M6b: the rules' before-command steps, the engine's
    // adjudication, then the rules' after-command steps (Sequence.h).
    Position adjudicate(const Command&, EventSink&);
    // The engine's own adjudication of one command; an EndPhase runs the phase boundary's steps and
    // Place or a game verb goes to the command the StepRegistry holds for its verb.
    Position adjudicateCommand(const Command&, const std::string& verb, const Sequence::Env&);

    std::shared_ptr<const HexRules::GameDefinition> definition_;
    Policies policies_;
    Position position_;
    PrngStreams streams_;
    HexSearch::SearchScratch scratch_;
    EventLog log_;
    std::vector<EventSink*> sinks_;
  };

}  // namespace HexEngine
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------

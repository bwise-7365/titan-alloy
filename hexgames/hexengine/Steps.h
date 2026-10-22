// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
// The step registry (M6b, replacing M6's GameAdjudicator). The rules document's phase steps name
// behaviours by `does`; the engine registers its own and a game registers the rest. A before-command
// step names a check, which may refuse the command by throwing and cannot change the position or
// draw a die; every other step names an effect, pure from Position to Position. Place and a game's
// own verbs are adjudicated by the command registered under their grammar verb. Building a Session
// verifies that every step names a behaviour of its kind, and throws naming the step and its does.
// ----------------------------------------------
#pragma once
#include "hexengine/Command.h"
#include "hexengine/Event.h"
#include "hexengine/Policies.h"
#include "hexengine/PrngStreams.h"
#include "hexrules/RuleSet.h"
#include "hexsearch/Search.h"

#include <functional>
#include <map>
#include <string>
#include <string_view>
#include <vector>

namespace HexEngine {

  struct CheckCall {
    const Ctx& ctx;
    const Policies& policies;
    const Command& command;      // the command about to be adjudicated
    const HexRules::Step& step;  // the rules' own step: id, @rules, text
  };

  struct StepCall {
    const Ctx& ctx;
    const Policies& policies;
    PrngStreams& streams;
    HexSearch::SearchScratch& scratch;
    EventSink& sink;
    const Command& command;      // the command applied; the EndPhase for enter and end steps
    const HexRules::Step& step;
  };

  struct CommandCall {
    const Ctx& ctx;
    const Policies& policies;
    PrngStreams& streams;
    HexSearch::SearchScratch& scratch;
    EventSink& sink;
    const Command& command;
  };

  using CheckFn = std::function<void(const CheckCall&)>;
  using EffectFn = std::function<Position(const StepCall&)>;
  using CommandFn = std::function<Position(const CommandCall&)>;

  enum class StepKind : std::uint8_t { Check, Effect };
  StepKind kindOf(const HexRules::Step&);  // before-command: Check; the rest: Effect

  class StepRegistry {
  public:
    // Each throws std::invalid_argument when the name is already registered for that kind.
    void addCheck(const std::string& does, CheckFn);
    void addEffect(const std::string& does, EffectFn);
    void addCommand(const std::string& verb, CommandFn);
    // Marks `does` withheld: its steps are not run (a no-op is registered if nothing is), and
    // withheld() lists it with the first reason given.
    void withhold(const std::string& does, StepKind, const std::string& why);
    bool registeredP(const HexRules::Step&) const;  // a behaviour of the step's kind is held under its does

    // Two checks, in order, over every step that is not withheld. First, every @commands token must
    // be one of grammar.verbs(): all unknown verbs are reported in one std::invalid_argument, each
    // with its step id and line. Then every step must name a registered behaviour of its kind; the
    // first that does not is reported (step id, line, does).
    void verify(const HexRules::RuleSet&, const CommandGrammar&) const;
    const CheckFn& checkFor(const HexRules::Step&) const;    // throws as verify() does
    const EffectFn& effectFor(const HexRules::Step&) const;  // throws as verify() does
    const CommandFn& commandFor(const std::string& verb) const;  // throws naming the verb

    const std::map<std::string, std::string>& withheld() const { return withheld_; }
    bool withheldP(const std::string& does) const { return withheld_.contains(does); }
    // The prose rules named by the steps whose behaviour is registered and not withheld: what a
    // game's ledger counts as implemented by its sequence of play.
    std::vector<std::string_view> claims(const HexRules::RuleSet&) const;

  private:
    std::map<std::string, CheckFn> checks_;
    std::map<std::string, EffectFn> effects_;
    std::map<std::string, CommandFn> commands_;
    std::map<std::string, std::string> withheld_;
  };

  // The engine's own behaviours: effects "stacking-repair" (the StackingPolicy's excess to each
  // owner's returning box) and "supply-check" (the acting side's units marked supplied or isolated),
  // and the check "refuse" (refuses its @commands in its phase, naming its @rules).
  void registerEngineSteps(StepRegistry&);

  // Withholds, with `why`, every behaviour a rules step names that the registry lacks, and the
  // behaviour of every step whose @commands name a verb the grammar lacks (the reason names them).
  void withholdUnregistered(StepRegistry&, const HexRules::RuleSet&, const CommandGrammar&, const std::string& why);

}  // namespace HexEngine
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------

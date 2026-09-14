// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
#include "hexengine/Steps.h"

#include <algorithm>
#include <stdexcept>
#include <utility>

namespace HexEngine {

  namespace {

    template <class Fn>
    void
    forEachStep(const std::vector<HexRules::PhaseNode>& nodes, const Fn& fn)
    {
      for (const HexRules::PhaseNode& node : nodes) {
        for (const HexRules::Step& step : node.steps) {
          fn(step);
        }
        forEachStep(node.children, fn);
      }
      return;
    }

    [[noreturn]] void
    unregistered(const HexRules::Step& step, const char* kind)
    {
      throw std::invalid_argument("StepRegistry: rules step '" + step.id + "' (line " + std::to_string(step.line) +
                                  ") names does '" + step.does + "', and no " + kind +
                                  " of that name is registered");
    }

    std::vector<std::string>
    unknownVerbs(const HexRules::Step& step, const std::vector<std::string>& known)
    {
      std::vector<std::string> out;
      for (const std::string& verb : step.commands) {
        if (known.end() == std::find(known.begin(), known.end(), verb)) {
          out.push_back(verb);
        }
      }
      return out;
    }

  }  // namespace

  StepKind
  kindOf(const HexRules::Step& step)
  {
    switch (step.at) {
      case HexRules::StepAt::BeforeCommand:
        return StepKind::Check;
      case HexRules::StepAt::Enter:
      case HexRules::StepAt::AfterCommand:
      case HexRules::StepAt::End:
        return StepKind::Effect;
    }
    throw std::invalid_argument("StepRegistry: step '" + step.id + "' has an unknown at");
  }

  void
  StepRegistry::addCheck(const std::string& does, CheckFn fn)
  {
    if (!checks_.emplace(does, std::move(fn)).second) {
      throw std::invalid_argument("StepRegistry: check '" + does + "' is registered twice");
    }
    return;
  }

  void
  StepRegistry::addEffect(const std::string& does, EffectFn fn)
  {
    if (!effects_.emplace(does, std::move(fn)).second) {
      throw std::invalid_argument("StepRegistry: effect '" + does + "' is registered twice");
    }
    return;
  }

  void
  StepRegistry::addCommand(const std::string& verb, CommandFn fn)
  {
    if (!commands_.emplace(verb, std::move(fn)).second) {
      throw std::invalid_argument("StepRegistry: command '" + verb + "' is registered twice");
    }
    return;
  }

  void
  StepRegistry::withhold(const std::string& does, StepKind kind, const std::string& why)
  {
    switch (kind) {
      case StepKind::Check:
        if (!checks_.contains(does)) {
          addCheck(does, [](const CheckCall&) { return; });
        }
        break;
      case StepKind::Effect:
        if (!effects_.contains(does)) {
          addEffect(does, [](const StepCall& call) { return call.ctx.position; });
        }
        break;
    }
    withheld_.emplace(does, why);
    return;
  }

  bool
  StepRegistry::registeredP(const HexRules::Step& step) const
  {
    switch (kindOf(step)) {
      case StepKind::Check:
        return checks_.contains(step.does);
      case StepKind::Effect:
        return effects_.contains(step.does);
    }
    throw std::invalid_argument("StepRegistry: step '" + step.id + "' has an unknown kind");
  }

  const CheckFn&
  StepRegistry::checkFor(const HexRules::Step& step) const
  {
    const auto found = checks_.find(step.does);
    if (StepKind::Check != kindOf(step) || checks_.end() == found) {
      unregistered(step, "before-command check");
    }
    return found->second;
  }

  const EffectFn&
  StepRegistry::effectFor(const HexRules::Step& step) const
  {
    const auto found = effects_.find(step.does);
    if (StepKind::Effect != kindOf(step) || effects_.end() == found) {
      unregistered(step, "effect");
    }
    return found->second;
  }

  const CommandFn&
  StepRegistry::commandFor(const std::string& verb) const
  {
    const auto found = commands_.find(verb);
    if (commands_.end() == found) {
      throw std::invalid_argument("Session::apply: no adjudicator is registered for command '" + verb + "'");
    }
    return found->second;
  }

  void
  StepRegistry::verify(const HexRules::RuleSet& rules, const CommandGrammar& grammar) const
  {
    const std::vector<std::string> known = grammar.verbs();
    std::string unknown;
    forEachStep(rules.phases(), [&](const HexRules::Step& step) {
      if (withheld_.contains(step.does)) {
        return;
      }
      for (const std::string& verb : unknownVerbs(step, known)) {
        unknown += (unknown.empty() ? "" : "; ") +
                   ("step '" + step.id + "' (line " + std::to_string(step.line) + ") names '" + verb + "'");
      }
      return;
    });
    if (!unknown.empty()) {
      throw std::invalid_argument("StepRegistry: rules steps name command verbs the grammar does not know: " + unknown);
    }
    forEachStep(rules.phases(), [this](const HexRules::Step& step) {
      switch (kindOf(step)) {
        case StepKind::Check:
          (void)checkFor(step);
          break;
        case StepKind::Effect:
          (void)effectFor(step);
          break;
      }
      return;
    });
    return;
  }

  std::vector<std::string_view>
  StepRegistry::claims(const HexRules::RuleSet& rules) const
  {
    std::vector<std::string_view> out;
    forEachStep(rules.phases(), [&](const HexRules::Step& step) {
      if (!registeredP(step) || withheld_.contains(step.does)) {
        return;
      }
      for (const HexRules::RuleId& rule : step.rules) {
        out.push_back(rule.text);
      }
      return;
    });
    return out;
  }

  void
  withholdUnregistered(StepRegistry& registry, const HexRules::RuleSet& rules, const CommandGrammar& grammar,
                       const std::string& why)
  {
    const std::vector<std::string> known = grammar.verbs();
    std::vector<std::pair<const HexRules::Step*, std::string>> missing;
    forEachStep(rules.phases(), [&](const HexRules::Step& step) {
      const std::vector<std::string> unknown = unknownVerbs(step, known);
      if (!unknown.empty()) {
        std::string verbs;
        for (const std::string& verb : unknown) {
          verbs += (verbs.empty() ? "" : " ") + verb;
        }
        missing.emplace_back(&step, why + "; step '" + step.id + "' names command verbs the grammar lacks: " + verbs);
      } else if (!registry.registeredP(step)) {
        missing.emplace_back(&step, why);
      }
      return;
    });
    for (const auto& [step, reason] : missing) {
      registry.withhold(step->does, kindOf(*step), reason);
    }
    return;
  }

}  // namespace HexEngine
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------

// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
#include "hexengine/Sequence.h"

#include "hexengine/Adjudicators.h"

namespace HexEngine::Sequence {

  namespace {

    bool
    namesVerbP(const HexRules::Step& step, const std::string& verb)
    {
      if (step.commands.empty()) {
        return true;
      }
      for (const std::string& named : step.commands) {
        if (named == verb) {
          return true;
        }
      }
      return false;
    }

    void
    appendSteps(const HexRules::PhaseNode& node, HexRules::StepAt at, int turn, const std::string* verb,
                std::vector<const HexRules::Step*>& out)
    {
      for (const HexRules::Step& step : node.steps) {
        if (at == step.at && step.turns.containsP(turn) && (nullptr == verb || namesVerbP(step, *verb))) {
          out.push_back(&step);
        }
      }
      return;
    }

    Ctx
    contextOf(const Env& env, const Position& position)
    {
      return Ctx{*env.definition.board, *env.definition.rules, *env.definition.roster, position};
    }

    Position
    runEffects(const Env& env, const Position& start, const Command& command,
               const std::vector<const HexRules::Step*>& steps)
    {
      Position next = start;
      for (const HexRules::Step* step : steps) {
        if (env.policies.steps->withheldP(step->does)) {
          continue;  // withheld: listed by StepRegistry::withheld(), not run (and not copied through)
        }
        const Ctx ctx = contextOf(env, next);
        Position after =
            env.policies.steps->effectFor(*step)(StepCall{ctx, env.policies, env.streams, env.scratch, env.sink, command, *step});
        next = std::move(after);
      }
      return next;
    }

  }  // namespace

  std::vector<const HexRules::Step*>
  commandSteps(const PhaseCursor& cursor, HexModel::PhaseId phase, int turn, HexRules::StepAt at, const std::string& verb)
  {
    std::vector<const HexRules::Step*> out;
    for (HexModel::PhaseId id : cursor.ancestry(phase)) {
      appendSteps(cursor.node(id), at, turn, &verb, out);
    }
    return out;
  }

  Boundary
  boundary(const PhaseCursor& cursor, const PhaseCursor::Stop& current, const PhaseCursor::ActiveFilter& activeP)
  {
    Boundary out{{}, cursor.next(current, activeP), {}};
    const std::vector<PhaseCursor::Frame> leaving = cursor.lineage(current, activeP);
    const std::vector<PhaseCursor::Frame> arriving = cursor.lineage(out.next, activeP);
    std::size_t shared = 0;
    if (current.turn == out.next.turn) {
      while (shared < leaving.size() && shared < arriving.size() && leaving[shared] == arriving[shared]) {
        ++shared;
      }
    }
    for (std::size_t i = leaving.size(); i > shared; --i) {
      appendSteps(cursor.node(leaving[i - 1].phase), HexRules::StepAt::End, current.turn, nullptr, out.end);
    }
    for (std::size_t i = shared; i < arriving.size(); ++i) {
      appendSteps(cursor.node(arriving[i].phase), HexRules::StepAt::Enter, out.next.turn, nullptr, out.enter);
    }
    return out;
  }

  void
  check(const Env& env, const Position& position, const Command& command, const std::string& verb)
  {
    const PhaseCursor cursor(*env.definition.rules);
    const Ctx ctx = contextOf(env, position);
    const HexModel::TurnClock& clock = position.clock();
    for (const HexRules::Step* step : commandSteps(cursor, clock.phase, clock.turn, HexRules::StepAt::BeforeCommand, verb)) {
      if (env.policies.steps->withheldP(step->does)) {
        continue;  // withheld: listed by StepRegistry::withheld(), not run
      }
      env.policies.steps->checkFor(*step)(CheckCall{ctx, env.policies, command, *step});
    }
    return;
  }

  Position
  afterCommand(const Env& env, const Position& applied, const Command& command, const std::string& verb,
               const HexModel::TurnClock& issued)
  {
    const PhaseCursor cursor(*env.definition.rules);
    return runEffects(env, applied, command,
                      commandSteps(cursor, issued.phase, issued.turn, HexRules::StepAt::AfterCommand, verb));
  }

  Position
  endPhase(const Env& env, const Position& position, const Command& command)
  {
    const PhaseCursor cursor(*env.definition.rules);
    const Ctx ending = contextOf(env, position);
    const PhaseCursor::ActiveFilter activeP = [&](HexModel::PhaseId phase) {
      return env.policies.phases->activeP(ending, phase);
    };
    const HexModel::TurnClock& clock = position.clock();
    const Boundary crossing = boundary(cursor, PhaseCursor::Stop{clock.turn, clock.phase, clock.actingSide}, activeP);
    const Position ended = runEffects(env, position, command, crossing.end);
    const Position moved = Adjudicators::advancePhase(contextOf(env, ended), crossing.next, env.sink);
    return runEffects(env, moved, command, crossing.enter);
  }

}  // namespace HexEngine::Sequence
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------

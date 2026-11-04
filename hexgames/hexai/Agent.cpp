// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
#include "Agent.h"

#include "Fingerprint.h"

#include "hexengine/GameNames.h"

#include <stdexcept>
#include <variant>

namespace HexAi {

  namespace {

    bool
    mayChooseAgainP(const HexEngine::Command& command)
    {
      return !std::holds_alternative<HexEngine::EndPhase>(command) &&
             !std::holds_alternative<HexEngine::DecisionAnswer>(command);
    }

    std::string
    describe(const HexEngine::Session& session, const HexEngine::GameNames& names, const HexEngine::Command& command)
    {
      const HexModel::TurnClock& clock = session.position().clock();
      return std::to_string(clock.turn) + " " + names.phase(clock.phase) + " " + session.policies().grammar->verb(command);
    }

  }  // namespace

  PlayReport
  playOut(HexEngine::Session& session, const std::vector<Agent*>& agents, int attempts)
  {
    const HexRules::GameDefinition& definition = session.definition();
    const HexEngine::GameNames names(*definition.board, *definition.roster, *definition.rules);
    Fingerprint fingerprint(names, *session.policies().grammar);
    PlayReport report;
    for (int attempt = 0; attempt < attempts; ++attempt) {
      const HexEngine::Prompt prompt = session.prompt();
      if (prompt.overP) {
        report.finishedP = true;
        break;
      }
      Agent* const agent = prompt.side ? agents.at(prompt.side->value) : nullptr;
      const HexEngine::Command command = nullptr != agent ? agent->choose(session, *prompt.side) : HexEngine::Command{HexEngine::EndPhase{}};
      try {
        const HexEngine::Applied applied = session.apply(command);
        fingerprint.absorb(session, command, applied);
        report.turnFingerprints[prompt.turn] = fingerprint.value();
        report.commands.push_back(AppliedCommand{prompt.turn, prompt.phase, prompt.side, command});
      } catch (const std::invalid_argument& refusal) {
        if (nullptr == agent || !mayChooseAgainP(command)) {
          throw std::invalid_argument("no way on from turn " + describe(session, names, command) + ": " + refusal.what());
        }
        report.refusals.push_back(describe(session, names, command) + ": " + refusal.what());
        agent->refused(command, refusal.what());
      }
    }
    report.finishedP = report.finishedP || session.prompt().overP;
    report.fingerprint = fingerprint.value();
    return report;
  }

}  // namespace HexAi
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------

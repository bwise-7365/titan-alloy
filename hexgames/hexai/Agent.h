// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
// An agent is a Player that is told when the engine refuses its command, because the engine's own
// queries cannot promise a command is legal (a game's checks run only on apply). playOut drives a whole
// game with one agent per side and returns what happened: every command applied, every refusal with its
// reason, and a fingerprint of the game.
//
// A refused move, attack, placement or game verb is reported to the agent, which chooses something else.
// A refused phase end or decision answer leaves the game with no way on, so it throws: that is a fault
// in the agent or the engine, and hiding it would hide what these games exist to find.
// ----------------------------------------------
#pragma once
#include "hexengine/Player.h"
#include "hexengine/Session.h"

#include <cstdint>
#include <map>
#include <optional>
#include <string>
#include <vector>

namespace HexAi {

  class Agent : public HexEngine::Player {
  public:
    virtual void refused(const HexEngine::Command&, const std::string& why) = 0;
  };

  struct AppliedCommand {
    int turn = 0;
    HexModel::PhaseId phase;
    std::optional<HexModel::SideId> side;
    HexEngine::Command command;
  };

  struct PlayReport {
    std::vector<AppliedCommand> commands;
    std::vector<std::string> refusals;  // "<turn> <phase> <verb>: <reason>"
    std::uint64_t fingerprint = 0;
    std::map<int, std::uint64_t> turnFingerprints;  // as each turn ended, to find where two games part
    bool finishedP = false;                         // false: the limit came first
  };

  // Plays until the game is over, or until `attempts` commands have been applied or refused. `agents` is
  // indexed by SideId. A phase that names no side is ended here.
  PlayReport playOut(HexEngine::Session&, const std::vector<Agent*>& agents, int attempts);

}  // namespace HexAi
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------

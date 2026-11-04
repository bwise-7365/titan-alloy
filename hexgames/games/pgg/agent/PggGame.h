// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
// One game of PGG with two agents, as a value in and a value out. Every game of a batch reads the same
// PggShared and owns everything else, so that no game can touch another.
//
// With a drawn set-up, the Soviet divisions on the map and in the pool are shuffled within each arm (12.1)
// by a generator of their own, so the draw does not disturb the session's Setup stream. The fixed set-up is
// the scenario as written, drawn once with seed 1941. A seizing Soviet player sends between 1/2 and 2/3 of his
// strength to the fringe, the same share all game, drawn from the game seed by a generator of its own.
// ----------------------------------------------
#pragma once
#include "PggAgent.h"
#include "PggPolicySet.h"

#include "hexai/Agent.h"
#include "hexai/Tactics.h"

#include <cstdint>
#include <filesystem>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace Pgg {

  struct PggShared {
    std::shared_ptr<const HexRules::GameDefinition> definition;
    std::unique_ptr<PggPolicySet> set;
    Position start;
  };
  // hexgames' own directory: the package and the 1941 scenario are found from it.
  PggShared loadShared(const std::filesystem::path& root);

  enum class SetUp : std::uint8_t { Fixed, Drawn };
  Position drawnSetUp(const PggPolicySet&, Position start, std::uint64_t seed);

  struct PggGameTask {
    std::uint64_t seed = 0;
    SetUp setUp = SetUp::Fixed;
    HexAi::Tactic german;
    HexAi::Tactic soviet;
    int attempts = 200000;
    bool keepEventsP = false;
  };

  struct PggGameResult {
    HexAi::PlayReport play;
    std::optional<int> germanPoints;  // from the victory-points event, as the rules score after turn 12
    std::optional<int> sovietPoints;
    std::string level;                // the level of victory's name; empty when the game did not finish
    int lastTurn = 0;
    std::vector<std::string> events;  // each event as text, when the task keeps them
    int battles = 0;
    int recapturePoints = 0;          // the Soviet points from retaking cities (15.12)
    std::vector<std::string> germanCities;  // the Victory Point hexes the Germans held at the end
    double sovietFringeShare = 0.0;   // seize: the share drawn for this game
    PggAgentStats germanStats;
    PggAgentStats sovietStats;
  };

  // Throws std::invalid_argument when the game can go no further: a fault to report.
  PggGameResult playGame(const PggShared&, const PggGameTask&);

}  // namespace Pgg
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------

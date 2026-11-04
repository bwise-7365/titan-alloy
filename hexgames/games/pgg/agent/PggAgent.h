// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
// PggAgent -- a scripted player of one side of Panzergruppe Guderian, first version
// (doc/2026-11-04-pgg-balance-plan.md, section 3.2). It moves and attacks by a HexAi tactic and draws on
// no random stream: it works through the units in roster order, and ties go to the lowest index.
//
//   movement phases   arrivals enter, then each unit moves toward its goal; Leaders follow their divisions
//   first Soviet move the 13th and 20th Armies march and overstacked hexes clear, as 5.0 and 5.2 require
//   combat phases     each enemy hex, with every unit that can reach it, at 1-1 or better as far as it knows
//   air interdiction  the westernmost Soviet hexes
//
// It does not seize or hold locations, overrun, use rail, call the South-Western Front or set the Soviet
// Interdiction marker. One agent plays one game: it remembers what the engine refused this phase.
// ----------------------------------------------
#pragma once
#include "PggPolicySet.h"
#include "PggSeen.h"

#include "hexai/Agent.h"
#include "hexai/Tactics.h"
#include "hexsearch/Search.h"

#include <map>
#include <optional>
#include <set>
#include <string>
#include <vector>

namespace Pgg {

  class PggAgent final : public HexAi::Agent {
  public:
    PggAgent(const PggPolicySet&, SideId, HexAi::Tactic);
    HexEngine::Command choose(const HexEngine::Session&, SideId) override;
    void refused(const HexEngine::Command&, const std::string& why) override;

  private:
    struct PhaseMemory {  // forgotten when the phase changes
      std::set<UnitId> considered;                // decided on already, to move or to stay
      std::map<UnitId, std::set<HexIndex>> hexes;  // destinations the engine refused, per unit
      std::set<UnitId> arrivals;                  // Place commands the engine refused
      std::set<std::string> entries;              // reinforce commands refused: the type and the hex
      std::set<HexIndex> targets;                 // attacks and interdictions refused
    };

    void startPhase(const HexEngine::Session&);
    bool roomP(const Ctx&, HexIndex, UnitId) const;
    bool crossableP(HexIndex, HexModel::Direction) const;

    // PggAgent.cpp: the decisions a battle raises.
    HexEngine::Command answer(const HexEngine::Session&, const PggSeen&) const;
    std::string answerChoice(const PggSeen&, const HexModel::GameChoice&);

    // PggAgentMove.cpp
    std::optional<HexEngine::Command> nextArrival(const HexEngine::Session&);
    std::optional<HexEngine::Command> nextReinforcement(const HexEngine::Session&);
    std::optional<HexEngine::Command> nextMove(const HexEngine::Session&, const PggSeen&);
    std::optional<std::vector<HexIndex>> firstTurnPath(const Ctx&, UnitId, Army) const;
    std::vector<HexIndex> entryHexes(const Ctx&, Area) const;

    // PggAgentCombat.cpp
    std::optional<HexEngine::Command> nextAttack(const HexEngine::Session&, const PggSeen&);
    std::optional<HexEngine::Command> nextInterdiction(const HexEngine::Session&);
    std::vector<UnitId> attackersOn(const Ctx&, HexIndex target);

    const PggPolicySet& set_;
    const SideId side_;
    const HexAi::Tactic tactic_;
    HexSearch::SearchScratch scratch_;
    int turn_ = 0;
    PhaseId phase_{};
    PhaseMemory memory_;
    std::optional<HexAi::EnemyField> field_;  // the enemy does not move in this side's phase
    bool advancedP_ = false;                  // one unit advances after each battle
  };

}  // namespace Pgg
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------

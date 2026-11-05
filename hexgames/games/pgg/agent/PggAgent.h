// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
// PggAgent -- a scripted player of one side of Panzergruppe Guderian, first version
// (doc/2026-11-04-pgg-balance-plan.md, section 3.2). It moves and attacks by a HexAi tactic and draws on
// no random stream: it works through the units in roster order, and ties go to the lowest index.
//
//   movement phases   arrivals enter, then each unit moves toward its goal; Leaders follow their divisions, or
//                     under the seize tactic move last, to bring the most units within their radius
//   first Soviet move the 13th and 20th Armies march and overstacked hexes clear, as 5.0 and 5.2 require
//   combat phases     each enemy hex, with every unit that can reach it, at 1-1 or better as far as it knows
//   air interdiction  the westernmost Soviet hexes
//
// With the seize tactic (doc/2026-11-04-seize-locations-tactic.md), the movement goal comes from an allocation
// of forces to locations, made afresh each phase, and German divisions move as one. It does not overrun, use
// rail, call the South-Western Front or set the Soviet Interdiction marker. One agent plays one game: it
// remembers what the engine refused this phase.
// ----------------------------------------------
#pragma once
#include "PggPolicySet.h"
#include "PggSeen.h"

#include "hexai/Agent.h"
#include "hexai/Seize.h"
#include "hexai/Tactics.h"
#include "hexsearch/Search.h"

#include <map>
#include <optional>
#include <set>
#include <string>
#include <vector>

namespace Pgg {

  // What an agent did in one game, to report beside the score.
  struct PggAgentStats {
    int attacks = 0;
    int integratedAttacks = 0;    // attacks with a doubled division among the attackers
    int targetsKept = 0;          // seize: units free to move whose city is the one they last moved for
    int targetsChanged = 0;
    int unitsCounted = 0;         // Soviet: combat units on the map as each combat phase opened
    int unitsBeyondRadius = 0;    // of those, the ones no working Leader could send to attack
  };

  class PggAgent final : public HexAi::Agent {
  public:
    PggAgent(const PggPolicySet&, SideId, HexAi::Tactic);
    HexEngine::Command choose(const HexEngine::Session&, SideId) override;
    void refused(const HexEngine::Command&, const std::string& why) override;
    const PggAgentStats& stats() const { return stats_; }

  private:
    struct PhaseMemory {  // forgotten when the phase changes
      std::set<UnitId> considered;                // decided on already, to move or to stay
      std::map<UnitId, std::set<HexIndex>> hexes;  // destinations the engine refused, per unit
      std::set<UnitId> arrivals;                  // Place commands the engine refused
      std::set<std::string> entries;              // reinforce commands refused: the type and the hex
      std::set<HexIndex> targets;                 // attacks and interdictions refused
      bool plannedP = false;                      // seize: the allocation is made once a phase
      std::map<UnitId, std::size_t> cityOf;       // seize: the allocation, an index into the Victory Point hexes
      bool countedP = false;                      // the stats of this combat phase are taken
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
    std::vector<HexIndex> entryHexes(const Ctx&, Area, std::optional<UnitId> unit) const;

    // PggAgentSeize.cpp
    bool seizingP() const;
    void buildCityFields();
    std::vector<UnitId> groupOf(const Ctx&, UnitId) const;
    std::vector<HexIndex> germanPositions(const Ctx&) const;
    std::vector<HexAi::Target> seizeTargets(const Ctx&, const PggSeen&, std::vector<std::size_t>& cities) const;
    void planSeizure(const HexEngine::Session&, const PggSeen&);
    std::optional<HexEngine::Command> seizeMove(const HexEngine::Session&, const Ctx&, UnitId);
    bool leadersCoverP() const;
    std::optional<HexEngine::Command> coverMove(const HexEngine::Session&, const Ctx&, UnitId);
    std::string seizeAdvance(const Ctx&, const PggSeen&, const HexModel::GameChoice&);

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
    HexAi::StepCost stepCost_;                // seize: the terrain, for this side's commonest kind of unit
    std::vector<HexAi::CostField> cityFields_;  // seize: parallel to the Victory Point hexes
    std::map<UnitId, std::size_t> lastCityOf_;  // the city each unit was last free to move for
    std::optional<HexIndex> lastTarget_;      // the hex of the attack last declared
    std::optional<UnitId> advancing_;
    PggAgentStats stats_;
  };

}  // namespace Pgg
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------

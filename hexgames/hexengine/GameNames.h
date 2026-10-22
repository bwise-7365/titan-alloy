// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
// The names an event line, a command line and a hexsave document are written in: printed hex ids,
// counter ids, and the rules documents' own side, phase, space, mode and modifier ids. One small
// object over the immutable trio, so that nothing downstream has to carry three references.
// ----------------------------------------------
#pragma once
#include "hexmodel/Board.h"
#include "hexmodel/Ids.h"
#include "hexmodel/Roster.h"
#include "hexrules/RuleSet.h"

#include <map>
#include <optional>
#include <string>

namespace HexEngine {

  class GameNames {
  public:
    GameNames(const HexModel::Board&, const HexModel::Roster&, const HexRules::RuleSet&);

    const HexModel::Board& board() const { return board_; }
    const HexModel::Roster& roster() const { return roster_; }
    const HexRules::RuleSet& rules() const { return rules_; }

    // Dense id -> the token the documents write. Each throws std::invalid_argument naming the id.
    std::string hex(HexModel::HexIndex) const;
    std::string counter(HexModel::UnitId) const;
    std::string side(HexModel::SideId) const;
    std::string space(HexModel::SpaceId) const;
    std::string phase(HexModel::PhaseId) const;
    std::string mode(HexModel::ModeId) const;
    std::string modifier(HexModel::ModifierId) const;
    std::string randomizer(HexModel::RandomizerId) const;

    // The token a document writes -> the dense id. Each throws std::invalid_argument naming the id.
    HexModel::HexIndex hexOf(const std::string&) const;
    HexModel::UnitId unitOf(const std::string&) const;
    HexModel::SideId sideOf(const std::string&) const;
    HexModel::SpaceId spaceOf(const std::string&) const;
    HexModel::PhaseId phaseOf(const std::string&) const;
    HexModel::ModeId modeOf(const std::string&) const;
    HexModel::ModifierId modifierOf(const std::string&) const;

  private:
    const HexModel::Board& board_;
    const HexModel::Roster& roster_;
    const HexRules::RuleSet& rules_;
    // PhaseId -> the rules document's own phase id; RuleSet holds only the other direction.
    std::map<std::uint32_t, std::string> phaseIds_;
  };

}  // namespace HexEngine
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------

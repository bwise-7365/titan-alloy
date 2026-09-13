// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
#include "hexengine/GameNames.h"

#include <stdexcept>

namespace HexEngine {

  namespace {

    template <class Spec>
    std::uint32_t
    indexOfId(const std::vector<Spec>& specs, const std::string& id, const char* what)
    {
      for (std::size_t i = 0; i < specs.size(); ++i) {
        if (specs[i].id == id) {
          return static_cast<std::uint32_t>(i);
        }
      }
      throw std::invalid_argument(std::string("GameNames: unknown ") + what + " '" + id + "'");
    }

    template <class Spec>
    const std::string&
    idOfIndex(const std::vector<Spec>& specs, std::uint32_t index, const char* what)
    {
      if (specs.size() <= index) {
        throw std::invalid_argument(std::string("GameNames: ") + what + " index " +
                                     std::to_string(index) + " outside the rules");
      }
      return specs[index].id;
    }

  }  // namespace

  GameNames::GameNames(const HexModel::Board& board, const HexModel::Roster& roster,
                        const HexRules::RuleSet& rules)
    : board_(board), roster_(roster), rules_(rules)
  {
    for (const auto& [text, id] : rules_.phaseIds()) {
      phaseIds_[id.value] = text;
    }
  }

  std::string
  GameNames::hex(HexModel::HexIndex h) const
  {
    return board_.id(h).text;
  }

  std::string
  GameNames::counter(HexModel::UnitId u) const
  {
    return roster_.unit(u).counter.text;
  }

  std::string
  GameNames::side(HexModel::SideId s) const
  {
    return idOfIndex(rules_.sides(), s.value, "side");
  }

  std::string
  GameNames::space(HexModel::SpaceId s) const
  {
    return idOfIndex(rules_.spaces(), s.value, "space");
  }

  std::string
  GameNames::phase(HexModel::PhaseId p) const
  {
    const auto it = phaseIds_.find(p.value);
    if (phaseIds_.end() == it) {
      throw std::invalid_argument("GameNames: phase index " + std::to_string(p.value) +
                                   " outside the rules");
    }
    return it->second;
  }

  std::string
  GameNames::mode(HexModel::ModeId m) const
  {
    return idOfIndex(rules_.movement().modes, m.value, "mode");
  }

  std::string
  GameNames::modifier(HexModel::ModifierId m) const
  {
    return idOfIndex(rules_.combat().modifiers, m.value, "modifier");
  }

  std::string
  GameNames::randomizer(HexModel::RandomizerId r) const
  {
    return idOfIndex(rules_.randomizers(), r.value, "randomizer");
  }

  HexModel::HexIndex
  GameNames::hexOf(const std::string& id) const
  {
    return board_.indexOf(HexCoord::HexId{id});
  }

  HexModel::UnitId
  GameNames::unitOf(const std::string& id) const
  {
    const std::optional<HexModel::UnitId> found = roster_.find(HexModel::CounterId{id});
    if (!found) {
      throw std::invalid_argument("GameNames: unknown counter '" + id + "'");
    }
    return *found;
  }

  HexModel::SideId
  GameNames::sideOf(const std::string& id) const
  {
    return rules_.side(id);
  }

  HexModel::SpaceId
  GameNames::spaceOf(const std::string& id) const
  {
    return board_.spaceId(id);
  }

  HexModel::PhaseId
  GameNames::phaseOf(const std::string& id) const
  {
    return rules_.phase(id);
  }

  HexModel::ModeId
  GameNames::modeOf(const std::string& id) const
  {
    return HexModel::ModeId{indexOfId(rules_.movement().modes, id, "mode")};
  }

  HexModel::ModifierId
  GameNames::modifierOf(const std::string& id) const
  {
    return HexModel::ModifierId{indexOfId(rules_.combat().modifiers, id, "modifier")};
  }

}  // namespace HexEngine
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------

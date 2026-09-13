// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
#include "hexrules/RuleSet.h"

#include <stdexcept>

namespace HexRules {

  SideMask
  HostilityMatrix::enemiesOf(SideId s) const
  {
    if (s.value >= rows_.size()) {
      throw std::invalid_argument("HostilityMatrix::enemiesOf: side id out of range");
    }
    return rows_[s.value];
  }

  bool
  HostilityMatrix::hostileP(SideId a, SideId b) const
  {
    if (a.value >= rows_.size()) {
      throw std::invalid_argument("HostilityMatrix::hostileP: side id out of range");
    }
    return rows_[a.value].test(b.value);
  }

  const std::string&
  Table::cell(std::size_t row, std::size_t col) const
  {
    if (row >= cells.size() || col >= cells[row].size()) {
      throw std::invalid_argument("Table: row/col out of range for table '" + id + "'");
    }
    return cells[row][col];
  }

  SideId
  RuleSet::side(const std::string& id) const
  {
    for (std::size_t i = 0; i < sides_.size(); ++i) {
      if (sides_[i].id == id) {
        return SideId{static_cast<std::uint32_t>(i)};
      }
    }
    throw std::invalid_argument("RuleSet::side: unknown side id '" + id + "'");
  }

  UnitTypeId
  RuleSet::unitType(const std::string& id) const
  {
    for (std::size_t i = 0; i < unitTypes_.size(); ++i) {
      if (unitTypes_[i].id == id) {
        return UnitTypeId{static_cast<std::uint32_t>(i)};
      }
    }
    throw std::invalid_argument("RuleSet::unitType: unknown unit type id '" + id + "'");
  }

  TerrainId
  RuleSet::terrain(const std::string& id) const
  {
    for (std::size_t i = 0; i < hexTerrain_.size(); ++i) {
      if (hexTerrain_[i].id == id) {
        return TerrainId{static_cast<std::uint32_t>(i)};
      }
    }
    throw std::invalid_argument("RuleSet::terrain: unknown terrain id '" + id + "'");
  }

  EdgeTerrainId
  RuleSet::edgeTerrain(const std::string& id) const
  {
    for (std::size_t i = 0; i < hexsideTerrain_.size(); ++i) {
      if (hexsideTerrain_[i].id == id) {
        return EdgeTerrainId{static_cast<std::uint32_t>(i)};
      }
    }
    throw std::invalid_argument("RuleSet::edgeTerrain: unknown hexside terrain id '" + id + "'");
  }

  PhaseId
  RuleSet::phase(const std::string& id) const
  {
    const auto it = phaseByName_.find(id);
    if (phaseByName_.end() == it) {
      throw std::invalid_argument("RuleSet::phase: unknown phase id '" + id + "'");
    }
    return it->second;
  }

}  // namespace HexRules
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------

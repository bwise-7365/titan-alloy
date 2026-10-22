// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
// Small, shared, header-only parsing and name-resolution helpers for RuleSetBuilder.cpp and
// RuleSetBuilderCombat.cpp. Not part of the public facade.
// ----------------------------------------------
#pragma once
#include "hexrules/RuleSet.h"
#include "hexxml/RulesDoc.h"

#include <cmath>
#include <map>
#include <optional>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace HexRules::Detail {

  [[noreturn]] inline void
  throwUnknown(const std::string& kind, const std::string& id)
  {
    throw std::invalid_argument("RuleSetBuilder: unknown " + kind + " id '" + id + "'");
  }

  template <class IdT>
  IdT
  resolveOne(const std::map<std::string, IdT>& table, const std::string& name, const std::string& kind)
  {
    const auto it = table.find(name);
    if (table.end() == it) {
      throwUnknown(kind, name);
    }
    return it->second;
  }

  template <class IdT>
  std::vector<IdT>
  resolveMany(const std::map<std::string, IdT>& table, const std::vector<std::string>& names,
              const std::string& kind)
  {
    std::vector<IdT> result;
    result.reserve(names.size());
    for (const std::string& n : names) {
      result.push_back(resolveOne(table, n, kind));
    }
    return result;
  }

  inline SideMask
  resolveSideMask(const std::map<std::string, SideId>& table, const std::vector<std::string>& names)
  {
    SideMask mask;
    for (const std::string& n : names) {
      mask.set(resolveOne(table, n, "side").value);
    }
    return mask;
  }

  inline PurposeMask
  resolvePurposes(const std::vector<std::string>& tokens)
  {
    static constexpr std::pair<std::string_view, Purpose> kTable[] = {
        {"movement", Purpose::Movement}, {"zoc", Purpose::Zoc},         {"supply", Purpose::Supply},
        {"retreat", Purpose::Retreat},   {"network", Purpose::Network}, {"control", Purpose::Control},
        {"grouping", Purpose::Grouping}};
    PurposeMask mask;
    for (const std::string& t : tokens) {
      bool found = false;
      for (const auto& [name, purpose] : kTable) {
        if (name == t) {
          mask.set(static_cast<std::size_t>(purpose));
          found = true;
          break;
        }
      }
      if (!found) {
        throwUnknown("purpose", t);
      }
    }
    return mask;
  }

  inline MoveCost
  parseMoveCost(const std::optional<std::string>& raw)
  {
    if (!raw) {
      return NoCost{};
    }
    if ("prohibited" == *raw) {
      return Prohibited{};
    }
    if ("entire" == *raw) {
      return Entire{};
    }
    if ("other-terrain" == *raw) {
      return OtherTerrain{};
    }
    if ("none" == *raw) {
      return NoCost{};
    }
    const double v = std::stod(*raw);
    return MovementPoints{static_cast<int>(std::lround(v * 2.0))};
  }

  inline Extent
  parseExtent(const std::string& raw)
  {
    if ("unlimited" == raw) {
      return Unlimited{};
    }
    if ("authored" == raw) {
      return Authored{};
    }
    return HexCount{std::stoi(raw)};
  }

  inline Rounding
  parseRounding(const std::optional<std::string>& raw)
  {
    if (!raw) {
      return Rounding::Defender;
    }
    if ("defender" == *raw) {
      return Rounding::Defender;
    }
    if ("attacker" == *raw) {
      return Rounding::Attacker;
    }
    return Rounding::None;
  }

  inline TurnSelector
  parseTurnsOr(const std::optional<std::string>& raw, TurnSelector fallback)
  {
    return raw ? TurnSelector::parse(*raw) : std::move(fallback);
  }

  inline Table
  toTable(const HexXml::TableAnnex& t)
  {
    Table out;
    out.id = t.id;
    for (const HexXml::TableColDoc& c : t.columns) {
      out.cols.push_back(c.label);
    }
    for (const HexXml::TableRowDoc& r : t.tableRows) {
      out.rowLabels.push_back(r.label);
      out.cells.push_back(r.cells);
    }
    return out;
  }

}  // namespace HexRules::Detail
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------

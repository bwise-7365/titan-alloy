// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
#include "TrcFlags.h"

#include <algorithm>
#include <stdexcept>

namespace Trc::Flags {

  int
  counter(const HexModel::Position& position, HexModel::SideId side, const std::string& name)
  {
    const std::optional<std::string> value = position.flag(side, name);
    if (!value) {
      return 0;
    }
    try {
      return std::stoi(*value);
    } catch (const std::exception&) {
      throw std::invalid_argument("Trc::Flags: flag '" + name + "' holds '" + *value + "', not a count");
    }
  }

  void
  bump(HexModel::Position& position, HexModel::SideId side, const std::string& name, int by)
  {
    position.setFlag(side, name, std::to_string(counter(position, side, name) + by));
    return;
  }

  std::vector<std::string>
  list(const HexModel::Position& position, HexModel::SideId side, const std::string& name)
  {
    std::vector<std::string> out;
    std::string current;
    for (char c : position.flag(side, name).value_or("")) {
      if (' ' == c) {
        if (!current.empty()) {
          out.push_back(current);
        }
        current.clear();
      } else {
        current += c;
      }
    }
    if (!current.empty()) {
      out.push_back(current);
    }
    return out;
  }

  void
  add(HexModel::Position& position, HexModel::SideId side, const std::string& name, const std::string& item)
  {
    std::vector<std::string> items = list(position, side, name);
    if (items.end() != std::find(items.begin(), items.end(), item)) {
      return;
    }
    items.push_back(item);
    std::sort(items.begin(), items.end());
    std::string joined;
    for (const std::string& each : items) {
      joined += (joined.empty() ? "" : " ") + each;
    }
    position.setFlag(side, name, joined);
    return;
  }

  bool
  listedP(const HexModel::Position& position, HexModel::SideId side, const std::string& name,
           const std::string& item)
  {
    const std::vector<std::string> items = list(position, side, name);
    return items.end() != std::find(items.begin(), items.end(), item);
  }

  bool
  markedP(const HexModel::Position& position, HexModel::UnitId unit, const std::string& marker)
  {
    const std::vector<std::string>& markers = position.unit(unit).markers;
    return markers.end() != std::find(markers.begin(), markers.end(), marker);
  }

  void
  mark(HexModel::Position& position, HexModel::UnitId unit, const std::string& marker)
  {
    if (!markedP(position, unit, marker)) {
      position.state(unit).markers.push_back(marker);
    }
    return;
  }

  void
  unmark(HexModel::Position& position, HexModel::UnitId unit, const std::string& marker)
  {
    std::vector<std::string>& markers = position.state(unit).markers;
    markers.erase(std::remove(markers.begin(), markers.end(), marker), markers.end());
    return;
  }

}  // namespace Trc::Flags
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------

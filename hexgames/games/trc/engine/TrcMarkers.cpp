// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
#include "TrcMarkers.h"

#include <algorithm>

namespace Trc::Markers {

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

}  // namespace Trc::Markers
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------

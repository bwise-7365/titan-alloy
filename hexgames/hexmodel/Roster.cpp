// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
#include "hexmodel/Roster.h"

#include <stdexcept>

namespace HexModel {

  const UnitSpec&
  Roster::unit(UnitId id) const
  {
    if (id.value >= units_.size()) {
      throw std::invalid_argument("Roster::unit: unit id out of range");
    }
    return units_[id.value];
  }

  std::optional<UnitId>
  Roster::find(const CounterId& counter) const
  {
    const auto it = byCounter_.find(counter);
    if (byCounter_.end() == it) {
      return std::nullopt;
    }
    return it->second;
  }

  std::vector<UnitId>
  Roster::ofSide(SideId side) const
  {
    std::vector<UnitId> result;
    for (std::size_t i = 0; i < units_.size(); ++i) {
      if (units_[i].side == side) {
        result.push_back(UnitId{static_cast<std::uint32_t>(i)});
      }
    }
    return result;
  }

}  // namespace HexModel
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------

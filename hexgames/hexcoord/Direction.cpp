// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
// The two partial direction functions: the ones that can be handed something that is not a direction
// at all, and so must reject it rather than substitute a default.
// ----------------------------------------------
#include "Direction.h"

#include <string>

namespace HexCoord {

  Direction
  directionOf(Qrs v)
  {
    for (int i = 0; i < kDirections; ++i) {
      const Direction d = static_cast<Direction>(i);
      if (step(d) == v) {
        return d;
      }
    }
    throw std::invalid_argument("directionOf: not a unit step: [QRS " + std::to_string(v.q()) +
                                ", " + std::to_string(v.r()) + ", " + std::to_string(v.s()) + "]");
  }

  Direction
  fromCompass(std::string_view name, Orientation orientation)
  {
    for (int i = 0; i < kDirections; ++i) {
      const Direction d = static_cast<Direction>(i);
      if (compassName(d, orientation) == name) {
        return d;
      }
    }
    const std::string which = Orientation::Flat == orientation ? "flat" : "pointy";
    throw std::invalid_argument("fromCompass: no " + which + " hexside is named '" +
                                std::string(name) + "'");
  }

}  // namespace HexCoord
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------

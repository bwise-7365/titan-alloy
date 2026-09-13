// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
// The six hexside directions as a cyclic type, orientation-free in the algebra; compass names are a
// property of the sheet orientation only.
// ----------------------------------------------
#pragma once
#include "Abc.h"

#include <cstdint>
#include <string_view>

namespace HexCoord {

  enum class Orientation : std::uint8_t { Flat, Pointy };
  enum class Parity : std::uint8_t { Odd, Even };

  // Clockwise from the first: -Q, +R, -S, +Q, -R, +S. The cycle is the same for both orientations
  // (a rotation preserves handedness); only the names differ:
  //   Flat:   n  ne  se  s   sw  nw
  //   Pointy: ne e   se  sw  w   nw
  // which is exactly hexsheet2svg.py's edge order for each orientation.
  enum class Direction : std::uint8_t { D0, D1, D2, D3, D4, D5 };

  inline constexpr int kDirections = 6;

  constexpr Qrs step(Direction);                    // the unit Qrs displacement
  constexpr Direction rotate(Direction, int k);     // k steps clockwise (negative: anticlockwise)
  constexpr Direction opposite(Direction);          // rotate(d, 3)

  // The direction of a unit step; throws std::invalid_argument for any other Qrs.
  Direction directionOf(Qrs);

  constexpr std::string_view compassName(Direction, Orientation);
  // Throws std::invalid_argument for a name that does not exist in that orientation.
  Direction fromCompass(std::string_view name, Orientation);

  // Angle, in degrees clockwise from screen east (y down), of the outward normal of the hexside in
  // this direction, for the given orientation. Used by the pixel mapping and by glyph rotation.
  constexpr double edgeAngleDegrees(Direction, Orientation);

}  // namespace HexCoord
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------

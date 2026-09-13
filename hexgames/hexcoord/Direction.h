// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
// The six hexside directions as a cyclic type, orientation-free in the algebra; compass names are a
// property of the sheet orientation only.
// ----------------------------------------------
#pragma once
#include "Abc.h"

#include <cstdint>
#include <stdexcept>
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

  // ---- definitions of the constexpr declarations above ----------------------------------------

  constexpr Qrs
  step(Direction d)
  {
    switch (d) {
      case Direction::D0:
        return Qrs{-1, 0, 0};
      case Direction::D1:
        return Qrs{0, 1, 0};
      case Direction::D2:
        return Qrs{0, 0, -1};
      case Direction::D3:
        return Qrs{1, 0, 0};
      case Direction::D4:
        return Qrs{0, -1, 0};
      case Direction::D5:
        return Qrs{0, 0, 1};
    }
    throw std::invalid_argument("step: direction outside D0..D5");
  }

  constexpr Direction
  rotate(Direction d, int k)
  {
    return static_cast<Direction>(iMod(static_cast<int>(d) + k, kDirections));
  }

  constexpr Direction
  opposite(Direction d)
  {
    return rotate(d, 3);
  }

  namespace Detail {

    constexpr std::string_view
    flatCompass(Direction d)
    {
      switch (d) {
        case Direction::D0:
          return "n";
        case Direction::D1:
          return "ne";
        case Direction::D2:
          return "se";
        case Direction::D3:
          return "s";
        case Direction::D4:
          return "sw";
        case Direction::D5:
          return "nw";
      }
      throw std::invalid_argument("flatCompass: direction outside D0..D5");
    }

    constexpr std::string_view
    pointyCompass(Direction d)
    {
      switch (d) {
        case Direction::D0:
          return "ne";
        case Direction::D1:
          return "e";
        case Direction::D2:
          return "se";
        case Direction::D3:
          return "sw";
        case Direction::D4:
          return "w";
        case Direction::D5:
          return "nw";
      }
      throw std::invalid_argument("pointyCompass: direction outside D0..D5");
    }

  }  // namespace Detail

  constexpr std::string_view
  compassName(Direction d, Orientation o)
  {
    switch (o) {
      case Orientation::Flat:
        return Detail::flatCompass(d);
      case Orientation::Pointy:
        return Detail::pointyCompass(d);
    }
    throw std::invalid_argument("compassName: orientation is neither Flat nor Pointy");
  }

  // D0 is the outward normal of the first hexside: due north for a flat-topped hex, north-east for a
  // pointy-topped one, the whole cycle turning by 60 degrees clockwise. These are hexsheet2svg.py's
  // `edges` angles for each orientation.
  constexpr double
  edgeAngleDegrees(Direction d, Orientation o)
  {
    const double first = Orientation::Flat == o ? 270.0 : 300.0;
    const double turned = first + 60.0 * static_cast<int>(d);
    return turned < 360.0 ? turned : turned - 360.0;
  }

}  // namespace HexCoord
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------

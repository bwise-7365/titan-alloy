// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
// The TRC calendar: turn 1 is May/June 1941 and each turn is two months (rules document
// sequence/@turns and its note), so a turn number alone gives the months and the year.
// ----------------------------------------------
#pragma once
#include <cstdint>
#include <string_view>

namespace Trc {

  enum class Months : std::uint8_t { MayJun, JulAug, SepOct, NovDec, JanFeb, MarApr };
  enum class Weather : std::uint8_t { Clear, LightMud, Mud, Snow };

  Months monthsOf(int turn);
  int yearOf(int turn);  // 1941 for turns 1-4, 1942 for turns 5-10, ...

  std::string_view weatherName(Weather);  // "clear", "light-mud", "mud", "snow" (weather-states list)
  Weather weatherNamed(std::string_view);  // throws std::invalid_argument naming the text

}  // namespace Trc
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------

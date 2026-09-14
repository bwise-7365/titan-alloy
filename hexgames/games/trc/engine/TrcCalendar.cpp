// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
#include "TrcCalendar.h"

#include <stdexcept>
#include <string>

namespace Trc {

  Months
  monthsOf(int turn)
  {
    if (1 > turn) {
      throw std::invalid_argument("Trc::monthsOf: turn " + std::to_string(turn) + " is before May/June 1941");
    }
    return static_cast<Months>((turn - 1) % 6);
  }

  int
  yearOf(int turn)
  {
    if (1 > turn) {
      throw std::invalid_argument("Trc::yearOf: turn " + std::to_string(turn) + " is before May/June 1941");
    }
    return 1941 + (turn + 1) / 6;
  }

  std::string_view
  weatherName(Weather weather)
  {
    switch (weather) {
      case Weather::Clear:
        return "clear";
      case Weather::LightMud:
        return "light-mud";
      case Weather::Mud:
        return "mud";
      case Weather::Snow:
        return "snow";
    }
    throw std::invalid_argument("Trc::weatherName: weather outside the four states");
  }

  Weather
  weatherNamed(std::string_view text)
  {
    for (Weather weather : {Weather::Clear, Weather::LightMud, Weather::Mud, Weather::Snow}) {
      if (weatherName(weather) == text) {
        return weather;
      }
    }
    throw std::invalid_argument("Trc::weatherNamed: '" + std::string(text) + "' is not a weather state");
  }

}  // namespace Trc
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------

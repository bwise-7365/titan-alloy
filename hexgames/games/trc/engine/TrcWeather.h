// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
// TRC weather (4.1.1): May/June and July/August are clear and January/February snow; the other
// three turns roll one die plus a running DRM, clamped to 0-7, on a chart indexed by months. The
// state and the DRM are Axis side flags "weather" and "weather-drm" (the Axis rolls for both).
// ----------------------------------------------
#pragma once
#include "TrcCalendar.h"
#include "TrcFacts.h"

namespace Trc {

  class TrcWeather {
  public:
    explicit TrcWeather(const TrcFacts&);
    static std::span<const std::string_view> claims();

    // The weather of the position's turn: fixed by the calendar, else the flag the roll wrote.
    // Throws when a rolled turn has no roll recorded.
    Weather current(const Position&) const;
    int drm(const Position&) const;

    // The Weather Phase: roll (or read the fixed state), update the DRM, write both flags.
    Position roll(const Ctx&, HexEngine::PrngStreams&, HexEngine::EventSink&) const;

    // The chart itself, exposed for the tests: the state for a modified roll, and the DRM change
    // that state carries forward.
    static Weather chart(Months, int modifiedRoll);
    static int drmChange(Weather);

  private:
    const TrcFacts& facts_;
  };

}  // namespace Trc
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------

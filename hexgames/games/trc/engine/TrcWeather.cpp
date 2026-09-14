// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
#include "TrcWeather.h"

#include <algorithm>
#include <array>
#include <stdexcept>
#include <string>

namespace Trc {

  namespace {

    constexpr std::array<std::string_view, 2> kClaims{"weather-fixed-turns", "weather-drm"};

    using Row = std::array<Weather, 8>;  // modified roll 0 (or less) .. 7 (or more)

    // Transcribed from the sheet's Weather Chart panel (map_graphics/xml/the-russian-campaign.xml).
    constexpr Row kMarApr{Weather::Clear, Weather::LightMud, Weather::LightMud, Weather::LightMud,
                          Weather::LightMud, Weather::Mud, Weather::Mud, Weather::Snow};
    constexpr Row kSepOct{Weather::Clear, Weather::Clear, Weather::Clear, Weather::LightMud,
                          Weather::LightMud, Weather::Mud, Weather::Mud, Weather::Snow};
    // TODO(decide): PROVISIONAL. The sheet's panel prints no November/December row although the
    // rules roll that turn; this row is the engine's placeholder until the chart is transcribed.
    constexpr Row kNovDec{Weather::LightMud, Weather::LightMud, Weather::Mud, Weather::Mud,
                          Weather::Snow, Weather::Snow, Weather::Snow, Weather::Snow};

    const std::string kWeather = "weather";
    const std::string kDrm = "weather-drm";

    std::string
    describe(Weather weather, int die, int drm, int modified)
    {
      return std::string(weatherName(weather)) + " (die " + std::to_string(die) + ", drm " +
             std::to_string(drm) + ", row " + std::to_string(modified) + ")";
    }

  }  // namespace

  TrcWeather::TrcWeather(const TrcFacts& facts) : facts_(facts)
  {
  }

  std::span<const std::string_view>
  TrcWeather::claims()
  {
    return kClaims;
  }

  Weather
  TrcWeather::chart(Months months, int modifiedRoll)
  {
    const std::size_t column = static_cast<std::size_t>(std::clamp(modifiedRoll, 0, 7));
    switch (months) {
      case Months::MarApr:
        return kMarApr[column];
      case Months::SepOct:
        return kSepOct[column];
      case Months::NovDec:
        return kNovDec[column];
      case Months::MayJun:
      case Months::JulAug:
        return Weather::Clear;
      case Months::JanFeb:
        return Weather::Snow;
    }
    throw std::invalid_argument("TrcWeather::chart: months outside the six");
  }

  // TODO(decide): PROVISIONAL. The rules say clear pushes the running DRM up and mud and snow push
  // it sharply down, but no input prints the amounts; these are the engine's placeholder values.
  int
  TrcWeather::drmChange(Weather weather)
  {
    switch (weather) {
      case Weather::Clear:
        return 1;
      case Weather::LightMud:
        return 0;
      case Weather::Mud:
        return -2;
      case Weather::Snow:
        return -3;
    }
    throw std::invalid_argument("TrcWeather::drmChange: weather outside the four states");
  }

  Weather
  TrcWeather::current(const Position& position) const
  {
    switch (monthsOf(position.clock().turn)) {
      case Months::MayJun:
      case Months::JulAug:
        return Weather::Clear;
      case Months::JanFeb:
        return Weather::Snow;
      case Months::SepOct:
      case Months::NovDec:
      case Months::MarApr:
        break;
    }
    const std::optional<std::string> flag = position.flag(facts_.axis(), kWeather);
    if (!flag) {
      throw std::invalid_argument("TrcWeather: turn " + std::to_string(position.clock().turn) +
                                   " rolls its weather, and the position records no roll (axis flag 'weather')");
    }
    return weatherNamed(*flag);
  }

  int
  TrcWeather::drm(const Position& position) const
  {
    const std::optional<std::string> flag = position.flag(facts_.axis(), kDrm);
    if (!flag) {
      throw std::invalid_argument("TrcWeather: the position records no weather DRM (axis flag 'weather-drm')");
    }
    return std::stoi(*flag);
  }

  Position
  TrcWeather::roll(const Ctx& ctx, HexEngine::PrngStreams& streams, HexEngine::EventSink& sink) const
  {
    Position next = ctx.position;
    const Months months = monthsOf(ctx.position.clock().turn);
    if (Months::MayJun == months || Months::JulAug == months || Months::JanFeb == months) {
      const Weather fixed = chart(months, 0);
      next.setFlag(facts_.axis(), kWeather, std::string(weatherName(fixed)));
      sink.onEvent(HexEngine::GameEvent{"weather", std::string(weatherName(fixed)) + " (fixed)"});
      return next;
    }
    const int running = drm(ctx.position);
    const int die = HexEngine::rollDie(streams, HexEngine::StreamTag::Weather, 6);
    const int modified = std::clamp(die + running, 0, 7);
    const Weather rolled = chart(months, modified);
    next.setFlag(facts_.axis(), kWeather, std::string(weatherName(rolled)));
    next.setFlag(facts_.axis(), kDrm, std::to_string(running + drmChange(rolled)));
    sink.onEvent(HexEngine::DieRolled{HexEngine::StreamTag::Weather, die});
    sink.onEvent(HexEngine::GameEvent{"weather", describe(rolled, die, running, modified)});
    return next;
  }

}  // namespace Trc
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------

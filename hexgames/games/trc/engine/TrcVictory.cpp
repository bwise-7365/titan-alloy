// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
#include "TrcVictory.h"

#include "TrcMarkers.h"
#include "TrcState.h"

#include <stdexcept>

namespace Trc {

  namespace {

    // TODO(decide): the rules say "German cities", and the sheet carries no country membership
    // (see TrcFacts::dataGaps); these are the four cities the sheet draws inside Germany's 1941
    // borders, East Prussia included.
    const std::vector<std::string_view> kGermanCities{"Berlin", "Posen", "Breslau", "Königsberg"};
    const std::vector<std::string_view> kOil{"Ploesti Oil Fields", "Maikop Oil Fields", "Grozny Oil Fields"};

    struct Objectives {
      int turn;
      const char* condition;
      std::vector<std::string_view> hexes;
    };
    const std::vector<Objectives> kSixCities{
        {5, "sudden-death-1942", {"Kiev", "Kalinin", "Leningrad", "Rostov", "Kharkov", "Stalino"}},
        {11, "sudden-death-1943", {"Maikop Oil Fields", "Moscow", "Stalingrad", "Kursk", "Leningrad", "Rostov"}},
        {17, "sudden-death-1944", {"Leningrad", "Smolensk", "Kiev", "Dnepropetrovsk", "Sevastopol", "Kharkov"}},
    };

  }  // namespace

  TrcVictory::TrcVictory(const TrcFacts& facts) : facts_(facts)
  {
  }

  std::span<const std::string_view>
  TrcVictory::claims() const
  {
    return {};
  }

  bool
  TrcVictory::holdsAllP(const Ctx& ctx, SideId side, const std::vector<std::string_view>& names) const
  {
    for (std::string_view name : names) {
      if (side != ctx.position.control(facts_.named(name))) {
        return false;
      }
    }
    return true;
  }

  bool
  TrcVictory::holdsAnyP(const Ctx& ctx, SideId side, const std::vector<std::string_view>& names) const
  {
    for (std::string_view name : names) {
      if (side == ctx.position.control(facts_.named(name))) {
        return true;
      }
    }
    return false;
  }

  bool
  TrcVictory::surrenderedP(const Ctx& ctx, std::string_view nation) const
  {
    return stateOf(ctx.position).surrendered.contains(nationNamed(nation));
  }

  std::optional<HexEngine::Outcome>
  TrcVictory::suddenDeath(const Ctx& ctx) const
  {
    const int turn = ctx.position.clock().turn;
    for (const Objectives& year : kSixCities) {
      if (year.turn != turn) {
        continue;
      }
      for (SideId side : {facts_.axis(), facts_.russian()}) {
        if (holdsAllP(ctx, side, year.hexes)) {
          return HexEngine::Outcome{side, year.condition};
        }
      }
    }
    if (23 == turn) {
      if (surrenderedP(ctx, "finnish") && surrenderedP(ctx, "rumanian") && surrenderedP(ctx, "hungarian") &&
          holdsAnyP(ctx, facts_.russian(), kGermanCities) && holdsAllP(ctx, facts_.russian(), kOil)) {
        return HexEngine::Outcome{facts_.russian(), "sudden-death-1945-russian"};
      }
      if (!surrenderedP(ctx, "rumanian") && !surrenderedP(ctx, "hungarian") &&
          holdsAllP(ctx, facts_.axis(), kGermanCities) && holdsAnyP(ctx, facts_.axis(), kOil)) {
        return HexEngine::Outcome{facts_.axis(), "sudden-death-1945-axis"};
      }
    }
    return std::nullopt;
  }

  Position
  TrcVictory::recordSuddenDeath(const Ctx& ctx, HexEngine::EventSink& sink) const
  {
    Position next = ctx.position;
    if (const std::optional<HexEngine::Outcome> met = suddenDeath(ctx)) {
      stateOf(next).suddenDeath = SuddenDeathMet{met->condition, *met->winner};
      sink.onEvent(HexEngine::VictoryDeclared{met->winner, met->condition});
    }
    return next;
  }

  std::optional<HexEngine::Outcome>
  TrcVictory::check(const Ctx& ctx) const
  {
    if (facts_.russian() == ctx.position.control(facts_.named("Berlin"))) {
      return HexEngine::Outcome{facts_.russian(), "russian-berlin"};
    }
    const std::optional<Location>& stalin = ctx.position.unit(facts_.stalin()).where;
    const bool stalinGoneP = !stalin || !std::holds_alternative<HexIndex>(*stalin);
    if (facts_.axis() == ctx.position.control(facts_.named("Moscow")) && stalinGoneP) {
      return HexEngine::Outcome{facts_.axis(), "axis-moscow-stalin"};
    }
    if (const std::optional<SuddenDeathMet>& met = stateOf(ctx.position).suddenDeath) {
      return HexEngine::Outcome{met->winner, met->condition};
    }
    if (25 < ctx.position.clock().turn) {
      return HexEngine::Outcome{facts_.axis(), "axis-berlin-held"};
    }
    return std::nullopt;
  }

}  // namespace Trc
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------

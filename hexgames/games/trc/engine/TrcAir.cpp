// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
#include "TrcAir.h"

#include "TrcFlags.h"

#include <array>
#include <stdexcept>

namespace Trc {

  namespace {

    constexpr std::array<std::string_view, 1> kClaims{"air-range"};

    struct Cell {
      int stukas;
      int sturmoviks;
    };
    // Rows 1941..1945, columns clear, light mud, mud. 1941-1944 from the sheet's Airpower
    // Availability panel; 1945 from game_rules/the-russian-campaign.md 2.8 ("Sturmoviks rising to
    // 3/1/1 by 1945"; Stukas "none afterwards").
    constexpr std::array<std::array<Cell, 3>, 5> kChart{{
        {{{3, 0}, {1, 0}, {1, 0}}},
        {{{2, 0}, {1, 0}, {1, 0}}},
        {{{1, 1}, {0, 0}, {0, 0}}},
        {{{0, 2}, {0, 1}, {0, 0}}},
        {{{0, 3}, {0, 1}, {0, 1}}},
    }};

  }  // namespace

  TrcAir::TrcAir(const TrcFacts& facts, const TrcWeather& weather) : facts_(facts), weather_(weather)
  {
  }

  std::span<const std::string_view>
  TrcAir::claims()
  {
    return kClaims;
  }

  int
  TrcAir::available(bool axisP, int year, Weather weather)
  {
    std::size_t column = 0;
    switch (weather) {
      case Weather::Clear:
        column = 0;
        break;
      case Weather::LightMud:
        column = 1;
        break;
      case Weather::Mud:
        column = 2;
        break;
      case Weather::Snow:
        return 0;
    }
    const std::size_t row = static_cast<std::size_t>(std::clamp(year, 1941, 1945) - 1941);
    return axisP ? kChart[row][column].stukas : kChart[row][column].sturmoviks;
  }

  int
  TrcAir::used(const Position& position, SideId side)
  {
    return Flags::counter(position, side, Flags::kAirUsed);
  }

  bool
  TrcAir::inRangeP(const Ctx& ctx, SideId side, HexIndex target) const
  {
    const auto withinP = [&](UnitId anchor) {
      const std::optional<Location>& where = ctx.position.unit(anchor).where;
      return where && std::holds_alternative<HexIndex>(*where) &&
             8 >= ctx.board.distance(std::get<HexIndex>(*where), target);
    };
    if (facts_.russian() == side) {
      const std::optional<Location>& stavka = ctx.position.unit(facts_.stavka()).where;
      const bool stavkaOnMapP = stavka && std::holds_alternative<HexIndex>(*stavka);
      return withinP(stavkaOnMapP ? facts_.stavka() : facts_.stalin());  // 15.8
    }
    for (UnitId unit : ctx.roster.ofSide(side)) {
      if (facts_.germanHqP(unit) && withinP(unit)) {
        return true;
      }
    }
    return false;
  }

  void
  TrcAir::check(const Ctx& ctx, const HexEngine::DeclareAttack& attack) const
  {
    if (attack.modifiers.empty()) {
      return;
    }
    const SideId side = ctx.roster.unit(attack.attackers.front()).side;
    const ModifierId own = facts_.axis() == side ? facts_.stuka() : facts_.sturmovik();
    int count = 0;
    for (ModifierId id : attack.modifiers) {
      if (own != id) {
        throw std::invalid_argument("TrcAir: modifier '" + ctx.rules.combat().modifiers[id.value].id +
                                     "' may not be declared by this side (15.5, 15.7; optional artillery 26.5 is not "
                                     "implemented; terrain and supply are computed, never declared)");
      }
      ++count;
    }
    const PhaseInfo phase = facts_.phase(ctx.position.clock().phase);
    if (PhaseKind::Combat != phase.kind || Impulse::First != phase.impulse) {
      throw std::invalid_argument("TrcAir: air power is committed in the first-impulse combat phase only (15.1)");
    }
    if (facts_.axis() == side && 1 < count) {
      throw std::invalid_argument("TrcAir: one Stuka per attack (15.5)");
    }
    const int allowed = available(facts_.axis() == side, yearOf(ctx.position.clock().turn),
                                  weather_.current(ctx.position));
    if (allowed < used(ctx.position, side) + count) {
      throw std::invalid_argument("TrcAir: the Airpower Availability chart allows " + std::to_string(allowed) +
                                   " this impulse (15.2)");
    }
    if (facts_.axis() == side) {
      int hqs = 0;
      for (UnitId unit : ctx.roster.ofSide(side)) {
        const std::optional<Location>& where = ctx.position.unit(unit).where;
        hqs += facts_.germanHqP(unit) && where && std::holds_alternative<HexIndex>(*where) ? 1 : 0;
      }
      if (hqs < used(ctx.position, side) + count) {
        throw std::invalid_argument("TrcAir: each German Army Group HQ supports one Stuka (15.5)");
      }
    }
    if (!inRangeP(ctx, side, attack.target)) {
      throw std::invalid_argument("TrcAir: the defenders are not within eight hexes of a friendly HQ (15.2)");
    }
    return;
  }

}  // namespace Trc
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------

// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
#include "PggSchedule.h"

namespace Pgg {

  namespace {

    constexpr DivisionKind kPz = DivisionKind::Panzer;
    constexpr DivisionKind kMot = DivisionKind::Motorized;
    constexpr DivisionKind kInf = DivisionKind::Infantry;

  }  // namespace

  const std::vector<GermanArrival>&
  germanSchedule()
  {
    static const std::vector<GermanArrival> schedule{
        // Game-Turn One, Area C: 39th Panzer Corps
        {{kPz, 7}, 1, Area::C}, {{kPz, 12}, 1, Area::C}, {{kPz, 20}, 1, Area::C},
        {{kMot, 14}, 1, Area::C}, {{kMot, 20}, 1, Area::C},
        // Game-Turn Two, Area A: 57th Panzer Corps; Area B: 6th and 26th (errata) Infantry
        {{kMot, 18}, 2, Area::A}, {{kPz, 19}, 2, Area::A}, {{DivisionKind::Independent, 1}, 2, Area::A},
        {{kInf, 6}, 2, Area::B}, {{kInf, 26}, 2, Area::B},
        // Game-Turn Three, Area D: infantry; Area E: 47th and 46th Panzer Corps; Area H: 24th Panzer Corps
        {{kInf, 5}, 3, Area::D}, {{kInf, 35}, 3, Area::D}, {{kInf, 161}, 3, Area::D},
        {{kMot, 29}, 3, Area::E}, {{kPz, 17}, 3, Area::E}, {{kPz, 18}, 3, Area::E},
        {{kPz, 10}, 3, Area::E}, {{DivisionKind::DasReich, 0}, 3, Area::E}, {{DivisionKind::Independent, 0}, 3, Area::E},
        {{kPz, 3}, 3, Area::H}, {{kPz, 4}, 3, Area::H}, {{kMot, 10}, 3, Area::H}, {{DivisionKind::Cavalry, 1}, 3, Area::H},
        // Game-Turns Five to Nine: infantry (the 268th added on Seven by errata)
        {{kInf, 263}, 5, Area::E}, {{kInf, 137}, 5, Area::E}, {{kInf, 23}, 5, Area::E},
        {{kInf, 258}, 6, Area::E}, {{kInf, 292}, 6, Area::E},
        {{kInf, 7}, 7, Area::E}, {{kInf, 15}, 7, Area::E}, {{kInf, 17}, 7, Area::E}, {{kInf, 268}, 7, Area::E},
        {{kInf, 252}, 8, Area::F}, {{kInf, 78}, 8, Area::F},
        {{kInf, 31}, 9, Area::G}, {{kInf, 34}, 9, Area::G},
    };
    return schedule;
  }

  const std::vector<SovietArrival>&
  sovietSchedule()
  {
    static const std::vector<SovietArrival> schedule{
        {1, Area::X, 6, 0, "s1-rokovski-reserve-hq-5-10"},
        {1, Area::V, 5, 0, "s1-yrshakov-22nd-army-hq-3-10"},
        {2, Area::W, 3, 1, "s1-khomenko-30th-army-hq-3-10"},
        {2, Area::X, 4, 0, std::nullopt},
        {3, Area::Z, 5, 2, "s1-gramenko-21st-army-hq-2-10"},
        {3, Area::X, 4, 0, std::nullopt},
        {4, Area::X, 5, 1, "s1-kachalov-28th-army-hq-2-10"},
        {5, Area::X, 4, 1, std::nullopt},
        {6, Area::X, 4, 0, std::nullopt},
        {7, Area::X, 4, 0, "s1-maslikev-29th-army-hq-3-10"},
        {8, Area::X, 4, 0, "s1-dolmatov-31st-army-hq-2-10"},
        {9, Area::X, 4, 0, "s1-zkhatkin-49th-army-hq-3-10"},
        {10, Area::X, 4, 0, "s1-vshnvsky-32nd-army-hq-2-10"},
        {11, Area::X, 4, 0, "s1-onprenko-33rd-army-hq-2-10"},
    };
    return schedule;
  }

  const std::vector<SovietDeployment>&
  sovietDeployment()
  {
    static const std::vector<SovietDeployment> deployment{
        {24, {"4015"}, 5, 1, "s1-rakutin-24th-army-hq-2-10"},
        {16, {"2216"}, 6, 0, "s1-lukin-16th-army-hq-3-10"},
        {19, {"1414"}, 4, 0, "s1-konev-19th-army-hq-5-10"},
        {13, {"0123", "0124", "0125", "0126"}, 7, 1, "s1-remezov-13th-army-hq-4-10"},
        {20, {"0108", "0109", "0110", "0111", "0112", "0113", "0114", "0115"}, 6, 4, "s1-kurchkin-20th-army-hq-4-10"},
    };
    return deployment;
  }

  std::optional<GermanArrival>
  germanArrivalOf(const PggFacts& facts, UnitId unit)
  {
    const std::optional<Division> division = facts.division(unit);
    if (!division) {
      return std::nullopt;
    }
    for (const GermanArrival& arrival : germanSchedule()) {
      if (arrival.division == *division) {
        return arrival;
      }
    }
    return std::nullopt;
  }

  std::optional<SovietArrival>
  leaderArrivalOf(const PggFacts& facts, UnitId unit)
  {
    const std::string& id = facts.definition().roster->unit(unit).counter.text;
    for (const SovietArrival& arrival : sovietSchedule()) {
      if (arrival.leader && id == *arrival.leader) {
        return arrival;
      }
    }
    return std::nullopt;
  }

}  // namespace Pgg
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------

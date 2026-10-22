// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
// The reinforcement schedules of section 16 with the errata (the 26th, not the 16th, Infantry
// Division on Game-Turn Two; the 268th added on Game-Turn Seven; no Soviet schedule on Game-Turn
// Twelve) and the Soviet initial deployment of 5.1. Transcribed from the rulebook text; the counters'
// own arrival codes ("3H", "2B") agree with them.
// ----------------------------------------------
#pragma once
#include "PggFacts.h"

#include <optional>
#include <string_view>
#include <vector>

namespace Pgg {

  struct GermanArrival {
    Division division;
    int turn = 1;
    Area area;
  };
  const std::vector<GermanArrival>& germanSchedule();

  struct SovietArrival {
    int turn = 1;
    Area area;
    int rifles = 0;
    int armour = 0;
    std::optional<std::string_view> leader;  // the Leader's counter id
  };
  const std::vector<SovietArrival>& sovietSchedule();

  struct SovietDeployment {
    int army = 0;
    std::vector<std::string_view> hexes;
    int rifles = 0;
    int armour = 0;
    std::string_view leader;
  };
  const std::vector<SovietDeployment>& sovietDeployment();

  // When and where a German counter or a scheduled Soviet Leader enters; nullopt for any other unit.
  std::optional<GermanArrival> germanArrivalOf(const PggFacts&, UnitId);
  std::optional<SovietArrival> leaderArrivalOf(const PggFacts&, UnitId);

}  // namespace Pgg
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------

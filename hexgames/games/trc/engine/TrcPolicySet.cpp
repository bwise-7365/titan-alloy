// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
#include "TrcPolicySet.h"

namespace Trc {

  TrcPolicySet::TrcPolicySet(const HexRules::GameDefinition& definition)
    : names_(*definition.board, *definition.roster, *definition.rules),
      facts_(definition),
      zoc_(facts_),
      weather_(facts_),
      movement_(facts_, zoc_, weather_),
      stacking_(facts_),
      supply_(facts_, zoc_, weather_),
      combat_(facts_, supply_),
      air_(facts_, weather_),
      retreat_(facts_, zoc_),
      control_(facts_, zoc_),
      rail_(facts_, zoc_),
      victory_(facts_),
      mandatory_(facts_, zoc_, combat_),
      arrivals_(facts_, zoc_, weather_),
      special_(facts_, zoc_, weather_, combat_),
      politics_(facts_, zoc_),
      phases_(facts_),
      grammar_(facts_, names_),
      game_(TrcParts{facts_, zoc_, weather_, stacking_, supply_, combat_, air_, control_, rail_, victory_, mandatory_,
                     arrivals_, special_, politics_})
  {
    policies_.zoc = &zoc_;
    policies_.movement = &movement_;
    policies_.supply = &supply_;
    policies_.combat = &combat_;
    policies_.retreat = &retreat_;
    policies_.stacking = &stacking_;
    policies_.victory = &victory_;
    policies_.phases = &phases_;
    policies_.grammar = &grammar_;
    policies_.game = &game_;
  }

  std::vector<std::string_view>
  TrcPolicySet::claims() const
  {
    std::vector<std::string_view> out;
    const auto add = [&out](std::span<const std::string_view> claimed) {
      out.insert(out.end(), claimed.begin(), claimed.end());
      return;
    };
    add(zoc_.claims());
    add(TrcWeather::claims());
    add(movement_.claims());
    add(stacking_.claims());
    add(supply_.claims());
    add(combat_.claims());
    add(TrcAir::claims());
    add(retreat_.claims());
    add(TrcControl::claims());
    add(TrcRail::claims());
    add(victory_.claims());
    add(TrcMandatory::claims());
    add(TrcArrivals::claims());
    add(TrcSpecialMoves::claims());
    add(TrcPolitics::claims());
    add(phases_.claims());
    add(game_.claims());
    return out;
  }

}  // namespace Trc
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------

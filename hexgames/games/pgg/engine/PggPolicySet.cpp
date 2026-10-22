// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
#include "PggPolicySet.h"

namespace Pgg {

  PggPolicySet::PggPolicySet(const HexRules::GameDefinition& definition)
    : names_(*definition.board, *definition.roster, *definition.rules),
      facts_(definition),
      zoc_(facts_),
      movement_(facts_, zoc_),
      supply_(facts_, zoc_),
      stacking_(facts_),
      combat_(facts_, supply_),
      retreat_(facts_, zoc_),
      obligations_(facts_, retreat_, stacking_),
      interdiction_(facts_, supply_),
      arrivals_(facts_, movement_, zoc_),
      firstTurn_(facts_, movement_, stacking_),
      victory_(facts_, supply_),
      overrun_(facts_, movement_, combat_),
      phases_(facts_),
      grammar_(facts_, names_),
      state_(facts_, names_),
      battleCodec_(names_)
  {
    HexEngine::registerEngineSteps(steps_);
    registerPggSteps(steps_, PggParts{facts_, zoc_, movement_, supply_, stacking_, combat_, interdiction_, arrivals_,
                                      firstTurn_, victory_, overrun_});
    policies_.steps = &steps_;
    policies_.zoc = &zoc_;
    policies_.movement = &movement_;
    policies_.supply = &supply_;
    policies_.combat = &combat_;
    policies_.retreat = &retreat_;
    policies_.stacking = &stacking_;
    policies_.victory = &victory_;
    policies_.phases = &phases_;
    policies_.grammar = &grammar_;
    policies_.obligations = &obligations_;
    policies_.state = &state_;
    policies_.obligationCodec = &battleCodec_;
  }

  std::vector<std::string_view>
  PggPolicySet::claims() const
  {
    std::vector<std::string_view> out;
    const auto add = [&out](std::span<const std::string_view> claimed) {
      out.insert(out.end(), claimed.begin(), claimed.end());
      return;
    };
    add(zoc_.claims());
    add(movement_.claims());
    add(supply_.claims());
    add(stacking_.claims());
    add(combat_.claims());
    add(retreat_.claims());
    add(obligations_.claims());
    add(PggInterdiction::claims());
    add(PggArrivals::claims());
    add(PggFirstTurn::claims());
    add(victory_.claims());
    add(PggOverrun::claims());
    add(phases_.claims());
    const std::vector<std::string_view> stepRules = steps_.claims(*facts_.definition().rules);
    out.insert(out.end(), stepRules.begin(), stepRules.end());
    return out;
  }

}  // namespace Pgg
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------

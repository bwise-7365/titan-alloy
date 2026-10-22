// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
// PGG's ObligationPolicy: settles and answers the PggBattle on top of the resolution stack, one
// transition at a time (PggBattleFlow.h), and pops it once it is done.
// ----------------------------------------------
#pragma once
#include "PggBattle.h"
#include "PggRetreat.h"
#include "PggStacking.h"

namespace Pgg {

  class PggObligations : public HexEngine::ObligationPolicy {
  public:
    PggObligations(const PggFacts&, const PggRetreat&, const PggStacking&);
    std::span<const std::string_view> claims() const override;
    Position settle(const Ctx&, HexEngine::PrngStreams&, HexEngine::EventSink&) const override;
    Position answer(const Ctx&, const HexEngine::DecisionAnswer&, HexEngine::PrngStreams&,
                    HexEngine::EventSink&) const override;

  private:
    const PggFacts& facts_;
    const PggRetreat& retreat_;
    const PggStacking& stacking_;
  };

}  // namespace Pgg
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------

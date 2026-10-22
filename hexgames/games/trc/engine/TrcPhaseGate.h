// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
// What each stop of TRC's sequence of play allows (4.1): first-impulse movement phases grant
// movement, rail and sea movement, reinforcement and replacement; second-impulse ones movement,
// reinforcement and replacement but no rail or sea (8.4); combat phases combat; end phases
// administration and, for the Russian, partisan placement; the weather and sudden-death stops
// nothing but ending them. Every stop is active; the rules document's turn selectors do the rest.
// ----------------------------------------------
#pragma once
#include "TrcFacts.h"

namespace Trc {

  class TrcPhaseGate : public HexEngine::PhaseGate {
  public:
    explicit TrcPhaseGate(const TrcFacts&);
    std::span<const std::string_view> claims() const override;
    bool activeP(const Ctx&, PhaseId) const override;
    HexEngine::PhaseCaps capsFor(PhaseId) const override;

  private:
    const TrcFacts& facts_;
  };

}  // namespace Trc
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------

// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
// What each PGG phase grants (4.2): movement phases grant movement and reinforcement (and the Soviet
// one rail movement), combat phases combat, and the set-up, disruption-removal and interdiction phases
// administration only. Every phase is active on every turn its rules document gives it.
// ----------------------------------------------
#pragma once
#include "PggFacts.h"

namespace Pgg {

  class PggPhaseGate : public HexEngine::PhaseGate {
  public:
    explicit PggPhaseGate(const PggFacts&);
    std::span<const std::string_view> claims() const override;
    bool activeP(const Ctx&, PhaseId) const override;
    HexEngine::PhaseCaps capsFor(PhaseId) const override;

  private:
    const PggFacts& facts_;
  };

}  // namespace Pgg
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------

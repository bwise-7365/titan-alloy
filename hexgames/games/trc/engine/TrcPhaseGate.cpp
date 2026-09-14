// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
#include "TrcPhaseGate.h"

#include <initializer_list>

namespace Trc {

  namespace {

    using HexEngine::Cap;

    HexEngine::PhaseCaps
    capsOf(std::initializer_list<Cap> caps)
    {
      HexEngine::PhaseCaps out;
      for (Cap cap : caps) {
        out.set(static_cast<std::size_t>(cap));
      }
      out.set(static_cast<std::size_t>(Cap::Decision));
      return out;
    }

  }  // namespace

  TrcPhaseGate::TrcPhaseGate(const TrcFacts& facts) : facts_(facts)
  {
  }

  std::span<const std::string_view>
  TrcPhaseGate::claims() const
  {
    return {};
  }

  bool
  TrcPhaseGate::activeP(const Ctx&, PhaseId) const
  {
    return true;
  }

  HexEngine::PhaseCaps
  TrcPhaseGate::capsFor(PhaseId phase) const
  {
    const PhaseInfo info = facts_.phase(phase);
    switch (info.kind) {
      case PhaseKind::Weather:
        return capsOf({Cap::Weather});
      case PhaseKind::Move:
        if (Impulse::First == info.impulse) {
          return capsOf({Cap::Move, Cap::RailMove, Cap::SeaMove, Cap::Reinforce, Cap::Replace});
        }
        return capsOf({Cap::Move, Cap::Reinforce, Cap::Replace});
      case PhaseKind::Combat:
        return capsOf({Cap::Combat});
      case PhaseKind::End:
        return capsOf({Cap::Admin, Cap::Reinforce});
      case PhaseKind::SuddenDeath:
        return capsOf({Cap::Admin});
    }
    throw std::invalid_argument("TrcPhaseGate: phase kind outside TRC's sequence");
  }

}  // namespace Trc
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------

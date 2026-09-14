// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
#include "PggPhaseGate.h"

#include <initializer_list>
#include <stdexcept>

namespace Pgg {

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

  PggPhaseGate::PggPhaseGate(const PggFacts& facts) : facts_(facts)
  {
  }

  std::span<const std::string_view>
  PggPhaseGate::claims() const
  {
    return {};
  }

  bool
  PggPhaseGate::activeP(const Ctx&, PhaseId) const
  {
    return true;
  }

  HexEngine::PhaseCaps
  PggPhaseGate::capsFor(PhaseId phase) const
  {
    const PhaseInfo info = facts_.phase(phase);
    switch (info.kind) {
      case PhaseKind::Move:
        return facts_.soviet() == info.side ? capsOf({Cap::Move, Cap::RailMove, Cap::Reinforce})
                                            : capsOf({Cap::Move, Cap::Reinforce});
      case PhaseKind::MechanizedMove:
        return capsOf({Cap::Move});
      case PhaseKind::Combat:
        return capsOf({Cap::Combat});
      case PhaseKind::SetUp:
      case PhaseKind::DisruptionRemoval:
      case PhaseKind::SovietInterdiction:
      case PhaseKind::AirInterdiction:
        return capsOf({Cap::Admin});
      case PhaseKind::Container:
        break;
    }
    throw std::invalid_argument("PggPhaseGate: phase '" + std::to_string(phase.value) +
                                "' contains other phases and is never a stop of its own");
  }

}  // namespace Pgg
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------

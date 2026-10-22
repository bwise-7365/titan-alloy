// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
// Place and the PGG verbs, registered under their grammar verbs.
// ----------------------------------------------
#include "PggSteps.h"

namespace Pgg {

  namespace {

    using HexEngine::CommandCall;

    const HexEngine::GameCommand&
    gameCommandOf(const CommandCall& call)
    {
      if (const HexEngine::GameCommand* game = std::get_if<HexEngine::GameCommand>(&call.command)) {
        return *game;
      }
      throw std::invalid_argument("Pgg: a PGG verb was registered for a command that is not a game command");
    }

  }  // namespace

  void
  registerPggVerbs(HexEngine::StepRegistry& registry, const PggParts& parts)
  {
    registry.addCommand("place", [parts](const CommandCall& call) {
      const HexEngine::Place* place = std::get_if<HexEngine::Place>(&call.command);
      if (nullptr == place) {
        throw std::invalid_argument("Pgg: 'place' was registered for a command that is not a placement");
      }
      return parts.arrivals.place(call.ctx, *place, call.sink);
    });
    registry.addCommand("reinforce", [parts](const CommandCall& call) {
      return parts.arrivals.reinforce(call.ctx, gameCommandOf(call), call.streams, call.sink);
    });
    registry.addCommand("overrun", [parts](const CommandCall& call) { return parts.overrun.overrun(call); });
    registry.addCommand("interdict", [parts](const CommandCall& call) {
      return parts.interdiction.interdict(call.ctx, gameCommandOf(call), call.sink);
    });
    registry.addCommand("cut-rail", [parts](const CommandCall& call) {
      return parts.interdiction.cutRail(call.ctx, gameCommandOf(call), call.sink);
    });
    registry.addCommand("lift-cut", [parts](const CommandCall& call) {
      return parts.interdiction.liftCut(call.ctx, gameCommandOf(call), call.sink);
    });
    return;
  }

  void
  registerPggSteps(HexEngine::StepRegistry& registry, const PggParts& parts)
  {
    registerPggCommandSteps(registry, parts);
    registerPggPhaseSteps(registry, parts);
    registerPggVerbs(registry, parts);
    return;
  }

}  // namespace Pgg
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------

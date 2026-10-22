// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
// Place and the TRC verbs, registered under their grammar verbs.
// ----------------------------------------------
#include "TrcSteps.h"

namespace Trc {

  namespace {

    using HexEngine::CommandCall;

    const HexEngine::GameCommand&
    gameCommandOf(const CommandCall& call)
    {
      if (const HexEngine::GameCommand* game = std::get_if<HexEngine::GameCommand>(&call.command)) {
        return *game;
      }
      throw std::invalid_argument("Trc: a TRC verb was registered for a command that is not a game command");
    }

  }  // namespace

  void
  registerTrcVerbs(HexEngine::StepRegistry& registry, const TrcParts& parts)
  {
    registry.addCommand("place", [parts](const CommandCall& call) {
      const HexEngine::Place* place = std::get_if<HexEngine::Place>(&call.command);
      if (nullptr == place) {
        throw std::invalid_argument("Trc: 'place' was registered for a command that is not a placement");
      }
      return parts.arrivals.place(call.ctx, *place, call.sink);
    });
    registry.addCommand("sea-move", [parts](const CommandCall& call) {
      return parts.special.seaMove(call.ctx, gameCommandOf(call), call.streams, call.sink);
    });
    registry.addCommand("paradrop", [parts](const CommandCall& call) {
      return parts.special.paradrop(call.ctx, gameCommandOf(call), call.sink);
    });
    registry.addCommand("av-attack", [parts](const CommandCall& call) {
      return parts.special.automaticVictory(call.ctx, gameCommandOf(call), call.sink);
    });
    return;
  }

  void
  registerTrcSteps(HexEngine::StepRegistry& registry, const TrcParts& parts)
  {
    registerTrcCommandSteps(registry, parts);
    registerTrcPhaseSteps(registry, parts);
    registerTrcVerbs(registry, parts);
    return;
  }

}  // namespace Trc
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------

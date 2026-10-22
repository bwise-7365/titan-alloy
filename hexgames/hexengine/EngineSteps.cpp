// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
// The behaviours the engine registers under its own names, for any rules document to place.
// ----------------------------------------------
#include "hexengine/Adjudicators.h"
#include "hexengine/Steps.h"

#include <stdexcept>
#include <string>

namespace HexEngine {

  namespace {

    std::string
    rulesOf(const HexRules::Step& step)
    {
      std::string out;
      for (const HexRules::RuleId& rule : step.rules) {
        out += (out.empty() ? "" : " ") + rule.text;
      }
      return out.empty() ? std::string("none named") : out;
    }

  }  // namespace

  void
  registerEngineSteps(StepRegistry& registry)
  {
    registry.addEffect("stacking-repair", [](const StepCall& call) {
      return Adjudicators::applyStackingRepair(call.ctx, call.policies, call.sink);
    });
    registry.addEffect("supply-check", [](const StepCall& call) {
      const std::optional<SideId> acting = call.ctx.position.clock().actingSide;
      if (!acting) {
        throw std::invalid_argument("step '" + call.step.id + "': supply-check needs an acting side, and no side acts");
      }
      return Adjudicators::applySupplyCheck(call.ctx, call.policies, call.scratch, *acting, call.sink);
    });
    registry.addCheck("refuse", [](const CheckCall& call) {
      throw std::invalid_argument("Session::apply: step '" + call.step.id + "' refuses '" +
                                  call.policies.grammar->verb(call.command) + "' in this phase (rules: " +
                                  rulesOf(call.step) + ")");
    });
    return;
  }

}  // namespace HexEngine
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------

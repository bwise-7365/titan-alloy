// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
#include "PggAgent.h"

#include "PggUnits.h"

#include <algorithm>
#include <stdexcept>
#include <tuple>
#include <variant>

namespace Pgg {

  PggAgent::PggAgent(const PggPolicySet& set, SideId side, HexAi::Tactic tactic)
    : set_(set), side_(side), tactic_(std::move(tactic))
  {
    if (seizingP()) {
      buildCityFields();
    }
  }

  void
  PggAgent::startPhase(const HexEngine::Session& session)
  {
    const HexModel::TurnClock& clock = session.position().clock();
    if (clock.turn == turn_ && clock.phase == phase_) {
      return;
    }
    turn_ = clock.turn;
    phase_ = clock.phase;
    memory_ = PhaseMemory{};
    const Ctx ctx = session.context();
    std::vector<HexIndex> enemyHexes;
    for (UnitId unit : Units::onMap(ctx, set_.facts(), set_.facts().enemy(side_))) {
      enemyHexes.push_back(*Units::hexOf(ctx.position, unit));
    }
    field_.emplace(ctx.board, enemyHexes, [this](HexIndex hex, HexModel::Direction direction) { return crossableP(hex, direction); });
    return;
  }

  bool
  PggAgent::crossableP(HexIndex hex, HexModel::Direction direction) const
  {
    const std::optional<HexIndex> to = set_.facts().definition().board->neighbour(hex, direction);
    return to && !set_.facts().lakeP(*to);
  }

  bool
  PggAgent::roomP(const Ctx& ctx, HexIndex hex, UnitId unit) const
  {
    std::vector<UnitId> units;
    for (UnitId there : ctx.position.unitsAt(hex)) {
      if (!set_.facts().markerP(there)) {
        units.push_back(there);
      }
    }
    units.push_back(unit);
    return set_.stacking().legalP(units);
  }

  HexEngine::Command
  PggAgent::choose(const HexEngine::Session& session, SideId side)
  {
    if (side != side_) {
      throw std::invalid_argument("PggAgent: asked to play the other side");
    }
    startPhase(session);
    const Ctx ctx = session.context();
    const PggSeen seen(set_, ctx);
    if (session.prompt().decisionPendingP) {
      if (const HexModel::GameChoice* choice = std::get_if<HexModel::GameChoice>(&ctx.position.pending())) {
        const bool advanceP = "advance" == choice->verb || "advance-on" == choice->verb;
        return HexEngine::DecisionAnswer{choice->verb, seizingP() && advanceP ? seizeAdvance(ctx, seen, *choice) : answerChoice(seen, *choice)};
      }
      return answer(session, seen);
    }
    const PhaseInfo phase = set_.facts().phase(ctx.position.clock().phase);
    std::optional<HexEngine::Command> next;
    switch (phase.kind) {
      case PhaseKind::Move:
        next = nextArrival(session);
        next = next ? next : nextReinforcement(session);
        next = next ? next : nextMove(session, seen);
        break;
      case PhaseKind::MechanizedMove:
        next = nextMove(session, seen);
        break;
      case PhaseKind::Combat:
        next = nextAttack(session, seen);
        break;
      case PhaseKind::SetUp:
      case PhaseKind::AirInterdiction:
        next = nextInterdiction(session);
        break;
      case PhaseKind::DisruptionRemoval:
      case PhaseKind::SovietInterdiction:
      case PhaseKind::Container:
        break;
    }
    return next ? *next : HexEngine::Command{HexEngine::EndPhase{}};
  }

  void
  PggAgent::refused(const HexEngine::Command& command, const std::string&)
  {
    constexpr std::size_t kTries = 3;  // destinations tried for one unit, as the plan has it
    if (const HexEngine::MoveUnit* move = std::get_if<HexEngine::MoveUnit>(&command)) {
      const UnitId unit = move->units.front();
      memory_.hexes[unit].insert(move->path.back());
      if (kTries > memory_.hexes[unit].size()) {
        for (UnitId mover : move->units) {
          memory_.considered.erase(mover);
        }
      }
    } else if (const HexEngine::DeclareAttack* attack = std::get_if<HexEngine::DeclareAttack>(&command)) {
      memory_.targets.insert(attack->target);
    } else if (const HexEngine::Place* place = std::get_if<HexEngine::Place>(&command)) {
      memory_.arrivals.insert(place->unit);
    } else if (const HexEngine::GameCommand* game = std::get_if<HexEngine::GameCommand>(&command)) {
      if ("reinforce" == game->verb) {
        memory_.entries.insert(game->args[0] + " " + game->args[1]);
      } else {
        memory_.targets.insert(set_.facts().hex(game->args[0]));
      }
    }
    return;
  }

  // A step loss falls on the weakest unit, and a retreat is routed next to as many of this side's units
  // as it can be.
  HexEngine::Command
  PggAgent::answer(const HexEngine::Session& session, const PggSeen& seen) const
  {
    const Ctx ctx = session.context();
    if (const HexModel::ChooseLoss* loss = std::get_if<HexModel::ChooseLoss>(&ctx.position.pending())) {
      const auto weakest = std::min_element(loss->candidates.begin(), loss->candidates.end(), [&](UnitId l, UnitId r) {
        return std::make_tuple(seen.defence(l), l.value) < std::make_tuple(seen.defence(r), r.value);
      });
      return HexEngine::DecisionAnswer{"loss", ctx.roster.unit(*weakest).counter.text};
    }
    if (const HexModel::ChooseRetreat* retreat = std::get_if<HexModel::ChooseRetreat>(&ctx.position.pending())) {
      std::optional<HexIndex> best;
      int bestCount = -1;
      for (HexIndex hex : retreat->candidates) {
        int count = 0;
        for (int d = 0; d < HexCoord::kDirections; ++d) {
          const std::optional<HexIndex> near = ctx.board.neighbour(hex, static_cast<HexModel::Direction>(d));
          count += near && Units::friendlyAtP(ctx, *near, side_) ? 1 : 0;
        }
        if (bestCount < count) {
          best = hex;
          bestCount = count;
        }
      }
      return HexEngine::DecisionAnswer{"retreat", best ? ctx.board.id(*best).text : std::string("stop")};
    }
    throw std::invalid_argument("PggAgent: PGG asks for no decision of this kind");
  }

  std::string
  PggAgent::answerChoice(const PggSeen& seen, const HexModel::GameChoice& choice)
  {
    if ("advance-on" == choice.verb) {
      return "stop";
    }
    if ("advance" == choice.verb && advancedP_) {
      advancedP_ = false;
      return "done";
    }
    if ("result" == choice.verb && choice.options.end() != std::find(choice.options.begin(), choice.options.end(), "retreat")) {
      return "retreat";
    }
    if ("result" != choice.verb && "loss" != choice.verb && "advance" != choice.verb) {
      throw std::invalid_argument("PggAgent: no answer to the decision '" + choice.verb + "'");
    }
    // The options that name counters: "done" comes first in an advance, "retreat" first in a result.
    const bool advanceP = "advance" == choice.verb;
    std::optional<std::string> best;
    std::tuple<double, std::uint32_t> bestKey{};
    for (const std::string& option : choice.options) {
      if ("done" == option || "retreat" == option) {
        continue;
      }
      const UnitId unit = set_.facts().counter(option);
      // An advance takes the strongest attacker, and a step loss falls on the weakest defender.
      const std::tuple<double, std::uint32_t> key{advanceP ? -seen.attack(unit) : seen.defence(unit), unit.value};
      if (!best || key < bestKey) {
        best = option;
        bestKey = key;
      }
    }
    if (!best) {
      throw std::invalid_argument("PggAgent: the decision '" + choice.verb + "' offers no counter");
    }
    advancedP_ = advanceP;
    return *best;
  }

}  // namespace Pgg
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------

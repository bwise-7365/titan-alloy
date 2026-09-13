// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
// The Session: validate, adjudicate into a copy, then swap. An illegal command throws before any
// adjudicator runs, so the Position is untouched and nothing reaches the log or the sinks.
// ----------------------------------------------
#include "hexengine/Session.h"

#include "hexengine/Adjudicators.h"
#include "hexengine/Defaults.h"
#include "hexengine/GameNames.h"
#include "hexengine/PhaseCursor.h"

#include <algorithm>
#include <set>
#include <stdexcept>
#include <string>
#include <type_traits>

namespace HexEngine {

  namespace {

    // Collects what an adjudicator writes, so that nothing is logged until the whole command has
    // succeeded.
    class RecordingSink : public EventSink {
    public:
      explicit RecordingSink(std::vector<Event>& into) : into_(into) {}
      void
      onEvent(const Event& event) override
      {
        into_.push_back(event);
        return;
      }

    private:
      std::vector<Event>& into_;
    };

    MovementPoints
    asPoints(const Budget& budget)
    {
      if (const MovementPoints* points = std::get_if<MovementPoints>(&budget)) {
        return *points;
      }
      if (const HexCount* hexes = std::get_if<HexCount>(&budget)) {
        return MovementPoints::whole(hexes->value);
      }
      return MovementPoints::whole(std::get<Actions>(budget).value);
    }

    std::optional<HexIndex>
    hexOf(const Position& position, UnitId unit)
    {
      const HexModel::UnitState& state = position.unit(unit);
      if (!state.where || !std::holds_alternative<HexIndex>(*state.where)) {
        return std::nullopt;
      }
      return std::get<HexIndex>(*state.where);
    }

  }  // namespace

  Session::Session(std::shared_ptr<const HexRules::GameDefinition> definition, const Policies& policies,
                    Position start, std::uint64_t seed)
    : definition_(std::move(definition)),
      policies_(policies),
      position_(std::move(start)),
      streams_(seed)
  {
    if (nullptr == definition_) {
      throw std::invalid_argument("Session: a session needs a game definition");
    }
  }

  Ctx
  Session::context() const
  {
    return Ctx{*definition_->board, *definition_->rules, *definition_->roster, position_};
  }

  Prompt
  Session::prompt() const
  {
    const HexModel::TurnClock& clock = position_.clock();
    Prompt out;
    out.turn = clock.turn;
    out.phase = clock.phase;
    out.side = clock.actingSide;
    out.decisionPendingP = !std::holds_alternative<HexModel::NoDecision>(position_.pending());
    if (nullptr != policies_.victory) {
      if (const std::optional<Outcome> outcome = policies_.victory->check(context())) {
        out.overP = true;
        out.side = std::nullopt;
      }
    }
    return out;
  }

  std::vector<Command>
  Session::legalCommands() const
  {
    std::vector<Command> out;
    const GameNames names(*definition_->board, *definition_->roster, *definition_->rules);
    const HexModel::PendingDecision& pending = position_.pending();

    if (const HexModel::ChooseLoss* loss = std::get_if<HexModel::ChooseLoss>(&pending)) {
      for (UnitId unit : loss->candidates) {
        out.push_back(DecisionAnswer{"loss", names.counter(unit)});
      }
      return out;
    }
    if (const HexModel::ChooseRetreat* retreat = std::get_if<HexModel::ChooseRetreat>(&pending)) {
      for (HexIndex hex : retreat->candidates) {
        out.push_back(DecisionAnswer{"retreat", names.hex(hex)});
      }
      return out;
    }
    if (const HexModel::GameChoice* choice = std::get_if<HexModel::GameChoice>(&pending)) {
      for (const std::string& option : choice->options) {
        out.push_back(DecisionAnswer{choice->verb, option});
      }
      return out;
    }

    // Ending the phase is always open; unit moves come from reachable(), and reinforcement
    // placements wait on the arrival schedule a game supplies (the Place hook, still empty in M4).
    out.push_back(EndPhase{});
    return out;
  }

  Reachability
  Session::reachable(std::span<const UnitId> units, ModeId mode) const
  {
    if (units.empty()) {
      throw std::invalid_argument("Session::reachable: no unit given");
    }
    if (nullptr == policies_.movement) {
      throw std::invalid_argument("Session::reachable: the policy set has no MovementPolicy");
    }
    const Ctx ctx = context();
    const std::optional<HexIndex> origin = hexOf(position_, units.front());
    if (!origin) {
      throw std::invalid_argument("Session::reachable: counter '" +
                                   definition_->roster->unit(units.front()).counter.text +
                                   "' is not on the map");
    }
    for (UnitId unit : units) {
      if (hexOf(position_, unit) != origin) {
        throw std::invalid_argument("Session::reachable: the units do not share one hex");
      }
    }

    MovementPoints ceiling = asPoints(policies_.movement->allowance(ctx, units.front(), mode));
    for (UnitId unit : units) {
      ceiling = std::min(ceiling, asPoints(policies_.movement->allowance(ctx, unit, mode)));
    }

    const SideId side = definition_->roster->unit(units.front()).side;
    const SideMask enemies = enemiesOf(*definition_->rules, side);
    const auto enemyHeldP = [&](HexIndex hex) {
      for (UnitId occupant : position_.unitsAt(hex)) {
        if (enemies.test(definition_->roster->unit(occupant).side.value)) {
          return true;
        }
      }
      return false;
    };

    const HexSearch::HexAdjacencyGraph graph(*definition_->board,
                                              [](HexIndex, Direction) { return true; });
    const std::function<std::optional<HexSearch::CostHalves>(HexSearch::NodeIndex, std::size_t)> arcCost =
        [&](HexSearch::NodeIndex from, std::size_t arcIndex) -> std::optional<HexSearch::CostHalves> {
      const Direction direction = static_cast<Direction>(arcIndex);
      const std::optional<HexIndex> to = definition_->board->neighbour(HexIndex{from}, direction);
      if (!to || enemyHeldP(*to)) {
        return std::nullopt;
      }
      MovementPoints worst{0};
      for (UnitId unit : units) {
        const EntryVerdict verdict = policies_.movement->enter(ctx, unit, HexIndex{from}, direction, mode);
        if (std::holds_alternative<HexModel::Prohibited>(verdict.cost)) {
          return std::nullopt;
        }
        worst = std::max(worst, std::get<MovementPoints>(verdict.cost));
      }
      return HexSearch::CostHalves{worst.halves};
    };

    // A hex the mover must stop in is settled but never expanded: the terrain says stop, or an
    // enemy zone of control covers it.
    const std::function<bool(HexSearch::NodeIndex)> terminalP = [&](HexSearch::NodeIndex node) {
      const HexIndex hex{node};
      if (hex == *origin) {
        return false;
      }
      if (nullptr != policies_.zoc && policies_.zoc->blockedForP(ctx, hex, side, Purpose::Movement)) {
        return true;
      }
      const HexRules::Terrain& terrain =
          definition_->rules->hexTerrain()[definition_->board->terrain(hex).value];
      if (!terrain.stopP) {
        return false;
      }
      for (UnitId unit : units) {
        const UnitTypeId type = definition_->roster->unit(unit).type;
        if (terrain.stopExcept.end() ==
            std::find(terrain.stopExcept.begin(), terrain.stopExcept.end(), type)) {
          return true;
        }
      }
      return false;
    };

    HexSearch::SearchScratch scratch;  // local, so that reachable() is const and thread-safe
    const HexSearch::Source source{origin->value, HexSearch::CostHalves::zero()};
    const HexSearch::Field<HexSearch::CostHalves> field = HexSearch::Algorithms::dijkstraBounded(
        graph, scratch, std::span<const HexSearch::Source>(&source, 1), arcCost,
        HexSearch::CostHalves{ceiling.halves}, terminalP);

    Reachability out;
    for (HexSearch::NodeIndex node : field.reached()) {
      if (node == origin->value) {
        continue;
      }
      out.hexes.push_back(HexIndex{node});
      std::vector<HexIndex> path;
      for (HexSearch::NodeIndex step : field.pathTo(node)) {
        path.push_back(HexIndex{step});
      }
      out.paths.push_back(std::move(path));
      out.costs.push_back(MovementPoints{field.distance(node).halves});
    }
    return out;
  }

  std::vector<HexIndex>
  Session::attackTargets(std::span<const UnitId> attackers) const
  {
    if (attackers.empty()) {
      throw std::invalid_argument("Session::attackTargets: no attacker given");
    }
    const SideId side = definition_->roster->unit(attackers.front()).side;
    const SideMask enemies = enemiesOf(*definition_->rules, side);

    std::vector<HexIndex> out;
    const std::optional<HexIndex> first = hexOf(position_, attackers.front());
    if (!first) {
      return out;
    }
    for (int d = 0; d < HexCoord::kDirections; ++d) {
      const std::optional<HexIndex> target =
          definition_->board->neighbour(*first, static_cast<Direction>(d));
      if (!target) {
        continue;
      }
      bool enemyHeldP = false;
      for (UnitId occupant : position_.unitsAt(*target)) {
        if (enemies.test(definition_->roster->unit(occupant).side.value)) {
          enemyHeldP = true;
        }
      }
      if (!enemyHeldP) {
        continue;
      }
      bool allAdjacentP = true;
      for (UnitId attacker : attackers) {
        const std::optional<HexIndex> from = hexOf(position_, attacker);
        if (!from || 1 != definition_->board->distance(*from, *target)) {
          allAdjacentP = false;
        }
      }
      if (allAdjacentP) {
        out.push_back(*target);
      }
    }
    return out;
  }

  Applied
  Session::apply(const Command& command)
  {
    std::vector<Event> events;
    RecordingSink sink(events);
    Position next = adjudicate(command, sink);

    const std::size_t firstEvent = log_.size();
    position_ = std::move(next);
    for (const Event& event : events) {
      log_.append(event);
      for (EventSink* observer : sinks_) {
        observer->onEvent(event);
      }
    }
    return Applied{firstEvent, events.size()};
  }

  Position
  Session::adjudicate(const Command& command, EventSink& sink)
  {
    const Ctx ctx = context();
    const GameNames names(*definition_->board, *definition_->roster, *definition_->rules);
    const bool pendingP = !std::holds_alternative<HexModel::NoDecision>(position_.pending());
    const std::optional<SideId> acting = position_.clock().actingSide;

    if (pendingP && !std::holds_alternative<DecisionAnswer>(command)) {
      throw std::invalid_argument("Session::apply: a decision pending on this position must be "
                                  "answered before any other command");
    }
    if (nullptr == policies_.phases) {
      throw std::invalid_argument("Session::apply: the policy set has no PhaseGate");
    }
    const PhaseCaps caps = policies_.phases->capsFor(position_.clock().phase);

    if (const MoveUnit* move = std::get_if<MoveUnit>(&command)) {
      if (!caps.test(static_cast<std::size_t>(Cap::Move))) {
        throw std::invalid_argument("Session::apply: this phase grants no movement");
      }
      if (!acting) {
        throw std::invalid_argument("Session::apply: no side is acting, so nothing may move");
      }
      for (UnitId unit : move->units) {
        if (definition_->roster->unit(unit).side != *acting) {
          throw std::invalid_argument("Session::apply: counter '" +
                                       definition_->roster->unit(unit).counter.text +
                                       "' does not belong to the acting side");
        }
      }
      if (2 > move->path.size()) {
        throw std::invalid_argument("Session::apply: a move needs a path of at least two hexes");
      }
      const std::optional<HexIndex> origin = hexOf(position_, move->units.front());
      if (!origin || *origin != move->path.front()) {
        throw std::invalid_argument("Session::apply: the path does not start on the unit's own hex");
      }
      const Reachability field = reachable(move->units, move->mode);
      const auto found = std::find(field.hexes.begin(), field.hexes.end(), move->path.back());
      if (field.hexes.end() == found) {
        throw std::invalid_argument("Session::apply: hex '" + names.hex(move->path.back()) +
                                     "' is not reachable by this stack in this phase");
      }
      return Adjudicators::applyMove(ctx, policies_, *move, sink);
    }

    if (const DeclareAttack* attack = std::get_if<DeclareAttack>(&command)) {
      if (!caps.test(static_cast<std::size_t>(Cap::Combat))) {
        throw std::invalid_argument("Session::apply: this phase grants no combat");
      }
      if (!acting) {
        throw std::invalid_argument("Session::apply: no side is acting, so nothing may attack");
      }
      for (UnitId unit : attack->attackers) {
        if (definition_->roster->unit(unit).side != *acting) {
          throw std::invalid_argument("Session::apply: counter '" +
                                       definition_->roster->unit(unit).counter.text +
                                       "' does not belong to the acting side");
        }
      }
      const std::vector<HexIndex> targets = attackTargets(attack->attackers);
      if (targets.end() == std::find(targets.begin(), targets.end(), attack->target)) {
        throw std::invalid_argument("Session::apply: hex '" + names.hex(attack->target) +
                                     "' is not adjacent to every attacker, or holds no enemy");
      }
      return Adjudicators::applyAttack(ctx, policies_, *attack, streams_, sink);
    }

    if (const DecisionAnswer* answer = std::get_if<DecisionAnswer>(&command)) {
      if (!pendingP) {
        throw std::invalid_argument("Session::apply: no decision is pending on this position");
      }
      return Adjudicators::applyDecision(ctx, policies_, *answer, names, sink);
    }

    if (std::holds_alternative<EndPhase>(command)) {
      Position next = position_;
      if (caps.test(static_cast<std::size_t>(Cap::Supply)) && acting) {
        const Ctx checking{*definition_->board, *definition_->rules, *definition_->roster, next};
        next = Adjudicators::applySupplyCheck(checking, policies_, scratch_, *acting, sink);
      }
      {
        const Ctx repairing{*definition_->board, *definition_->rules, *definition_->roster, next};
        next = Adjudicators::applyStackingRepair(repairing, policies_, sink);
      }
      const Ctx turning{*definition_->board, *definition_->rules, *definition_->roster, next};
      const PhaseCursor cursor(*definition_->rules);
      return Adjudicators::advancePhase(turning, policies_, cursor, sink);
    }

    if (std::holds_alternative<ResolveNextAttack>(command)) {
      throw std::invalid_argument("Session::apply: no declared attack is waiting to be resolved; the "
                                  "engine resolves an attack as it is declared");
    }
    if (std::holds_alternative<Place>(command)) {
      throw std::invalid_argument("Session::apply: placing a counter needs the game's own arrival "
                                  "schedule, which no game supplies yet");
    }
    throw std::invalid_argument("Session::apply: the engine has no default for this game command");
  }

  Session
  Session::fork() const
  {
    Session copy(*this);
    copy.sinks_.clear();
    return copy;
  }

  void
  Session::attach(EventSink& sink)
  {
    if (sinks_.end() == std::find(sinks_.begin(), sinks_.end(), &sink)) {
      sinks_.push_back(&sink);
    }
    return;
  }

  void
  Session::detach(EventSink& sink)
  {
    const auto found = std::find(sinks_.begin(), sinks_.end(), &sink);
    if (sinks_.end() != found) {
      sinks_.erase(found);
    }
    return;
  }

}  // namespace HexEngine
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------

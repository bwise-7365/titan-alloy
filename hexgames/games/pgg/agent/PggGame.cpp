// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
#include "PggGame.h"

#include "PggAgent.h"
#include "PggBinding.h"
#include "PggUnits.h"

#include "hexengine/Event.h"
#include "hexmodel/Seed.h"
#include "hexrecord/Record.h"

#include <map>
#include <random>
#include <sstream>
#include <stdexcept>
#include <utility>
#include <vector>

namespace Pgg {

  namespace {

    constexpr std::uint64_t kDrawnSetUpStream = 64;  // above the engine's StreamTags

    // An unbiased draw below `count` from raw outputs: a <random> distribution's draws differ between
    // standard libraries.
    std::size_t
    drawBelow(std::mt19937_64& gen, std::size_t count)
    {
      const std::uint64_t limit = gen.max() - (gen.max() % count + 1) % count;
      std::uint64_t draw = gen();
      while (limit < draw) {
        draw = gen();
      }
      return static_cast<std::size_t>(draw % count);
    }

  }  // namespace

  PggShared
  loadShared(const std::filesystem::path& root)
  {
    PggShared shared;
    shared.definition = loadPackage(root / "packages" / "xml" / "pgg.package.xml");
    shared.set = std::make_unique<PggPolicySet>(*shared.definition);
    shared.start = HexRecord::readRecord(root / "games" / "pgg" / "scenario" / "pgg-1941.xml", *shared.definition,
                                         shared.set->policies())
                       .position;
    return shared;
  }

  Position
  drawnSetUp(const PggPolicySet& set, Position start, std::uint64_t seed)
  {
    const PggFacts& facts = set.facts();
    const HexRules::GameDefinition& definition = facts.definition();
    std::mt19937_64 gen(HexModel::mixSeed(seed, kDrawnSetUpStream));
    Position next = start;
    std::map<UnitId, UnitId> drawnFor;
    for (Arm arm : {Arm::SovietRifle, Arm::SovietArmour}) {
      std::vector<UnitId> slots;
      for (UnitId unit : definition.roster->ofSide(facts.soviet())) {
        const bool inPlayP = Units::hexOf(start, unit) || Units::inSpaceP(start, unit, facts.sovietPool());
        if (arm == facts.arm(unit) && inPlayP) {
          slots.push_back(unit);
        }
      }
      std::vector<UnitId> drawn = slots;
      for (std::size_t i = drawn.size(); 1 < i; --i) {
        std::swap(drawn[i - 1], drawn[drawBelow(gen, i)]);
      }
      for (std::size_t i = 0; i < slots.size(); ++i) {
        next.place(drawn[i], *start.unit(slots[i]).where);
        drawnFor[slots[i]] = drawn[i];
      }
    }
    PggState& state = stateOf(next);
    std::map<UnitId, Army> armies;
    for (const auto& [unit, army] : state.armies) {
      const auto drawn = drawnFor.find(unit);
      armies[drawnFor.end() == drawn ? unit : drawn->second] = army;
    }
    state.armies = std::move(armies);
    return next;
  }

  PggGameResult
  playGame(const PggShared& shared, const PggGameTask& task)
  {
    const PggPolicySet& set = *shared.set;
    PggGameResult result;
    HexEngine::Session session(shared.definition, set.policies(),
                               SetUp::Drawn == task.setUp ? drawnSetUp(set, shared.start, task.seed) : shared.start, task.seed);
    PggAgent german(set, set.facts().german(), task.german);
    PggAgent soviet(set, set.facts().soviet(), task.soviet);
    std::vector<HexAi::Agent*> agents(2, nullptr);
    agents.at(set.facts().german().value) = &german;
    agents.at(set.facts().soviet().value) = &soviet;
    result.play = HexAi::playOut(session, agents, task.attempts);
    result.lastTurn = session.position().clock().turn;
    for (const HexEngine::Event& event : session.events().events()) {
      if (task.keepEventsP) {
        result.events.push_back(HexEngine::TextEventEncoder::line(event, set.names()));
      }
      if (const HexEngine::GameEvent* game = std::get_if<HexEngine::GameEvent>(&event)) {
        if ("victory-points" == game->kind) {
          std::istringstream words(game->text);  // "german N, soviet M"
          std::string germanWord, sovietWord;
          char comma = 0;
          int germanPoints = 0;
          int sovietPoints = 0;
          if (!(words >> germanWord >> germanPoints >> comma >> sovietWord >> sovietPoints)) {
            throw std::invalid_argument("playGame: cannot read the victory points '" + game->text + "'");
          }
          result.germanPoints = germanPoints;
          result.sovietPoints = sovietPoints;
        }
      } else if (const HexEngine::VictoryDeclared* victory = std::get_if<HexEngine::VictoryDeclared>(&event)) {
        result.level = victory->condition;
      }
    }
    return result;
  }

}  // namespace Pgg
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------

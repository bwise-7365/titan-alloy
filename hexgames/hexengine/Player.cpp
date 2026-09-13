// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
#include "hexengine/Player.h"

#include "hexengine/Policies.h"

#include <stdexcept>
#include <string>

namespace HexEngine {

  ScriptedPlayer::ScriptedPlayer(std::vector<Command> script) : script_(std::move(script))
  {
  }

  Command
  ScriptedPlayer::choose(const Session&, SideId)
  {
    if (script_.size() <= next_) {
      throw std::invalid_argument("ScriptedPlayer: the script has run out after " +
                                   std::to_string(script_.size()) + " commands");
    }
    return script_[next_++];
  }

  std::size_t
  ScriptedPlayer::remaining() const
  {
    return script_.size() - next_;
  }

  RandomPlayer::RandomPlayer(std::mt19937_64& stream) : stream_(stream)
  {
  }

  Command
  RandomPlayer::choose(const Session& session, SideId side)
  {
    std::vector<Command> choices = session.legalCommands();
    const Prompt prompt = session.prompt();
    const PhaseCaps caps = session.policies().phases->capsFor(prompt.phase);
    if (!caps.test(static_cast<std::size_t>(Cap::Move)) || prompt.decisionPendingP) {
      if (choices.empty()) {
        throw std::invalid_argument("RandomPlayer: the position offers no legal command");
      }
      return choices[stream_() % choices.size()];
    }

    // One move, for one of the side's own units that is on the map and prints an allowance: the
    // roster is in counter order and the reachable field is in settlement order, so the same stream
    // makes the same choice every time.
    const HexModel::Roster& roster = *session.definition().roster;
    std::vector<UnitId> movable;
    for (UnitId unit : roster.ofSide(side)) {
      const HexModel::UnitState& state = session.position().unit(unit);
      if (!state.where || !std::holds_alternative<HexIndex>(*state.where)) {
        continue;
      }
      if (!roster.unit(unit).front.allowance.has_value()) {
        continue;
      }
      movable.push_back(unit);
    }
    if (!movable.empty()) {
      const UnitId unit = movable[stream_() % movable.size()];
      const std::span<const UnitId> one(&unit, 1);
      try {
        const Reachability field = session.reachable(one, ModeId{0});
        if (!field.hexes.empty()) {
          const std::size_t pick = stream_() % field.hexes.size();
          choices.push_back(MoveUnit{{unit}, ModeId{0}, field.paths[pick]});
        }
      } catch (const std::invalid_argument&) {
        // A unit this policy set cannot move at all simply offers no move.
      }
    }

    if (choices.empty()) {
      throw std::invalid_argument("RandomPlayer: the position offers no legal command");
    }
    return choices[stream_() % choices.size()];
  }

  void
  play(Session& session, std::vector<Player*> playersBySide, int maxCommands)
  {
    for (int applied = 0; applied < maxCommands; ++applied) {
      const Prompt prompt = session.prompt();
      if (prompt.overP || !prompt.side) {
        return;
      }
      if (playersBySide.size() <= prompt.side->value || nullptr == playersBySide[prompt.side->value]) {
        throw std::invalid_argument("HexEngine::play: no player for side " +
                                     std::to_string(prompt.side->value));
      }
      session.apply(playersBySide[prompt.side->value]->choose(session, *prompt.side));
    }
    return;
  }

  PositionView::PositionView(const Session& session, SideId viewer) : session_(session), viewer_(viewer)
  {
  }

  bool
  PositionView::visibleP(UnitId unit) const
  {
    const HexModel::UnitSpec& spec = session_.definition().roster->unit(unit);
    if (spec.side == viewer_) {
      return true;
    }
    return session_.position().unit(unit).flags.revealedP;
  }

}  // namespace HexEngine
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------

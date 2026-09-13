// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
// Players choose commands; the engine never blocks on one. A human player is the GUI calling
// Session::apply; scripted and random players are here; an AI plugs in the same way.
// ----------------------------------------------
#pragma once
#include "hexengine/Command.h"
#include "hexengine/Session.h"

#include <random>
#include <vector>

namespace HexEngine {

  class Player {
  public:
    virtual ~Player() = default;
    // The next command for `side`, given the session; called only when prompt().side == side.
    virtual Command choose(const Session&, SideId side) = 0;
  };

  // Replays a fixed list of commands (a hexsave script) in order; throws when the list runs out.
  class ScriptedPlayer : public Player {
  public:
    explicit ScriptedPlayer(std::vector<Command>);
    Command choose(const Session&, SideId) override;
    std::size_t remaining() const;
  private:
    std::vector<Command> script_;
    std::size_t next_ = 0;
  };

  // Picks uniformly among legal commands and reachable moves from its own stream.
  class RandomPlayer : public Player {
  public:
    explicit RandomPlayer(std::mt19937_64& stream);
    Command choose(const Session&, SideId) override;
  private:
    std::mt19937_64& stream_;
  };

  // Runs a session with one player per side until the game ends or a player throws.
  void play(Session&, std::vector<Player*> playersBySide, int maxCommands);

  // A side's view of a position with hidden units concealed (DDaT), for players and views.
  class PositionView {
  public:
    PositionView(const Session&, SideId viewer);
    bool visibleP(UnitId) const;
  private:
    const Session& session_;
    SideId viewer_;
  };

}  // namespace HexEngine
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------

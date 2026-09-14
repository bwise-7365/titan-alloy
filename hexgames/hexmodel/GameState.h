// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
// A game's own state (M6b, replacing M6's string-keyed side flags): a game derives its state from
// GameState with real types (enums, counts, hex ids), and writes a GameStateCodec that turns it to
// and from hexsave <sides><side><flag name value/>. Strings exist only in the codec, which parses
// and validates once and names the offending flag on failure. Position holds at most one GameState
// and hands it out through one checked accessor. A game with no module has no state and no flags
// (M6b review): the engine's NoGameStateCodec refuses any flag.
// ----------------------------------------------
#pragma once
#include "hexmodel/Polymorphic.h"

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace HexModel {

  struct SideFlag {
    std::string name;
    std::string value;
  };
  // One list per rules side, in the rules document's side order; each list in document order.
  using SideFlags = std::vector<std::vector<SideFlag>>;

  class GameState {
  public:
    virtual ~GameState() = default;
    virtual std::unique_ptr<GameState> clone() const = 0;
    // The state's canonical text, appended to Position::digest()'s input.
    virtual void appendDigest(std::string&) const = 0;
  };

  class GameStateCodec {
  public:
    virtual ~GameStateCodec() = default;
    // Throws std::invalid_argument naming the flag (and its side) that is unknown, misplaced or
    // unreadable. May return an empty value: the game keeps no state.
    virtual Polymorphic<GameState> decode(const SideFlags&) const = 0;
    // The flags hexsave writes, one list per side in the order they are written.
    virtual SideFlags encode(const GameState&) const = 0;
  };

}  // namespace HexModel
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------

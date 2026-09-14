// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
// [PROPOSED contract, M10] Looking back and animating. ReplayController gives the position after any
// move of a record by replaying it in its own session (deterministic: the same record gives the same
// positions), caching every Nth position so a seek costs at most N moves. AnimationPlan turns the
// events of one applied command into timed tweens, so a view shows what the log says happened.
// ----------------------------------------------
#pragma once
#include "hexengine/Event.h"
#include "hexengine/Policies.h"
#include "hexmodel/Board.h"
#include "hexrecord/Record.h"
#include "hexview/MapFrame.h"

#include <cstddef>
#include <map>
#include <memory>
#include <span>
#include <variant>
#include <vector>

namespace HexView {

  class ReplayController {
  public:
    // Throws std::invalid_argument, naming the move, when the record does not replay (the same
    // divergence hexrecord's replay() reports). cacheEvery must be positive.
    ReplayController(std::shared_ptr<const HexRules::GameDefinition>, const HexEngine::Policies&,
                     const HexRecord::Record&, std::size_t cacheEvery);

    std::size_t moveCount() const;
    // The position after `move` moves (0: the record's start). Throws for move > moveCount().
    const HexModel::Position& at(std::size_t move);
    // The events move `move` produced (1-based). Throws for 0 or move > moveCount().
    std::span<const HexEngine::Event> eventsOf(std::size_t move) const;

  private:
    std::shared_ptr<const HexRules::GameDefinition> definition_;
    const HexEngine::Policies& policies_;
    HexRecord::Record record_;
    std::size_t cacheEvery_;
    std::map<std::size_t, HexModel::Position> cache_;
    std::vector<std::pair<std::size_t, std::size_t>> eventRanges_;  // per move: first event, count
    HexEngine::EventLog log_;
  };

  // ---- animation -------------------------------------------------------------------------------
  struct MoveTween { HexModel::UnitId unit; std::vector<Pixel> path; double seconds; };
  struct FlipTween { HexModel::UnitId unit; double seconds; };  // UnitRevealed, a reduction
  struct FadeTween { HexModel::UnitId unit; double seconds; };  // UnitEliminated
  struct DieTween { int value; double seconds; };               // DieRolled
  using Tween = std::variant<MoveTween, FlipTween, FadeTween, DieTween>;

  struct AnimationPlan {
    std::vector<std::vector<Tween>> beats;  // tweens in one beat run together; beats run in order
  };

  // Events with nothing to animate (phase changes, control, supply checks) add no beat.
  AnimationPlan planFor(std::span<const HexEngine::Event>, const HexModel::Board&, const MapFrame&);

}  // namespace HexView
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------

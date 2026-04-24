// Copyright Ben Paul Wise. All Rights Reserved.
#pragma once
#include <memory>
#include <vector>

namespace AbsGame {

// Integer move identifier. -1 (kPass) means "pass"; >= 0 is a board position.
using MoveId = int;
static constexpr MoveId kPass = -1;

// Abstract interface for a two-person, zero-sum, perfect-information game.
// Player 0 moves first (convention: Black), player 1 moves second (White).
class Game {
public:
    virtual ~Game() = default;

    // Index of the player to move: 0 (first) or 1 (second).
    virtual int currentPlayer() const = 0;

    // All legal moves in the current state, including kPass where applicable.
    virtual std::vector<MoveId> getLegalMoves() const = 0;

    // True iff the move is legal in the current state.
    virtual bool isLegalMove(MoveId move) const = 0;

    // Apply the move and update state; returns false if the move is illegal.
    virtual bool applyMove(MoveId move) = 0;

    // True when the game has ended and no further moves are possible.
    virtual bool isTerminal() const = 0;

    // Heuristic score from the perspective of the player to move.
    // Positive values favour the mover; negative values favour the opponent.
    virtual double staticEval() const = 0;

    // Eval used by NegaMax. Default delegates to staticEval(); subclasses may
    // override with a richer heuristic without affecting MCTS rollouts.
    virtual double negamaxEval() const { return staticEval(); }

    // Deep copy: board state is duplicated; the underlying graph is shared.
    virtual std::unique_ptr<Game> clone() const = 0;

};

} // namespace AbsGame
// Copyright Ben Paul Wise. All Rights Reserved.

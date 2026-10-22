// Copyright Ben Paul Wise. All Rights Reserved.
#pragma once
#include <cstdint>
#include <memory>
#include <random>
#include <vector>

namespace AbsGame {

// ---- PRNG / seed utilities (see Prng.cpp) --------------------------------
// A small, platform-independent quadratic bit-mixer and seed maker, used to
// turn a chosen (or clock-derived) value into a usable RNG seed.

// A fresh seed derived from the microsecond wall clock (non-reproducible).
uint64_t msRandom();

// The seed to use: `s` if non-zero (reproducible), else a clock-derived one.
uint64_t makeSeed(uint64_t s);

// Integer move identifier. -1 (kPass) means "pass"; >= 0 is a board position.
using MoveId = int;
static constexpr MoveId kPass = -1;

// Ply ceiling for one MCTS playout in a game that does not say otherwise (see
// Game::maxPlayoutDepth). Playouts must be bounded because a random walk is not
// guaranteed to terminate, but a ceiling below the length of a real game is not a safety
// net -- it is a silent change of what a playout measures, since a capped playout scores
// staticEval() on an unfinished position instead of a true outcome.
inline constexpr int kDefaultMaxPlayoutDepth = 200;

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

    // Rollout (playout) move policy used by MCTS simulations. The default is a
    // uniform-random pick over `legal` (the classic "light" playout). A game may
    // override it with an informed ("heavy") policy -- e.g. an epsilon-greedy bias
    // toward aggressive moves -- so rollouts stay informative in domains where
    // random play rarely reaches a terminal. `legal` must be non-empty. See
    // mcts.cpp (rollout) and Latrunculi::Game::chooseRolloutMove for the rationale
    // and the supporting literature.
    virtual MoveId chooseRolloutMove(const std::vector<MoveId>& legal,
                                     std::mt19937_64& rng) const;

    // The most plies one MCTS playout from this position may run before the searcher
    // gives up and scores it with staticEval(). A game whose length grows with its board
    // must override this: a fixed ceiling that a full game exceeds makes EVERY playout
    // end early, so the search never sees a real outcome and any count of terminal
    // positions reached measures the ceiling rather than the search. See
    // IrrGo::Game::maxPlayoutDepth. Must return a positive number.
    virtual int maxPlayoutDepth() const { return kDefaultMaxPlayoutDepth; }

    // Move-ordering hint for the alpha-beta searcher (negamax.cpp): a higher score is
    // searched earlier. Alpha-beta prunes far more when the strongest move is tried
    // first, so a game that can cheaply recognise a capture should rank it high here.
    // The default scores every move 0 and the searcher's sort is stable, so a game that
    // does not override this keeps getLegalMoves() order exactly. See
    // Latrunculi::Game::moveOrderScore.
    virtual int moveOrderScore(MoveId) const { return 0; }
};

} // namespace AbsGame
// Copyright Ben Paul Wise. All Rights Reserved.

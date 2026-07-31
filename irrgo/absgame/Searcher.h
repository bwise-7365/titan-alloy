// Copyright Ben Paul Wise. All Rights Reserved.
#pragma once
#include "AbsGame.h"
#include <chrono>

namespace AbsGame {

class Searcher {
public:
    // Returns the best move for game, searched by iterative deepening to `depth`.
    // timeLimitMs: wall-clock budget. Depths 1, 2, ... are searched in turn and only
    // an iteration that COMPLETES is allowed to change the answer, so a deadline that
    // fires mid-iteration falls back to the last fully-searched depth rather than to a
    // partially-scored move list. Returns kPass only when there are no legal moves.
    static MoveId bestMove(const Game& game, int depth, int timeLimitMs = 5000);

    // Returns the best move via MCTS-UCT (time-budget variant).
    // Runs growTree iterations for the given number of seconds, then returns
    // the robust child (most-visited).
    static MoveId mcts(const Game& game, int seconds);

private:
    // `aborted` is set true when the deadline fires. The returned score is then
    // meaningless and the caller must discard it -- bestMove throws away the whole
    // iteration rather than substituting a plausible-looking value for a search that
    // did not happen.
    static double negaMax(const Game& game, int depth, double alpha, double beta,
                          std::chrono::steady_clock::time_point deadline, bool& aborted);
};

} // namespace AbsGame
// Copyright Ben Paul Wise. All Rights Reserved.

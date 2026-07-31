// Copyright Ben Paul Wise. All Rights Reserved.
#pragma once
#include "AbsGame.h"
#include <chrono>

namespace AbsGame {

// What one MCTS search did, for a caller judging how much the estimate is worth.
//
// The Monte Carlo estimate is an average over playout outcomes, so it only means what it
// claims to when playouts actually reach outcomes. A playout that hits the depth ceiling
// (Game::maxPlayoutDepth) is scored by staticEval() on an unfinished position instead --
// still useful, but a heuristic guess rather than a sampled result. terminalRollouts
// against rollouts is how much of the estimate is which.
//
// Filled per call and owned by the caller, never stored in the Searcher: two searches may
// run at once on different threads, and a shared counter would race.
struct SearchStats {
    long long rollouts = 0;          // playouts run
    long long terminalRollouts = 0;  // of those, how many reached a true terminal
};

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
    // the robust child (most-visited). When `stats` is non-null it receives this
    // search's playout counts; passing nullptr costs nothing.
    static MoveId mcts(const Game& game, int seconds, SearchStats* stats = nullptr);

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

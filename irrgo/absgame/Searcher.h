// Copyright Ben Paul Wise. All Rights Reserved.
#pragma once
#include "AbsGame.h"
#include <chrono>

namespace AbsGame {

class Searcher {
public:
    // Returns the best move for game at the given search depth.
    // timeLimitMs: wall-clock budget; returns the best move found so far when
    // the deadline fires (may return kPass if no move was fully evaluated).
    static MoveId bestMove(const Game& game, int depth, int timeLimitMs = 5000);

    // Returns the best move via MCTS-UCT (node-budget variant).
    // nodeMin: minimum iterations before convergence check.
    // nodeMax: hard budget limit.
    static MoveId mcts(const Game& game, int nodeMin, int nodeMax);

    // Returns the best move via MCTS-UCT (time-budget variant).
    // Runs growTree iterations for the given number of seconds, then returns
    // the robust child (most-visited).
    static MoveId mcts(const Game& game, int seconds);


    static int terminalCount;

private:
    static double negaMax(const Game& game, int depth, double alpha, double beta,
                          std::chrono::steady_clock::time_point deadline);
};

} // namespace AbsGame
// Copyright Ben Paul Wise. All Rights Reserved.

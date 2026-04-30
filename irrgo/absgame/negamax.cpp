// Copyright Ben Paul Wise. All Rights Reserved.
#include "Searcher.h"
#include <chrono>
#include <limits>

namespace AbsGame {

using Clock     = std::chrono::steady_clock;
using TimePoint = Clock::time_point;

double Searcher::negaMax(const Game& game, int depth, double alpha, double beta,
                         TimePoint deadline) {
    if (depth == 0 || game.isTerminal() || Clock::now() >= deadline)
        return game.negamaxEval();

    double value = -std::numeric_limits<double>::infinity();
    for (MoveId move : game.getLegalMoves()) {
        auto child = game.clone();
        child->applyMove(move);
        double score = -negaMax(*child, depth - 1, -beta, -alpha, deadline);
        if (score > value) value = score;
        if (value > alpha) alpha = value;
        if (alpha >= beta) break;
    }
    return value;
}

MoveId Searcher::bestMove(const Game& game, int depth, int timeLimitMs) {
    terminalCount = 0;
    auto deadline = Clock::now() + std::chrono::milliseconds(timeLimitMs);

    MoveId best      = kPass;
    double bestScore = -std::numeric_limits<double>::infinity();
    double alpha     = -std::numeric_limits<double>::infinity();
    const double beta = std::numeric_limits<double>::infinity();

    for (MoveId move : game.getLegalMoves()) {
        if (Clock::now() >= deadline) break;
        auto child = game.clone();
        child->applyMove(move);
        double score = -negaMax(*child, depth - 1, -beta, -alpha, deadline);
        if (score > bestScore) {
            bestScore = score;
            best      = move;
        }
        if (score > alpha) alpha = score;
    }
    return best;
}

} // namespace AbsGame
// Copyright Ben Paul Wise. All Rights Reserved.

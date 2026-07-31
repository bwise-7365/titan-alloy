// Copyright Ben Paul Wise. All Rights Reserved.
#include "Searcher.h"
#include <algorithm>
#include <chrono>
#include <limits>
#include <utility>
#include <vector>

namespace AbsGame {

using Clock     = std::chrono::steady_clock;
using TimePoint = Clock::time_point;

namespace {

// Legal moves, strongest first, by the game's moveOrderScore() hook. Each move is
// scored exactly once (scoring can be expensive -- Latrunculi resolves the captures a
// move triggers) and the sort is stable, so a game that does not override the hook
// scores every move 0 and keeps getLegalMoves() order untouched.
std::vector<MoveId> orderedMoves(const Game& game) {
    std::vector<MoveId> legal = game.getLegalMoves();

    std::vector<std::pair<int, MoveId>> scored;
    scored.reserve(legal.size());
    for (MoveId move : legal) {
        scored.emplace_back(game.moveOrderScore(move), move);
    }
    std::stable_sort(scored.begin(), scored.end(),
                     [](const std::pair<int, MoveId>& a, const std::pair<int, MoveId>& b) {
                         return a.first > b.first;
                     });

    // Write the ranking back over `legal` rather than allocating a third vector:
    // this runs once per negamax node.
    for (std::size_t i = 0; i < scored.size(); ++i) {
        legal[i] = scored[i].second;
    }
    return legal;
}

} // anonymous namespace

double Searcher::negaMax(const Game& game, int depth, double alpha, double beta,
                         TimePoint deadline, bool& aborted) {
    // Once the deadline has fired every score below is meaningless; unwind without
    // pretending otherwise. bestMove discards the whole iteration (see Searcher.h).
    if (aborted) {
        return 0.0;
    }
    if (Clock::now() >= deadline) {
        aborted = true;
        return 0.0;
    }

    if (depth == 0 || game.isTerminal()) {
        return game.negamaxEval();
    }

    double value = -std::numeric_limits<double>::infinity();
    for (MoveId move : orderedMoves(game)) {
        auto child = game.clone();
        child->applyMove(move);
        const double score = -negaMax(*child, depth - 1, -beta, -alpha, deadline, aborted);
        if (aborted) {
            return 0.0;
        }
        if (score > value) {
            value = score;
        }
        if (value > alpha) {
            alpha = value;
        }
        if (alpha >= beta) {
            break;
        }
    }
    return value;
}

MoveId Searcher::bestMove(const Game& game, int depth, int timeLimitMs) {
    const TimePoint deadline = Clock::now() + std::chrono::milliseconds(timeLimitMs);

    std::vector<MoveId> rootMoves = orderedMoves(game);
    if (rootMoves.empty()) {
        return kPass;
    }

    // The answer before any iteration completes: the best move by ordering alone. That
    // is a real (if shallow) preference, not a filler value -- with a capture-aware
    // moveOrderScore it is the most aggressive legal move.
    MoveId best = rootMoves.front();

    // Iterative deepening. The previous depth's answer is searched first at the next
    // depth, which raises alpha immediately and is where most of the pruning comes from.
    for (int d = 1; d <= depth; ++d) {
        bool         aborted        = false;
        MoveId       iterationBest  = rootMoves.front();
        double       iterationScore = -std::numeric_limits<double>::infinity();
        double       alpha          = -std::numeric_limits<double>::infinity();
        const double beta           = std::numeric_limits<double>::infinity();

        for (MoveId move : rootMoves) {
            auto child = game.clone();
            child->applyMove(move);
            const double score = -negaMax(*child, d - 1, -beta, -alpha, deadline, aborted);
            if (aborted) {
                break;
            }
            if (score > iterationScore) {
                iterationScore = score;
                iterationBest  = move;
            }
            if (score > alpha) {
                alpha = score;
            }
        }

        // A truncated iteration scored only a prefix of the root moves, so its "best" is
        // the best of an arbitrary subset and is not comparable with the moves it never
        // reached. Throw it away and keep the last depth searched in full.
        if (aborted) {
            break;
        }

        best = iterationBest;
        const auto it = std::find(rootMoves.begin(), rootMoves.end(), best);
        if (it != rootMoves.end()) {
            std::rotate(rootMoves.begin(), it, it + 1);
        }
    }

    return best;
}

} // namespace AbsGame
// Copyright Ben Paul Wise. All Rights Reserved.

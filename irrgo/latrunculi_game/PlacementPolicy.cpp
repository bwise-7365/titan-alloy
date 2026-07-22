// Copyright Ben Paul Wise. All Rights Reserved.

#include "PlacementPolicy.h"

#include <stdexcept>

namespace Latrunculi {

PlacementPolicy::PlacementPolicy(std::uint64_t seed) : rng_(seed) {}

void PlacementPolicy::reset(std::uint64_t seed) {
    rng_.seed(seed);
    optimalRemaining_[0] = 0;
    optimalRemaining_[1] = 0;
}

bool PlacementPolicy::nextIsRandom(int player) {
    if (player != 0 && player != 1) {
        throw std::invalid_argument("Latrunculi PlacementPolicy: player must be 0 or 1");
    }
    if (optimalRemaining_[player] > 0) {
        --optimalRemaining_[player];
        return false;
    }
    // Starting a fresh cycle: this placement is the random one, and the run of searched
    // placements that follows it is kRunMin or kRunMin+1, drawn 50/50.
    optimalRemaining_[player] = kRunMin + static_cast<int>(rng_() % 2);
    return true;
}

namespace {

// True if any orthogonal neighbour of (row, column) already holds a disc of either
// colour. Orthogonal because that is the game's own notion of adjacency -- custodial
// capture works along ranks and files, so a diagonal neighbour is not "touching" in any
// sense the rules care about.
bool touchesAnyDisc(const Game& game, int row, int column) {
    const int rows = game.rows();
    const int columns = game.columns();
    const int dRow[4] = {-1, 1, 0, 0};
    const int dColumn[4] = {0, 0, -1, 1};
    for (int k = 0; k < 4; ++k) {
        const int nRow = row + dRow[k];
        const int nColumn = column + dColumn[k];
        if (nRow < 0 || nRow >= rows || nColumn < 0 || nColumn >= columns) {
            continue;
        }
        if (game.ownerAt(nRow * columns + nColumn) >= 0) {
            return true;
        }
    }
    return false;
}

}  // anonymous namespace

AbsGame::MoveId PlacementPolicy::pickRandomPlacement(
        const Game& game, const std::vector<AbsGame::MoveId>& moves) {
    if (moves.empty()) {
        throw std::invalid_argument("Latrunculi PlacementPolicy: no moves to pick from");
    }

    const int rows = game.rows();
    const int columns = game.columns();

    // A placement MoveId is just the target square (Game::placementMove), so it decodes
    // straight to row/column.
    std::vector<AbsGame::MoveId> interior;  // off the border
    std::vector<AbsGame::MoveId> isolated;  // off the border and touching nothing
    for (AbsGame::MoveId mv : moves) {
        const int row = mv / columns;
        const int column = mv % columns;
        if (row == 0 || row == rows - 1 || column == 0 || column == columns - 1) {
            continue;
        }
        interior.push_back(mv);
        if (!touchesAnyDisc(game, row, column)) {
            isolated.push_back(mv);
        }
    }

    // First non-empty tier wins; see the header for why 2 and 3 are ordinary outcomes.
    const std::vector<AbsGame::MoveId>& pool =
        !isolated.empty() ? isolated : (!interior.empty() ? interior : moves);
    return pool[rng_() % pool.size()];
}

}  // namespace Latrunculi
// Copyright Ben Paul Wise. All Rights Reserved.

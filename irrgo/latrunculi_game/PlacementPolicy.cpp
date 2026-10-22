// Copyright Ben Paul Wise. All Rights Reserved.

#include "PlacementPolicy.h"

#include <stdexcept>

namespace Latrunculi {

PlacementPolicy::PlacementPolicy(std::uint64_t seed) : rng_(seed) {}

void PlacementPolicy::reset(std::uint64_t seed) {
    rng_.seed(seed);
    placedFirst_[0] = false;
    placedFirst_[1] = false;
}

bool PlacementPolicy::nextIsRandom(int player) {
    if (player != 0 && player != 1) {
        throw std::invalid_argument("Latrunculi PlacementPolicy: player must be 0 or 1");
    }
    if (placedFirst_[player]) {
        return false;
    }
    placedFirst_[player] = true;
    return true;
}

namespace {

// True if (row, column) shares a row or a column with any disc already on the board,
// or is diagonally adjacent to one. Sharing a row or column covers orthogonal adjacency
// as a special case, so a square that passes is not touching any disc in any direction
// and cannot be flanked along a rank or file by a disc already present.
bool alignedOrDiagonalToAnyDisc(const Game& game, int row, int column) {
    const int rows = game.rows();
    const int columns = game.columns();
    for (int r = 0; r < rows; ++r) {
        for (int c = 0; c < columns; ++c) {
            if (game.ownerAt(r * columns + c) < 0) {
                continue;
            }
            if (r == row || c == column) {
                return true;
            }
            const int dRow = r > row ? r - row : row - r;
            const int dColumn = c > column ? c - column : column - c;
            if (dRow == 1 && dColumn == 1) {
                return true;
            }
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
    std::vector<AbsGame::MoveId> apart;     // off the border and clear of every disc
    for (AbsGame::MoveId mv : moves) {
        const int row = mv / columns;
        const int column = mv % columns;
        if (row == 0 || row == rows - 1 || column == 0 || column == columns - 1) {
            continue;
        }
        interior.push_back(mv);
        if (!alignedOrDiagonalToAnyDisc(game, row, column)) {
            apart.push_back(mv);
        }
    }

    // First non-empty tier wins; see the header for why 2 and 3 are ordinary outcomes.
    const std::vector<AbsGame::MoveId>& pool =
        !apart.empty() ? apart : (!interior.empty() ? interior : moves);
    return pool[rng_() % pool.size()];
}

}  // namespace Latrunculi
// Copyright Ben Paul Wise. All Rights Reserved.

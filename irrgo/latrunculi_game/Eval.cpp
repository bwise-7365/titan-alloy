// Copyright Ben Paul Wise. All Rights Reserved.

#include "Eval.h"

#include <cmath>
#include <cstddef>
#include <stdexcept>

namespace Latrunculi {

namespace {

// Weights in "disc units": what one unit of each term is worth against one Free disc of
// material. A threat is the most valuable positional asset because it is one move from
// becoming material. A pair is a threat in waiting. Mobility and centrality are small
// tie-breakers whose job is to choose between otherwise equal quiet moves -- they must
// never add up to a real capture, which is why they are an order of magnitude smaller.
constexpr double kThreatWeight   = 0.25;
// Lowered 0.10 -> 0.02 on 2026-07-22 after self-play under both movement rules produced
// two large mutually-defending blobs and ran out the quiet-game limit. `pairs` counts
// EVERY adjacency between own discs, so the bonus grows quadratically with clumping
// while the mobility penalty grows only linearly: nine discs in a solid 3x3 block hold
// 12 pairs against 12 open neighbours, where the same nine dispersed hold 0 pairs and 36
// open neighbours. At 0.10 that was +1.20 against +0.24 -- the engine was paid roughly
// three quarters of a captured disc, every ply, to build a fortress, and the blob was
// this function's stated optimum rather than a search artifact. At 0.02 the block scores
// +0.24 against +0.72 and dispersal wins outright.
//
// This rebalances the two weights; it does not fix the term's shape. A saturating count
// (each disc in at most one pair) or one restricted to pairs actually aimed at an enemy
// would make shape unfarmable. See doc/latrunculi-implementation-plan.md, Stage 7.
constexpr double kPairWeight     = 0.02;
constexpr double kMobilityWeight = 0.02;
constexpr double kCentreWeight   = 0.05;

std::size_t index(int row, int column, int columns) {
    return static_cast<std::size_t>(row) * static_cast<std::size_t>(columns) +
           static_cast<std::size_t>(column);
}

bool inBounds(int row, int column, int rows, int columns) {
    return row >= 0 && row < rows && column >= 0 && column < columns;
}

// 1.0 at the centre of the board, 0.0 at an edge, averaged over the two axes. A board
// one square wide has no spread on that axis, so every square is equally central there
// and the axis contributes its full 1.0. That is the correct value for a degenerate
// board, not a fallback hiding a division by zero.
double centrality(int row, int column, int rows, int columns) {
    const double rowTerm = (rows > 1)
        ? 1.0 - std::abs(2.0 * row - (rows - 1)) / (rows - 1)
        : 1.0;
    const double columnTerm = (columns > 1)
        ? 1.0 - std::abs(2.0 * column - (columns - 1)) / (columns - 1)
        : 1.0;
    return 0.5 * (rowTerm + columnTerm);
}

// If the disc at (row, column) is half-pinned along the axis (dRow, dColumn) by
// `flanker` -- one end of the axis holds a flanker disc and the other end is on the board
// and empty -- returns the index of that empty square, the one whose occupation completes
// the custodial capture. Returns -1 if the disc is not half-pinned on this axis.
int halfPinCompletion(const std::vector<Cell>& cells, int rows, int columns,
                      int row, int column, int dRow, int dColumn, Cell flanker) {
    const int aRow = row - dRow;
    const int aColumn = column - dColumn;
    const int bRow = row + dRow;
    const int bColumn = column + dColumn;
    if (!inBounds(aRow, aColumn, rows, columns) || !inBounds(bRow, bColumn, rows, columns)) {
        return -1;
    }
    const std::size_t aIndex = index(aRow, aColumn, columns);
    const std::size_t bIndex = index(bRow, bColumn, columns);
    const Cell a = cells[aIndex];
    const Cell b = cells[bIndex];
    if (a == flanker && b == Cell::Empty) {
        return static_cast<int>(bIndex);
    }
    if (b == flanker && a == Cell::Empty) {
        return static_cast<int>(aIndex);
    }
    return -1;
}

// True if `player` has a Free disc that can move onto the empty square `square` in one
// move under `style`. This is what separates a threat from a shape that merely looks like
// one: a half-pin nobody can complete costs the opponent nothing, and paying for it
// misprices every position where the completing square is walled off.
//
// The flanker holding the pin can never be the disc found here, so it needs no excluding:
// the axis reads [flanker][target][empty], and a ray cast from the empty square toward
// the flanker meets the target disc first and stops.
//
// Checked: that a mover of the right colour exists with a clear line to the square. NOT
// checked: whether the completing move would self-capture, or would repeat a position
// (super-ko). Both need game state this module deliberately does not hold, and both make
// the count too high rather than too low -- the same direction the term already errs in
// by ignoring leap chains under StepLeap.
bool canOccupy(const std::vector<Cell>& cells, int rows, int columns, int square,
               int player, MoveStyle style) {
    const int row = square / columns;
    const int column = square % columns;
    const Cell mine = freeCell(player);
    const int dRow[4] = {-1, 1, 0, 0};
    const int dColumn[4] = {0, 0, -1, 1};
    for (int k = 0; k < 4; ++k) {
        int nRow = row + dRow[k];
        int nColumn = column + dColumn[k];
        if (style == MoveStyle::Slide) {
            // Walk to the first occupied square on this ray: only that disc could slide
            // in, and only if it is mine.
            while (inBounds(nRow, nColumn, rows, columns) &&
                   cells[index(nRow, nColumn, columns)] == Cell::Empty) {
                nRow += dRow[k];
                nColumn += dColumn[k];
            }
        }
        if (!inBounds(nRow, nColumn, rows, columns)) {
            continue;
        }
        if (cells[index(nRow, nColumn, columns)] == mine) {
            return true;
        }
    }
    return false;
}

// True if the enemy disc at (row, column) is one completable move from being captured by
// `player`, on either axis.
bool isThreatened(const std::vector<Cell>& cells, int rows, int columns,
                  int row, int column, int player, MoveStyle style) {
    const Cell flanker = freeCell(player);
    const int dRow[2] = {1, 0};
    const int dColumn[2] = {0, 1};
    for (int k = 0; k < 2; ++k) {
        const int completion = halfPinCompletion(cells, rows, columns, row, column,
                                                 dRow[k], dColumn[k], flanker);
        if (completion >= 0 &&
            canOccupy(cells, rows, columns, completion, player, style)) {
            return true;
        }
    }
    return false;
}

int emptyNeighbours(const std::vector<Cell>& cells, int rows, int columns,
                    int row, int column) {
    const int dRow[4] = {-1, 1, 0, 0};
    const int dColumn[4] = {0, 0, -1, 1};
    int count = 0;
    for (int k = 0; k < 4; ++k) {
        const int nRow = row + dRow[k];
        const int nColumn = column + dColumn[k];
        if (!inBounds(nRow, nColumn, rows, columns)) {
            continue;
        }
        if (cells[index(nRow, nColumn, columns)] == Cell::Empty) {
            ++count;
        }
    }
    return count;
}

// Empty squares along the four rays out of (row, column), stopping at the first occupied
// square of either colour: the exact destination count of a slide. Mirrors
// Game::collectSlides -- if that blocking rule changes, this must change with it.
int slideDestinations(const std::vector<Cell>& cells, int rows, int columns,
                      int row, int column) {
    const int dRow[4] = {-1, 1, 0, 0};
    const int dColumn[4] = {0, 0, -1, 1};
    int count = 0;
    for (int k = 0; k < 4; ++k) {
        int nRow = row + dRow[k];
        int nColumn = column + dColumn[k];
        while (inBounds(nRow, nColumn, rows, columns) &&
               cells[index(nRow, nColumn, columns)] == Cell::Empty) {
            ++count;
            nRow += dRow[k];
            nColumn += dColumn[k];
        }
    }
    return count;
}

// The mobility proxy appropriate to `style`; see PositionalTerms::openNeighbours.
int mobilityProxy(const std::vector<Cell>& cells, int rows, int columns,
                  int row, int column, MoveStyle style) {
    if (style == MoveStyle::Slide) {
        return slideDestinations(cells, rows, columns, row, column);
    }
    return emptyNeighbours(cells, rows, columns, row, column);
}

}  // namespace

PositionalTerms positionalTerms(const std::vector<Cell>& cells, int rows, int columns,
                                int player, MoveStyle style) {
    if (rows < 1 || columns < 1) {
        throw std::invalid_argument("Latrunculi eval: rows and columns must be >= 1");
    }
    const std::size_t expected =
        static_cast<std::size_t>(rows) * static_cast<std::size_t>(columns);
    if (cells.size() != expected) {
        throw std::invalid_argument("Latrunculi eval: cells.size() != rows*columns");
    }
    if (player != 0 && player != 1) {
        throw std::invalid_argument("Latrunculi eval: player must be 0 or 1");
    }

    const Cell mine = freeCell(player);
    const Cell theirs = freeCell(1 - player);

    PositionalTerms terms;
    for (int row = 0; row < rows; ++row) {
        for (int column = 0; column < columns; ++column) {
            const Cell cell = cells[index(row, column, columns)];

            // Only Free discs can be threatened: a Bound disc is already captured, and a
            // Bound flanker does not pin (it mirrors Game::pinnedOn).
            if (cell == theirs) {
                if (isThreatened(cells, rows, columns, row, column, player, style)) {
                    ++terms.threats;
                }
                continue;
            }
            if (cell != mine) {
                continue;
            }

            terms.centrality += centrality(row, column, rows, columns);
            terms.openNeighbours += mobilityProxy(cells, rows, columns, row, column, style);
            // Look only right and down so each pair is counted exactly once.
            if (column + 1 < columns && cells[index(row, column + 1, columns)] == mine) {
                ++terms.pairs;
            }
            if (row + 1 < rows && cells[index(row + 1, column, columns)] == mine) {
                ++terms.pairs;
            }
        }
    }
    return terms;
}

double positionalScore(const PositionalTerms& terms) {
    return kThreatWeight * terms.threats +
           kPairWeight * terms.pairs +
           kMobilityWeight * terms.openNeighbours +
           kCentreWeight * terms.centrality;
}

}  // namespace Latrunculi
// Copyright Ben Paul Wise. All Rights Reserved.

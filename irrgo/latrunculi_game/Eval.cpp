// Copyright Ben Paul Wise. All Rights Reserved.

#include "Eval.h"

#include <cmath>
#include <cstddef>
#include <stdexcept>

namespace Latrunculi {

namespace {

// The weights themselves live in EvalWeights (EvalWeights.h), defaults included, so the
// bench can inject swept candidates at runtime; the justification comments moved there
// too.

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

}  // namespace

// ── Shared flanking predicates (declared in Eval.h) ──────────────────────────

// See the declaration comment: half-pin detection along one axis. Returns the index of
// the empty completing square, or -1.
int halfPinCompletion(const std::vector<Cell>& cells, int rows, int columns,
                      int row, int column, int dRow, int dColumn, Cell flanker) {
    const int aRow = row - dRow;
    const int aColumn = column - dColumn;
    const int bRow = row + dRow;
    const int bColumn = column + dColumn;
    if (!onBoard(aRow, aColumn, rows, columns) || !onBoard(bRow, bColumn, rows, columns)) {
        return -1;
    }
    const std::size_t aIndex = cellIndex(aRow, aColumn, columns);
    const std::size_t bIndex = cellIndex(bRow, bColumn, columns);
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
    for (int k = 0; k < 4; ++k) {
        int nRow = row + kDRow[k];
        int nColumn = column + kDColumn[k];
        if (style == MoveStyle::Slide) {
            // Walk to the first occupied square on this ray: only that disc could slide
            // in, and only if it is mine.
            while (onBoard(nRow, nColumn, rows, columns) &&
                   cells[cellIndex(nRow, nColumn, columns)] == Cell::Empty) {
                nRow += kDRow[k];
                nColumn += kDColumn[k];
            }
        }
        if (!onBoard(nRow, nColumn, rows, columns)) {
            continue;
        }
        if (cells[cellIndex(nRow, nColumn, columns)] == mine) {
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
    for (int k = 0; k < 2; ++k) {
        const int completion = halfPinCompletion(cells, rows, columns, row, column,
                                                 kAxisDRow[k], kAxisDColumn[k], flanker);
        if (completion >= 0 &&
            canOccupy(cells, rows, columns, completion, player, style)) {
            return true;
        }
    }
    return false;
}

namespace {

int emptyNeighbours(const std::vector<Cell>& cells, int rows, int columns,
                    int row, int column) {
    int count = 0;
    for (int k = 0; k < 4; ++k) {
        const int nRow = row + kDRow[k];
        const int nColumn = column + kDColumn[k];
        if (!onBoard(nRow, nColumn, rows, columns)) {
            continue;
        }
        if (cells[cellIndex(nRow, nColumn, columns)] == Cell::Empty) {
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
    int count = 0;
    for (int k = 0; k < 4; ++k) {
        int nRow = row + kDRow[k];
        int nColumn = column + kDColumn[k];
        while (onBoard(nRow, nColumn, rows, columns) &&
               cells[cellIndex(nRow, nColumn, columns)] == Cell::Empty) {
            ++count;
            nRow += kDRow[k];
            nColumn += kDColumn[k];
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
            const Cell cell = cells[cellIndex(row, column, columns)];

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
            if (column + 1 < columns && cells[cellIndex(row, column + 1, columns)] == mine) {
                ++terms.pairs;
            }
            if (row + 1 < rows && cells[cellIndex(row + 1, column, columns)] == mine) {
                ++terms.pairs;
            }
        }
    }
    return terms;
}

void validateEvalWeights(const EvalWeights& weights) {
    const double fields[] = {
        weights.threat, weights.pair, weights.mobility, weights.centre,
        weights.vulnerableAxes, weights.oneMoveCapturable, weights.spearheadPairs,
        weights.diagonalSupport, weights.deniedSquares, weights.strikers,
        weights.notchExposure,
    };
    for (double f : fields) {
        if (!std::isfinite(f)) {
            throw std::invalid_argument("Latrunculi eval: weight is not finite");
        }
    }
}

double positionalScore(const PositionalTerms& terms, const EvalWeights& weights) {
    return weights.threat * terms.threats +
           weights.pair * terms.pairs +
           weights.mobility * terms.openNeighbours +
           weights.centre * terms.centrality;
}

}  // namespace Latrunculi
// Copyright Ben Paul Wise. All Rights Reserved.

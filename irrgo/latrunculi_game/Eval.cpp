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
constexpr double kPairWeight     = 0.10;
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

// True if the disc at (row, column) is half-pinned along the axis (dRow, dColumn) by
// `flanker`: one end of the axis holds a flanker disc and the other end is on the board
// and empty, so a single move into it completes the custodial capture.
bool halfPinnedOnAxis(const std::vector<Cell>& cells, int rows, int columns,
                      int row, int column, int dRow, int dColumn, Cell flanker) {
    const int aRow = row - dRow;
    const int aColumn = column - dColumn;
    const int bRow = row + dRow;
    const int bColumn = column + dColumn;
    if (!inBounds(aRow, aColumn, rows, columns) || !inBounds(bRow, bColumn, rows, columns)) {
        return false;
    }
    const Cell a = cells[index(aRow, aColumn, columns)];
    const Cell b = cells[index(bRow, bColumn, columns)];
    return (a == flanker && b == Cell::Empty) || (b == flanker && a == Cell::Empty);
}

bool halfPinned(const std::vector<Cell>& cells, int rows, int columns,
                int row, int column, Cell flanker) {
    return halfPinnedOnAxis(cells, rows, columns, row, column, 1, 0, flanker) ||
           halfPinnedOnAxis(cells, rows, columns, row, column, 0, 1, flanker);
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

}  // namespace

PositionalTerms positionalTerms(const std::vector<Cell>& cells, int rows, int columns,
                                int player) {
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
                if (halfPinned(cells, rows, columns, row, column, mine)) {
                    ++terms.threats;
                }
                continue;
            }
            if (cell != mine) {
                continue;
            }

            terms.centrality += centrality(row, column, rows, columns);
            terms.openNeighbours += emptyNeighbours(cells, rows, columns, row, column);
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

// Copyright Ben Paul Wise. All Rights Reserved.

#include "PlacementEval.h"

#include <algorithm>
#include <stdexcept>

namespace Latrunculi {

namespace {

// Value of seamRamp(0): the fraction of the full oneMoveCapturable penalty charged at
// the very start of placement. A first guess awaiting the Stage C sweep, like the
// weights themselves.
constexpr double kSeamRampFloor = 0.25;

// True if `c` could someday hold an enemy flanker: empty now, or already an enemy Free
// disc. An own disc, an own Bound disc, or an enemy Bound disc all block the square
// forever as far as flanking is concerned (a Bound flanker does not pin -- see
// Game::pinnedOn).
bool openToEnemyFlanker(Cell c, Cell enemyFree) {
    return c == Cell::Empty || c == enemyFree;
}

bool isCornerSquare(int row, int column, int rows, int columns) {
    return (row == 0 || row == rows - 1) && (column == 0 || column == columns - 1);
}

// The corner-trap analogue of an axis half-pin for a disc sitting IN a corner: one of
// the two squares beside the corner holds an enemy Free disc, the other is empty, and
// an enemy Free disc can reach that empty square in one move. Eval.h's isThreatened
// deliberately skips the corner rule (it mirrors the axis-only pinnedOn scan the
// movement terms grew up with); placement cares because corners are exactly where
// anchors go (heuristic H1, penalty A3).
bool cornerThreatened(const std::vector<Cell>& cells, int rows, int columns,
                      int row, int column, int enemyPlayer, MoveStyle style) {
    const int hColumn = (column == 0) ? 1 : columns - 2;
    const int vRow = (row == 0) ? 1 : rows - 2;
    if (!onBoard(row, hColumn, rows, columns) || !onBoard(vRow, column, rows, columns)) {
        // A board one square wide on some axis has no second square beside the corner,
        // so the corner trap cannot be formed at all.
        return false;
    }
    const std::size_t hIndex = cellIndex(row, hColumn, columns);
    const std::size_t vIndex = cellIndex(vRow, column, columns);
    const Cell h = cells[hIndex];
    const Cell v = cells[vIndex];
    const Cell enemyFree = freeCell(enemyPlayer);
    if (h == enemyFree && v == Cell::Empty) {
        return canOccupy(cells, rows, columns, static_cast<int>(vIndex), enemyPlayer,
                         style);
    }
    if (v == enemyFree && h == Cell::Empty) {
        return canOccupy(cells, rows, columns, static_cast<int>(hIndex), enemyPlayer,
                         style);
    }
    return false;
}

// The corner analogue of a vulnerable axis: both squares beside the corner are open to
// an enemy flanker, so the corner trap is still buildable against this disc.
bool cornerVulnerable(const std::vector<Cell>& cells, int rows, int columns,
                      int row, int column, Cell enemyFree) {
    const int hColumn = (column == 0) ? 1 : columns - 2;
    const int vRow = (row == 0) ? 1 : rows - 2;
    if (!onBoard(row, hColumn, rows, columns) || !onBoard(vRow, column, rows, columns)) {
        return false;
    }
    return openToEnemyFlanker(cells[cellIndex(row, hColumn, columns)], enemyFree) &&
           openToEnemyFlanker(cells[cellIndex(vRow, column, columns)], enemyFree);
}

// True if the Free disc at (row, column) qualifies as a striker under `style`: enough
// immediate mobility to bring it to a completing square. Under Slide that is two or
// more rays of length >= 2 (a length-1 ray only shuffles into an adjacent square);
// under StepLeap, two or more empty orthogonal neighbours. Leap chains are ignored,
// in the same spirit as the mobility proxy in Eval.h.
bool isStriker(const std::vector<Cell>& cells, int rows, int columns,
               int row, int column, MoveStyle style) {
    int mobileDirections = 0;
    for (int k = 0; k < 4; ++k) {
        int nRow = row + kDRow[k];
        int nColumn = column + kDColumn[k];
        if (!onBoard(nRow, nColumn, rows, columns) ||
            cells[cellIndex(nRow, nColumn, columns)] != Cell::Empty) {
            continue;
        }
        if (style == MoveStyle::StepLeap) {
            ++mobileDirections;
            continue;
        }
        // Slide: the ray must run at least two empty squares.
        const int nnRow = nRow + kDRow[k];
        const int nnColumn = nColumn + kDColumn[k];
        if (onBoard(nnRow, nnColumn, rows, columns) &&
            cells[cellIndex(nnRow, nnColumn, columns)] == Cell::Empty) {
            ++mobileDirections;
        }
    }
    return mobileDirections >= 2;
}

}  // namespace

double seamRamp(double progress) {
    return kSeamRampFloor + (1.0 - kSeamRampFloor) * progress * progress * progress;
}

PlacementTerms placementTerms(const std::vector<Cell>& cells, int rows, int columns,
                              int player, MoveStyle style) {
    if (rows < 1 || columns < 1) {
        throw std::invalid_argument("Latrunculi placement eval: rows and columns must be >= 1");
    }
    const std::size_t expected =
        static_cast<std::size_t>(rows) * static_cast<std::size_t>(columns);
    if (cells.size() != expected) {
        throw std::invalid_argument("Latrunculi placement eval: cells.size() != rows*columns");
    }
    if (player != 0 && player != 1) {
        throw std::invalid_argument("Latrunculi placement eval: player must be 0 or 1");
    }

    const int enemy = 1 - player;
    const Cell mine = freeCell(player);
    const Cell enemyFree = freeCell(enemy);

    PlacementTerms terms;
    for (int row = 0; row < rows; ++row) {
        for (int column = 0; column < columns; ++column) {
            const Cell cell = cells[cellIndex(row, column, columns)];

            if (cell == Cell::Empty) {
                // deniedSquares: flanked by two of my Free discs on either axis; count
                // the square once even when both axes deny it.
                for (int k = 0; k < 2; ++k) {
                    const int aRow = row - kAxisDRow[k];
                    const int aColumn = column - kAxisDColumn[k];
                    const int bRow = row + kAxisDRow[k];
                    const int bColumn = column + kAxisDColumn[k];
                    if (onBoard(aRow, aColumn, rows, columns) &&
                        onBoard(bRow, bColumn, rows, columns) &&
                        cells[cellIndex(aRow, aColumn, columns)] == mine &&
                        cells[cellIndex(bRow, bColumn, columns)] == mine) {
                        ++terms.deniedSquares;
                        break;
                    }
                }
                continue;
            }
            if (cell != mine) {
                continue;
            }

            // ── Terms counted per own Free disc ──
            const bool corner = isCornerSquare(row, column, rows, columns);

            // vulnerableAxes (F1-F3).
            for (int k = 0; k < 2; ++k) {
                const int aRow = row - kAxisDRow[k];
                const int aColumn = column - kAxisDColumn[k];
                const int bRow = row + kAxisDRow[k];
                const int bColumn = column + kAxisDColumn[k];
                if (onBoard(aRow, aColumn, rows, columns) &&
                    onBoard(bRow, bColumn, rows, columns) &&
                    openToEnemyFlanker(cells[cellIndex(aRow, aColumn, columns)], enemyFree) &&
                    openToEnemyFlanker(cells[cellIndex(bRow, bColumn, columns)], enemyFree)) {
                    ++terms.vulnerableAxes;
                }
            }
            if (corner && cornerVulnerable(cells, rows, columns, row, column, enemyFree)) {
                ++terms.vulnerableAxes;
            }

            // oneMoveCapturable (A1): axis pins via the shared predicate (colors
            // swapped: from the enemy's point of view this disc is the enemy disc),
            // plus the corner trap isThreatened does not model.
            if (isThreatened(cells, rows, columns, row, column, enemy, style) ||
                (corner &&
                 cornerThreatened(cells, rows, columns, row, column, enemy, style))) {
                ++terms.oneMoveCapturable;
            }

            // strikers (H6).
            if (isStriker(cells, rows, columns, row, column, style)) {
                ++terms.strikers;
            }

            // notchExposure (A2/F7): half-pinned on BOTH axes -- each axis has an
            // enemy Free disc on one side and an empty square on the other. Merely
            // touching enemies on both axes is not exposure: a friend or an edge on
            // the far side blocks that axis for good, which is how a 2x2 block sits
            // safely against four attackers.
            if (halfPinCompletion(cells, rows, columns, row, column,
                                  kAxisDRow[0], kAxisDColumn[0], enemyFree) >= 0 &&
                halfPinCompletion(cells, rows, columns, row, column,
                                  kAxisDRow[1], kAxisDColumn[1], enemyFree) >= 0) {
                ++terms.notchExposure;
            }

            // spearheadPairs (F4) and diagonalSupport (F6): scan right/down (and the
            // down-left diagonal) so each pair is counted exactly once, mirroring the
            // pairs count in positionalTerms.
            for (int k = 0; k < 2; ++k) {
                const int dRow = kAxisDRow[k];
                const int dColumn = kAxisDColumn[k];
                const int pRow = row + dRow;
                const int pColumn = column + dColumn;
                if (!onBoard(pRow, pColumn, rows, columns) ||
                    cells[cellIndex(pRow, pColumn, columns)] != mine) {
                    continue;
                }
                // A pair along +d: its two outward extensions are (row,column)-d and
                // (pRow,pColumn)+d. Each extension holding an enemy Free disc with an
                // empty on-board square beyond it is one spearhead.
                const int extensions[2][4] = {
                    {row - dRow, column - dColumn, row - 2 * dRow, column - 2 * dColumn},
                    {pRow + dRow, pColumn + dColumn, pRow + 2 * dRow, pColumn + 2 * dColumn},
                };
                for (const int(&e)[4] : extensions) {
                    if (onBoard(e[0], e[1], rows, columns) &&
                        cells[cellIndex(e[0], e[1], columns)] == enemyFree &&
                        onBoard(e[2], e[3], rows, columns) &&
                        cells[cellIndex(e[2], e[3], columns)] == Cell::Empty) {
                        ++terms.spearheadPairs;
                    }
                }
            }
            for (int dColumn : {1, -1}) {
                const int dRow = 1;  // down-right and down-left; up-diagonals are the
                                     // same pairs seen from the other end
                const int pRow = row + dRow;
                const int pColumn = column + dColumn;
                if (!onBoard(pRow, pColumn, rows, columns) ||
                    cells[cellIndex(pRow, pColumn, columns)] != mine) {
                    continue;
                }
                // The two notch squares sit at the off-diagonal corners of the 2x2 the
                // pair spans.
                if (cells[cellIndex(row, pColumn, columns)] == Cell::Empty &&
                    cells[cellIndex(pRow, column, columns)] == Cell::Empty) {
                    ++terms.diagonalSupport;
                }
            }
        }
    }
    return terms;
}

double placementScore(const PlacementTerms& terms, double progress, int perSide,
                      const EvalWeights& weights) {
    if (!(progress >= 0.0 && progress <= 1.0)) {
        throw std::invalid_argument(
            "Latrunculi placement eval: progress must be in [0, 1]");
    }
    if (perSide < 1) {
        throw std::invalid_argument("Latrunculi placement eval: perSide must be >= 1");
    }
    // Strikers saturate: beyond the cap another mobile disc adds no attack the first
    // ones could not deliver, and an unbounded bonus would pay for pure dispersal.
    const int strikerCap = std::max(3, perSide / 5);
    const int cappedStrikers = std::min(terms.strikers, strikerCap);
    return weights.vulnerableAxes * terms.vulnerableAxes +
           weights.oneMoveCapturable * seamRamp(progress) * terms.oneMoveCapturable +
           weights.spearheadPairs * terms.spearheadPairs +
           weights.diagonalSupport * terms.diagonalSupport +
           weights.deniedSquares * terms.deniedSquares +
           weights.strikers * cappedStrikers +
           weights.notchExposure * terms.notchExposure;
}

}  // namespace Latrunculi
// Copyright Ben Paul Wise. All Rights Reserved.

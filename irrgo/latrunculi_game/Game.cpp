// Copyright Ben Paul Wise. All Rights Reserved.

#include "Game.h"

#include <cstddef>
#include <stdexcept>

namespace Latrunculi {

namespace {

// Cell <-> player helpers. playerOf returns -1 for Empty.
int playerOf(Cell c) {
    switch (c) {
        case Cell::P0Free:
        case Cell::P0Bound:
            return 0;
        case Cell::P1Free:
        case Cell::P1Bound:
            return 1;
        default:
            return -1;
    }
}

Cell freeCell(int player) {
    return (player == 0) ? Cell::P0Free : Cell::P1Free;
}

Cell boundCell(int player) {
    return (player == 0) ? Cell::P0Bound : Cell::P1Bound;
}

constexpr double kWinBase = 1000.0;  // dominates non-terminal piece heuristics

}  // namespace

Game::Game(int rows, int columns, int perSide)
    : rows_(rows),
      columns_(columns),
      perSide_(perSide),
      squares_(rows * columns),
      board_(static_cast<std::size_t>(rows * columns), Cell::Empty) {
    if (rows < 1 || columns < 1) {
        throw std::invalid_argument("Latrunculi: rows and columns must be >= 1");
    }
    if (perSide < 0 || 2 * perSide > squares_) {
        throw std::invalid_argument("Latrunculi: need 2*perSide <= rows*columns");
    }
}

// ── Counting ─────────────────────────────────────────────────────────────────

int Game::totalDiscs(int player) const {
    int count = 0;
    for (Cell c : board_) {
        if (playerOf(c) == player) {
            ++count;
        }
    }
    return count;
}

int Game::freeDiscs(int player) const {
    int count = 0;
    for (Cell c : board_) {
        if (c == freeCell(player)) {
            ++count;
        }
    }
    return count;
}

int Game::boundDiscs(int player) const {
    int count = 0;
    for (Cell c : board_) {
        if (c == boundCell(player)) {
            ++count;
        }
    }
    return count;
}

int Game::enemyCaptiveCount(int me) const {
    return boundDiscs(1 - me);
}

int Game::ownerAt(int square) const {
    return playerOf(board_[static_cast<std::size_t>(square)]);
}

// ── Capture geometry ─────────────────────────────────────────────────────────

bool Game::freeAt(const std::vector<Cell>& b, int row, int column, int player) const {
    if (!inBounds(row, column)) {
        return false;
    }
    return b[static_cast<std::size_t>(idx(row, column))] == freeCell(player);
}

bool Game::pinnedOn(const std::vector<Cell>& b, int pos, int byPlayer) const {
    const int row = pos / columns_;
    const int column = pos % columns_;

    const bool left = freeAt(b, row, column - 1, byPlayer);
    const bool right = freeAt(b, row, column + 1, byPlayer);
    const bool up = freeAt(b, row - 1, column, byPlayer);
    const bool down = freeAt(b, row + 1, column, byPlayer);

    if ((left && right) || (up && down)) {
        return true;
    }

    const bool on_left_or_right = (column == 0 || column == columns_ - 1);
    const bool on_top_or_bottom = (row == 0 || row == rows_ - 1);
    if (on_left_or_right && on_top_or_bottom) {  // a board corner
        const bool horizontal = (column == 0) ? right : left;
        const bool vertical = (row == 0) ? down : up;
        if (horizontal && vertical) {
            return true;
        }
    }
    return false;
}

void Game::applyRemoveMoveCapturesTo(std::vector<Cell>& b, int removeSquare,
                                     int from, int to, int me) const {
    if (removeSquare >= 0 && removeSquare < squares_) {
        b[static_cast<std::size_t>(removeSquare)] = Cell::Empty;
    }
    b[static_cast<std::size_t>(to)] = b[static_cast<std::size_t>(from)];
    b[static_cast<std::size_t>(from)] = Cell::Empty;

    // Custodial capture: each enemy Free disc adjacent to the moved disc that is
    // now pinned by my Free discs flips to Bound (immobilised, not removed).
    const int opp = 1 - me;
    const int dr[4] = {-1, 1, 0, 0};
    const int dc[4] = {0, 0, -1, 1};
    const int tr = to / columns_;
    const int tc = to % columns_;
    for (int k = 0; k < 4; ++k) {
        const int nr = tr + dr[k];
        const int nc = tc + dc[k];
        if (!inBounds(nr, nc)) {
            continue;
        }
        const int n = idx(nr, nc);
        if (b[static_cast<std::size_t>(n)] == freeCell(opp) && pinnedOn(b, n, me)) {
            b[static_cast<std::size_t>(n)] = boundCell(opp);
        }
    }
}

// ── Move legality ────────────────────────────────────────────────────────────

bool Game::isLegalMovement(int removeSquare, int from, int to) const {
    const int me = current_;
    const int opp = 1 - me;

    if (from < 0 || from >= squares_ || board_[static_cast<std::size_t>(from)] != freeCell(me)) {
        return false;
    }
    if (to < 0 || to >= squares_) {
        return false;
    }
    // Single orthogonal step.
    const int dr = (to / columns_) - (from / columns_);
    const int dc = (to % columns_) - (from % columns_);
    const int adr = dr < 0 ? -dr : dr;
    const int adc = dc < 0 ? -dc : dc;
    if (!((adr == 1 && adc == 0) || (adr == 0 && adc == 1))) {
        return false;
    }

    // Mandatory captive removal when enemy captives exist; forbidden otherwise.
    const bool have_captives = enemyCaptiveCount(me) > 0;
    if (have_captives) {
        if (removeSquare < 0 || removeSquare >= squares_) {
            return false;
        }
        if (board_[static_cast<std::size_t>(removeSquare)] != boundCell(opp)) {
            return false;
        }
    } else {
        if (removeSquare >= 0) {
            return false;
        }
    }

    // Destination must be empty, or the square just vacated by the removal.
    const bool open = (board_[static_cast<std::size_t>(to)] == Cell::Empty) ||
                      (removeSquare >= 0 && to == removeSquare);
    if (!open) {
        return false;
    }

    // Reject self-capture: simulate, then check the moved disc is not pinned by
    // surviving enemy Free discs (capturing one flanker does not save the others).
    std::vector<Cell> b = board_;
    applyRemoveMoveCapturesTo(b, removeSquare, from, to, me);
    if (pinnedOn(b, to, opp)) {
        return false;
    }
    return true;
}

// ── Legal-move enumeration ───────────────────────────────────────────────────

std::vector<AbsGame::MoveId> Game::enumerateLegalMoves() const {
    std::vector<AbsGame::MoveId> moves;
    if (phase_ == Phase::Placement) {
        for (int s = 0; s < squares_; ++s) {
            if (board_[static_cast<std::size_t>(s)] == Cell::Empty) {
                moves.push_back(placementMove(s));
            }
        }
        return moves;
    }

    const int me = current_;
    const int opp = 1 - me;
    const bool have_captives = enemyCaptiveCount(me) > 0;

    std::vector<int> removals;
    if (have_captives) {
        for (int s = 0; s < squares_; ++s) {
            if (board_[static_cast<std::size_t>(s)] == boundCell(opp)) {
                removals.push_back(s);
            }
        }
    } else {
        removals.push_back(-1);  // no removal
    }

    const int dr[4] = {-1, 1, 0, 0};
    const int dc[4] = {0, 0, -1, 1};
    for (int rem : removals) {
        for (int from = 0; from < squares_; ++from) {
            if (board_[static_cast<std::size_t>(from)] != freeCell(me)) {
                continue;
            }
            const int fr = from / columns_;
            const int fc = from % columns_;
            for (int k = 0; k < 4; ++k) {
                const int nr = fr + dr[k];
                const int nc = fc + dc[k];
                if (!inBounds(nr, nc)) {
                    continue;
                }
                const int to = idx(nr, nc);
                if (isLegalMovement(rem, from, to)) {
                    moves.push_back(movementMove(from, to, rem));
                }
            }
        }
    }
    return moves;
}

std::vector<AbsGame::MoveId> Game::getLegalMoves() const {
    if (gameOver_) {
        return {};
    }
    return enumerateLegalMoves();
}

bool Game::isLegalMove(AbsGame::MoveId mv) const {
    if (gameOver_) {
        return false;
    }
    if (phase_ == Phase::Placement) {
        return mv >= 0 && mv < squares_ &&
               board_[static_cast<std::size_t>(mv)] == Cell::Empty;
    }
    int removeSquare = -1;
    int from = -1;
    int to = -1;
    decodeMovement(mv, removeSquare, from, to);
    return isLegalMovement(removeSquare, from, to);
}

// ── Move encoding ────────────────────────────────────────────────────────────

AbsGame::MoveId Game::movementMove(int from, int to, int removeSquare) const {
    const int remCode = (removeSquare < 0) ? squares_ : removeSquare;
    return (remCode * squares_ + from) * squares_ + to;
}

void Game::decodeMovement(AbsGame::MoveId mv, int& removeSquare, int& from, int& to) const {
    to = mv % squares_;
    const int rest = mv / squares_;
    from = rest % squares_;
    const int remCode = rest / squares_;
    removeSquare = (remCode == squares_) ? -1 : remCode;
}

// ── Applying moves ───────────────────────────────────────────────────────────

void Game::recordMove(int from, int to, int removed) {
    Move m;
    m.turn = static_cast<int>(moveHistory_.size()) + 1;
    m.player = current_;
    m.from = from;
    m.to = to;
    m.removed = removed;
    moveHistory_.push_back(m);
}

void Game::checkImmobilizationTerminal() {
    if (phase_ != Phase::Movement || gameOver_) {
        return;
    }
    if (enumerateLegalMoves().empty()) {
        gameOver_ = true;
        winner_ = 1 - current_;  // the player to move has no legal move and loses
    }
}

bool Game::applyMove(AbsGame::MoveId mv) {
    if (gameOver_) {
        return false;
    }

    if (phase_ == Phase::Placement) {
        if (mv < 0 || mv >= squares_ ||
            board_[static_cast<std::size_t>(mv)] != Cell::Empty) {
            return false;
        }
        board_[static_cast<std::size_t>(mv)] = freeCell(current_);
        ++placed_[current_];
        recordMove(-1, mv, -1);
        if (placed_[0] >= perSide_ && placed_[1] >= perSide_) {
            phase_ = Phase::Movement;
        }
        current_ = 1 - current_;
        checkImmobilizationTerminal();
        return true;
    }

    int removeSquare = -1;
    int from = -1;
    int to = -1;
    decodeMovement(mv, removeSquare, from, to);
    if (!isLegalMovement(removeSquare, from, to)) {
        return false;
    }

    applyRemoveMoveCapturesTo(board_, removeSquare, from, to, current_);
    ++movementPlies_;
    recordMove(from, to, removeSquare);

    // Win by reduction: the opponent (whose captive was just removable) is down
    // to a single disc.
    const int opp = 1 - current_;
    if (totalDiscs(opp) <= 1) {
        gameOver_ = true;
        winner_ = current_;
    }

    current_ = 1 - current_;
    checkImmobilizationTerminal();  // win by immobilisation, checked at turn start
    return true;
}

// ── Evaluation ───────────────────────────────────────────────────────────────

double Game::staticEval() const {
    const int me = current_;
    const int opp = 1 - me;

    if (gameOver_) {
        const int M = totalDiscs(winner_);
        const int N = totalDiscs(1 - winner_);
        const double denom = 3.0 * M + 2.0 * N;
        const double s = (denom > 0.0) ? (3.0 * M) / denom : 0.0;
        const double terminal = kWinBase + kWinBase * s;
        return (winner_ == me) ? terminal : -terminal;
    }

    // Non-terminal heuristic from the mover's perspective: Free discs are mobile
    // and can capture (full weight); Bound discs count half (enemy captives are an
    // advantage, our own captives a liability).
    const double freeDiff = static_cast<double>(freeDiscs(me) - freeDiscs(opp));
    const double boundDiff = static_cast<double>(boundDiscs(opp) - boundDiscs(me));
    return freeDiff + 0.5 * boundDiff;
}

std::unique_ptr<AbsGame::Game> Game::clone() const {
    return std::make_unique<Game>(*this);
}

}  // namespace Latrunculi
// Copyright Ben Paul Wise. All Rights Reserved.

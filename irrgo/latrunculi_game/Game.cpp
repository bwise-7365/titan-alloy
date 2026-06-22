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

constexpr double kWinBase = 1000.0;  // an actual win/loss dominates the leaf score

// The material-balance score s = 3M / (3M + 2N) for a side with M discs facing an
// opponent with N. This is the game's terminal score for the winner, and also the
// basis of the search's leaf evaluation, so reducing the opponent's pieces
// (capturing then removing) raises the score.
double materialScore(double M, double N) {
    const double denom = 3.0 * M + 2.0 * N;
    return (denom > 0.0) ? (3.0 * M) / denom : 0.0;
}

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

Game::Game(int rows, int columns, int perSide, std::vector<Cell> board,
           Phase phase, int current, int placed0, int placed1,
           std::vector<Move> history, std::unordered_set<std::uint64_t> seen)
    : rows_(rows),
      columns_(columns),
      perSide_(perSide),
      squares_(rows * columns),
      board_(std::move(board)),
      current_(current),
      phase_(phase),
      placed_{placed0, placed1},
      moveHistory_(std::move(history)),
      seenPositions_(std::move(seen)) {
    if (rows < 1 || columns < 1) {
        throw std::invalid_argument("Latrunculi: rows and columns must be >= 1");
    }
    if (static_cast<int>(board_.size()) != squares_) {
        throw std::invalid_argument("Latrunculi: board size must equal rows*columns");
    }
    if (current_ != 0 && current_ != 1) {
        throw std::invalid_argument("Latrunculi: current player must be 0 or 1");
    }
    // Recompute terminal status from the restored position so a loaded game that
    // is already decided reports correctly. Win by reduction (a side reduced to one
    // disc) takes precedence; otherwise the side to move may be immobilised.
    if (phase_ == Phase::Movement) {
        if (totalDiscs(0) <= 1) {
            gameOver_ = true;
            winner_ = 1;
        } else if (totalDiscs(1) <= 1) {
            gameOver_ = true;
            winner_ = 0;
        } else {
            checkImmobilizationTerminal();
        }
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

void Game::moveAndCapture(std::vector<Cell>& b, int from, int to, int me) const {
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

void Game::applyRemoveMoveCapturesTo(std::vector<Cell>& b, int removeSquare,
                                     int from, int to, int me) const {
    if (removeSquare >= 0 && removeSquare < squares_) {
        b[static_cast<std::size_t>(removeSquare)] = Cell::Empty;
    }
    moveAndCapture(b, from, to, me);
}

// ── Reachability: single step + own-colour multi-leaps ───────────────────────

void Game::collectLeaps(const std::vector<Cell>& b, int pos, std::vector<bool>& leapt,
                        std::vector<bool>& reach, int me) const {
    const int dr[4] = {-1, 1, 0, 0};
    const int dc[4] = {0, 0, -1, 1};
    const int pr = pos / columns_;
    const int pc = pos % columns_;
    for (int k = 0; k < 4; ++k) {
        const int mr = pr + dr[k];
        const int mc = pc + dc[k];      // disc to leap over
        const int lr = pr + 2 * dr[k];
        const int lc = pc + 2 * dc[k];  // landing square
        if (!inBounds(mr, mc) || !inBounds(lr, lc)) {
            continue;
        }
        const int mid = idx(mr, mc);
        const int land = idx(lr, lc);
        if (playerOf(b[static_cast<std::size_t>(mid)]) == me &&
            b[static_cast<std::size_t>(land)] == Cell::Empty &&
            !leapt[static_cast<std::size_t>(mid)]) {
            reach[static_cast<std::size_t>(land)] = true;
            leapt[static_cast<std::size_t>(mid)] = true;  // can't leap the same disc twice
            collectLeaps(b, land, leapt, reach, me);
            leapt[static_cast<std::size_t>(mid)] = false;
        }
    }
}

std::vector<bool> Game::reachableMask(const std::vector<Cell>& b, int from, int me) const {
    std::vector<bool> reach(static_cast<std::size_t>(squares_), false);
    const int dr[4] = {-1, 1, 0, 0};
    const int dc[4] = {0, 0, -1, 1};
    const int fr = from / columns_;
    const int fc = from % columns_;
    for (int k = 0; k < 4; ++k) {  // single orthogonal step onto an empty square
        const int nr = fr + dr[k];
        const int nc = fc + dc[k];
        if (inBounds(nr, nc) && b[static_cast<std::size_t>(idx(nr, nc))] == Cell::Empty) {
            reach[static_cast<std::size_t>(idx(nr, nc))] = true;
        }
    }
    std::vector<bool> leapt(static_cast<std::size_t>(squares_), false);
    collectLeaps(b, from, leapt, reach, me);  // `from` stays occupied in b, blocking returns
    return reach;
}

std::uint64_t Game::hashBoard(const std::vector<Cell>& b) const {
    std::uint64_t h = 1469598103934665603ULL;  // FNV-1a 64-bit offset basis
    for (Cell c : b) {
        h ^= static_cast<std::uint64_t>(static_cast<std::uint8_t>(c));
        h *= 1099511628211ULL;  // FNV-1a 64-bit prime
    }
    return h;
}

bool Game::moveIsLegalOn(const std::vector<Cell>& b, int from, int to, int me) const {
    std::vector<Cell> result = b;
    moveAndCapture(result, from, to, me);
    if (pinnedOn(result, to, 1 - me)) {
        return false;  // self-capture
    }
    if (seenPositions_.find(hashBoard(result)) != seenPositions_.end()) {
        return false;  // super-ko: this exact board has occurred earlier this game
    }
    return true;
}

// ── Move path for the move log ───────────────────────────────────────────────

bool Game::findLeapPath(const std::vector<Cell>& b, int pos, int to,
                        std::vector<bool>& leapt, int me, std::vector<int>& path) const {
    if (pos == to) {
        return true;
    }
    const int dr[4] = {-1, 1, 0, 0};
    const int dc[4] = {0, 0, -1, 1};
    const int pr = pos / columns_;
    const int pc = pos % columns_;
    for (int k = 0; k < 4; ++k) {
        const int mr = pr + dr[k];
        const int mc = pc + dc[k];
        const int lr = pr + 2 * dr[k];
        const int lc = pc + 2 * dc[k];
        if (!inBounds(mr, mc) || !inBounds(lr, lc)) {
            continue;
        }
        const int mid = idx(mr, mc);
        const int land = idx(lr, lc);
        if (playerOf(b[static_cast<std::size_t>(mid)]) == me &&
            b[static_cast<std::size_t>(land)] == Cell::Empty &&
            !leapt[static_cast<std::size_t>(mid)]) {
            path.push_back(land);
            leapt[static_cast<std::size_t>(mid)] = true;
            if (findLeapPath(b, land, to, leapt, me, path)) {
                return true;
            }
            leapt[static_cast<std::size_t>(mid)] = false;
            path.pop_back();
        }
    }
    return false;
}

std::vector<int> Game::movePath(const std::vector<Cell>& b, int from, int to, int me) const {
    const int dr = (to / columns_) - (from / columns_);
    const int dc = (to % columns_) - (from % columns_);
    const int adr = dr < 0 ? -dr : dr;
    const int adc = dc < 0 ? -dc : dc;
    if (adr + adc == 1) {
        return {from, to};  // a single orthogonal slide
    }
    std::vector<int> path{from};
    std::vector<bool> leapt(static_cast<std::size_t>(squares_), false);
    if (findLeapPath(b, from, to, leapt, me, path)) {
        return path;
    }
    return {from, to};  // fallback (should not happen for a legal move)
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

    // On the post-removal board, `to` must be reachable (single step or leap chain)
    // -- which also requires it to be empty -- and not a self-capture.
    std::vector<Cell> b = board_;
    if (removeSquare >= 0) {
        b[static_cast<std::size_t>(removeSquare)] = Cell::Empty;
    }
    const std::vector<bool> reach = reachableMask(b, from, me);
    if (!reach[static_cast<std::size_t>(to)]) {
        return false;
    }
    return moveIsLegalOn(b, from, to, me);
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

    for (int rem : removals) {
        std::vector<Cell> b = board_;
        if (rem >= 0) {
            b[static_cast<std::size_t>(rem)] = Cell::Empty;
        }
        for (int from = 0; from < squares_; ++from) {
            if (board_[static_cast<std::size_t>(from)] != freeCell(me)) {
                continue;
            }
            const std::vector<bool> reach = reachableMask(b, from, me);
            for (int to = 0; to < squares_; ++to) {
                if (reach[static_cast<std::size_t>(to)] &&
                    moveIsLegalOn(b, from, to, me)) {
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

void Game::recordMove(int from, int to, int removed, const std::vector<int>& path) {
    Move m;
    m.turn = static_cast<int>(moveHistory_.size()) + 1;
    m.player = current_;
    m.from = from;
    m.to = to;
    m.removed = removed;
    m.path = path;
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
        seenPositions_.insert(hashBoard(board_));
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

    // Reconstruct a representative path for the move log (post-removal board, with
    // the disc still at `from`) before mutating the real board.
    std::vector<Cell> b = board_;
    if (removeSquare >= 0) {
        b[static_cast<std::size_t>(removeSquare)] = Cell::Empty;
    }
    const std::vector<int> path = movePath(b, from, to, current_);

    applyRemoveMoveCapturesTo(board_, removeSquare, from, to, current_);
    seenPositions_.insert(hashBoard(board_));
    ++movementPlies_;
    recordMove(from, to, removeSquare, path);

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
        const double s = materialScore(totalDiscs(winner_), totalDiscs(1 - winner_));
        const double terminal = kWinBase + kWinBase * s;  // an actual result dominates
        return (winner_ == me) ? terminal : -terminal;
    }

    // Search-leaf (non-terminal) evaluation: the rules' material-balance score from
    // the mover's perspective. The side with more material (M) scores
    // v(M, N) = 3M/(3M+2N); the other scores -v(M, N); equal material scores 0.
    // (This is the MD-document score, not the symmetric difference v(M,N)-v(N,M).)
    // Each side's material counts Free discs fully and Bound (immobilised) discs at
    // immobilizationDiscount, so both immobilising and removing the opponent help.
    const double myMaterial = freeDiscs(me) + immobilizationDiscount * boundDiscs(me);
    const double opMaterial = freeDiscs(opp) + immobilizationDiscount * boundDiscs(opp);
    if (myMaterial > opMaterial) {
        return materialScore(myMaterial, opMaterial);   // mover ahead: +v(M, N)
    }
    if (opMaterial > myMaterial) {
        return -materialScore(opMaterial, myMaterial);  // opponent ahead: -v(M, N)
    }
    return 0.0;  // equal material
}

std::unique_ptr<AbsGame::Game> Game::clone() const {
    return std::make_unique<Game>(*this);
}

}  // namespace Latrunculi
// Copyright Ben Paul Wise. All Rights Reserved.

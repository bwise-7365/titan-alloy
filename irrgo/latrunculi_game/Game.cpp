// Copyright Ben Paul Wise. All Rights Reserved.

#include "Game.h"

#include "Eval.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <numeric>
#include <random>
#include <stdexcept>

namespace Latrunculi {

namespace {

// playerOf / freeCell / boundCell now live in Eval.h so Eval.cpp shares them rather
// than keeping a second copy.

// The material-balance score s = 3M / (3M + 2N) for a side with M discs facing an
// opponent with N (weights kMaterialSelfWeight/kMaterialOppWeight from Game.h, kWinBase
// likewise). Shapes the magnitude of a decisive terminal score in staticEval; the
// non-terminal leaf uses the pressure score there instead. Higher M (capturing then
// removing the opponent's pieces) raises the score.
double materialScore(double M, double N) {
    const double denom = kMaterialSelfWeight * M + kMaterialOppWeight * N;
    return (denom > 0.0) ? (kMaterialSelfWeight * M) / denom : 0.0;
}

// PayoffStyle::ConvexMargin. The winner's decisiveness as the disc margin raised to
// kMarginExponent, so the marginal value of one more disc rises with the lead rather than
// falling. See PayoffStyle in Game.h for why the Gradient form above had to be replaced.
double marginScore(double M, double N) {
    const double total = M + N;
    if (total <= 0.0) {
        // Unreachable: komi is required to be positive, so player 1's effective material
        // is positive even on an empty board. Saying so out loud beats dividing by
        // zero and letting an infinity propagate into the search as a plausible number.
        throw std::logic_error("Latrunculi: total material is not positive");
    }
    // A winner can hold LESS material than the loser: immobilisation ends the game on
    // legal moves, not on discs, so a player can be strangled while ahead. That is a win
    // with no material margin -- worth the bare win and no bonus -- so the margin floors
    // at zero rather than going negative and (at an even exponent) coming back positive.
    const double margin = std::max(0.0, (M - N) / total);
    return std::pow(margin, kMarginExponent);
}

// MCTS rollout policy (epsilon-greedy "heavy playout"). Random rollouts in
// Latrunculi almost never set up a capture, so they neither terminate nor move the
// material eval -- the playout is uninformative and MCTS degrades toward noise.
// With probability kRolloutEpsilon we still play a uniform-random move (keeping
// exploration); otherwise we bias toward aggressive (capturing / decisive) moves,
// sampling at most kRolloutSampleCap candidates per ply to stay cheap. epsilon ~
// 0.3 is the literature sweet spot; pure greedy (epsilon = 0) is known to harm
// strength. Design references (kept here so the rationale is retrievable):
//   Swiechowski et al., "MCTS: A Review of Recent Modifications and Applications":
//       https://arxiv.org/pdf/2103.04931
//   Heavy playouts: Drake & Uurtamo (2007) -- no stable open URL; see review above.
//   Teytaud & Teytaud, "On the Huge Benefit of Decisive Moves in MCTS" (2010):
//       https://inria.hal.science/inria-00495078/en
//   epsilon-greedy playouts in practice (Scopone; best epsilon ~ 0.3):
//       https://arxiv.org/pdf/1807.06813
//   "Using evaluation functions in MCTS" (truncated rollouts / early termination):
//       https://www.sciencedirect.com/science/article/pii/S0304397516302717
//   Baier & Winands, "MCTS-Minimax Hybrids with State Evaluations" (alternative):
//       https://www.jair.org/index.php/jair/article/download/11208/26419/20772
//   Lanctot et al., "MCTS with Heuristic Evaluations using Implicit Minimax Backups":
//       https://arxiv.org/pdf/1406.0486
constexpr double kRolloutEpsilon  = 0.3;
constexpr int    kRolloutSampleCap = 12;

// Move-ordering weights for the alpha-beta searcher (see Game::moveOrderScore and
// AbsGame::Game::moveOrderScore). Alpha-beta prunes in proportion to how early the
// strongest move is tried, and in this rule set the strongest moves are the ones that
// take material. A removal deletes an enemy disc permanently; a custodial capture only
// binds one, and the opponent may free it again by pinning a flanker, so a removal is
// weighted higher. Quiet moves score 0 and keep their enumeration order.
constexpr int kOrderDecisive = 10000;  // reduces the opponent to a single disc: a win
constexpr int kOrderRemoval  = 10;     // per enemy disc permanently removed
constexpr int kOrderBind     = 5;      // per enemy Free disc newly immobilised

// ── Zobrist keys (super-ko hashing) ─────────────────────────────────────────
// One random 64-bit key per (square, cell state), Empty included, so a square's
// contribution can be XORed out and its replacement XORed in. That makes the hash of a
// candidate position O(1) in the number of cells the move touches instead of O(squares),
// which matters because legal-move enumeration hashes once per candidate and the slide
// rule set raised the candidate count several-fold.
//
// The seed is fixed rather than clock-derived. The keys must be identical across every
// Game and every clone in a run for super-ko to mean anything, and a fixed seed also
// makes a hash collision reproducible if one is ever suspected.
constexpr int kZobristStates  = 5;    // Cell has five values
constexpr int kZobristSquares = 256;  // 16x16; boards larger than this are rejected

const std::array<std::uint64_t, kZobristSquares * kZobristStates>& zobristKeys() {
    static const std::array<std::uint64_t, kZobristSquares * kZobristStates> keys = [] {
        std::array<std::uint64_t, kZobristSquares * kZobristStates> k{};
        std::mt19937_64 rng(0x9E3779B97F4A7C15ULL);
        for (std::uint64_t& v : k) {
            v = rng();
        }
        return k;
    }();
    return keys;
}

std::uint64_t cellKey(int square, Cell c) {
    return zobristKeys()[static_cast<std::size_t>(square) * kZobristStates +
                         static_cast<std::size_t>(c)];
}

}  // namespace

// Disc counts are integers, so an integral komi lets the two sides tie exactly -- and this
// engine has no draw to report that with: every terminal names a winner and winnerScore()
// throws without one. Rejecting it up front is the difference between a clear error at the
// point of entry and a logic_error thrown from inside a search hours later.
void validateKomi(double komi) {
    if (!(komi > 0.0)) {
        throw std::invalid_argument("Latrunculi: komi must be positive");
    }
    if (komi == std::floor(komi)) {
        throw std::invalid_argument(
            "Latrunculi: komi must not be a whole number (it would allow an exact tie)");
    }
}

Game::Game(int rows, int columns, int perSide, MoveStyle style, PayoffStyle payoff,
           double komi)
    : rows_(rows),
      columns_(columns),
      perSide_(perSide),
      squares_(rows * columns),
      moveStyle_(style),
      payoffStyle_(payoff),
      komi_(komi),
      board_(static_cast<std::size_t>(rows * columns), Cell::Empty) {
    if (rows < 1 || columns < 1) {
        throw std::invalid_argument("Latrunculi: rows and columns must be >= 1");
    }
    if (perSide < 0 || 2 * perSide > squares_) {
        throw std::invalid_argument("Latrunculi: need 2*perSide <= rows*columns");
    }
    if (squares_ > kZobristSquares) {
        throw std::invalid_argument("Latrunculi: board exceeds the Zobrist key table");
    }
    validateKomi(komi_);
    hash_ = hashBoard(board_);
    initScanOrder();
}

Game::Game(int rows, int columns, int perSide, std::vector<Cell> board,
           Phase phase, int current, int placed0, int placed1,
           std::vector<Move> history, std::unordered_set<std::uint64_t> seen,
           MoveStyle style, PayoffStyle payoff, double komi)
    : rows_(rows),
      columns_(columns),
      perSide_(perSide),
      squares_(rows * columns),
      moveStyle_(style),
      payoffStyle_(payoff),
      komi_(komi),
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
    if (squares_ > kZobristSquares) {
        throw std::invalid_argument("Latrunculi: board exceeds the Zobrist key table");
    }
    validateKomi(komi_);
    // Seed the incremental hash and build the scan order BEFORE any terminal recompute:
    // checkImmobilizationTerminal() below calls enumerateLegalMoves(), which indexes
    // scanOrder_ and reads hash_, so both must already be valid.
    hash_ = hashBoard(board_);
    initScanOrder();
    // Recompute terminal status from the restored position so a loaded game that
    // is already decided reports correctly. Win by reduction (a side reduced to one
    // disc) takes precedence; otherwise the side to move may be immobilised.
    //
    // Reduction is checked only in movement: during placement a side legitimately holds
    // one disc or none, and reading that as a rout would end the game at ply 1.
    if (phase_ == Phase::Movement) {
        if (totalDiscs(0) <= 1) {
            gameOver_ = true;
            winner_ = 1;
            winReason_ = WinReason::Reduction;
        } else if (totalDiscs(1) <= 1) {
            gameOver_ = true;
            winner_ = 0;
            winReason_ = WinReason::Reduction;
        } else {
            checkImmobilizationTerminal();
        }
    } else {
        checkImmobilizationTerminal();
    }
}

void Game::initScanOrder() {
    scanOrder_.resize(static_cast<std::size_t>(squares_));
    std::iota(scanOrder_.begin(), scanOrder_.end(), 0);
    // Clock-derived seed: a fresh random scan order for each new game. clone()
    // copies scanOrder_, so a search sees the same order as the live game.
    std::mt19937_64 rng(AbsGame::makeSeed(0));
    std::shuffle(scanOrder_.begin(), scanOrder_.end(), rng);
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

void Game::setCell(std::vector<Cell>& b, int square, Cell c, std::uint64_t* hash) const {
    const std::size_t i = static_cast<std::size_t>(square);
    if (hash != nullptr) {
        *hash ^= cellKey(square, b[i]) ^ cellKey(square, c);
    }
    b[i] = c;
}

void Game::moveAndCapture(std::vector<Cell>& b, int from, int to, int me,
                          std::uint64_t* hash) const {
    setCell(b, to, b[static_cast<std::size_t>(from)], hash);
    setCell(b, from, Cell::Empty, hash);

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
            setCell(b, n, boundCell(opp), hash);
        }
    }
}

void Game::applyRemoveMoveCapturesTo(std::vector<Cell>& b, int removeSquare,
                                     int from, int to, int me,
                                     std::uint64_t* hash) const {
    if (removeSquare >= 0 && removeSquare < squares_) {
        setCell(b, removeSquare, Cell::Empty, hash);
    }
    moveAndCapture(b, from, to, me, hash);
}

// ── Reachability: rook slide, or single step + own-color multi-leaps ─────────

void Game::collectSlides(const std::vector<Cell>& b, int from,
                         std::vector<bool>& reach) const {
    const int dr[4] = {-1, 1, 0, 0};
    const int dc[4] = {0, 0, -1, 1};
    const int fr = from / columns_;
    const int fc = from % columns_;
    for (int k = 0; k < 4; ++k) {
        int r = fr + dr[k];
        int c = fc + dc[k];
        // Stop at the first occupied square: a disc of EITHER colour blocks the ray, and
        // is not itself a destination -- there is no capture by displacement in this
        // rule set, only custodial capture resolved after the move lands.
        while (inBounds(r, c) && b[static_cast<std::size_t>(idx(r, c))] == Cell::Empty) {
            reach[static_cast<std::size_t>(idx(r, c))] = true;
            r += dr[k];
            c += dc[k];
        }
    }
}

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
    if (moveStyle_ == MoveStyle::Slide) {
        collectSlides(b, from, reach);
        return reach;  // the slide subsumes the step; leaps do not exist in this rule set
    }
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
    std::uint64_t h = 0;
    for (int s = 0; s < squares_; ++s) {
        h ^= cellKey(s, b[static_cast<std::size_t>(s)]);
    }
    return h;
}

bool Game::moveIsLegalOn(const std::vector<Cell>& b, int from, int to, int me,
                         std::uint64_t baseHash, std::vector<Cell>& scratch) const {
    scratch = b;  // reuses scratch's buffer once the caller has looped at least once
    std::uint64_t h = baseHash;
    moveAndCapture(scratch, from, to, me, &h);
    if (pinnedOn(scratch, to, 1 - me)) {
        return false;  // self-capture
    }
    if (seenPositions_.find(h) != seenPositions_.end()) {
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
    if (moveStyle_ == MoveStyle::Slide) {
        // A slide is one straight run along a rank or file, so there are no intermediate
        // landings to record -- the endpoints describe the move completely.
        return {from, to};
    }
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
    std::uint64_t baseHash = hash_;
    if (removeSquare >= 0) {
        setCell(b, removeSquare, Cell::Empty, &baseHash);
    }
    const std::vector<bool> reach = reachableMask(b, from, me);
    if (!reach[static_cast<std::size_t>(to)]) {
        return false;
    }
    std::vector<Cell> scratch;
    return moveIsLegalOn(b, from, to, me, baseHash, scratch);
}

// ── Legal-move enumeration ───────────────────────────────────────────────────

bool Game::isLegalPlacement(int square) const {
    if (square < 0 || square >= squares_) {
        return false;
    }
    if (board_[static_cast<std::size_t>(square)] != Cell::Empty) {
        return false;
    }
    // No captures of ANY kind may be made during the Placement Phase. Simulate the
    // placement and reject it if it would custodially capture any disc -- the placed
    // disc itself (self-capture) or an adjacent enemy it would flank.
    const int me = current_;
    const int opp = 1 - me;
    std::vector<Cell> b = board_;
    b[static_cast<std::size_t>(square)] = freeCell(me);

    // (a) Self-capture: the placed disc flanked by enemy Free discs.
    if (pinnedOn(b, square, opp)) {
        return false;
    }
    // (b) Capturing an enemy: any adjacent enemy Free disc the placement would pin.
    const int dr[4] = {-1, 1, 0, 0};
    const int dc[4] = {0, 0, -1, 1};
    const int r = square / columns_;
    const int c = square % columns_;
    for (int k = 0; k < 4; ++k) {
        const int nr = r + dr[k];
        const int nc = c + dc[k];
        if (!inBounds(nr, nc)) {
            continue;
        }
        const int n = idx(nr, nc);
        if (b[static_cast<std::size_t>(n)] == freeCell(opp) && pinnedOn(b, n, me)) {
            return false;
        }
    }
    return true;
}

std::vector<AbsGame::MoveId> Game::enumerateLegalMoves() const {
    std::vector<AbsGame::MoveId> moves;
    if (phase_ == Phase::Placement) {
        for (int i = 0; i < squares_; ++i) {
            const int s = scanOrder_[static_cast<std::size_t>(i)];
            if (isLegalPlacement(s)) {
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
        for (int i = 0; i < squares_; ++i) {
            const int s = scanOrder_[static_cast<std::size_t>(i)];
            if (board_[static_cast<std::size_t>(s)] == boundCell(opp)) {
                removals.push_back(s);
            }
        }
    } else {
        removals.push_back(-1);  // no removal
    }

    // One scratch board for the whole enumeration: moveIsLegalOn overwrites it per
    // candidate, and reusing it keeps the allocation instead of making one per move.
    std::vector<Cell> scratch;
    for (int rem : removals) {
        std::vector<Cell> b = board_;
        std::uint64_t baseHash = hash_;
        if (rem >= 0) {
            setCell(b, rem, Cell::Empty, &baseHash);
        }
        for (int fi = 0; fi < squares_; ++fi) {
            const int from = scanOrder_[static_cast<std::size_t>(fi)];
            if (board_[static_cast<std::size_t>(from)] != freeCell(me)) {
                continue;
            }
            const std::vector<bool> reach = reachableMask(b, from, me);
            for (int ti = 0; ti < squares_; ++ti) {
                const int to = scanOrder_[static_cast<std::size_t>(ti)];
                if (reach[static_cast<std::size_t>(to)] &&
                    moveIsLegalOn(b, from, to, me, baseHash, scratch)) {
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
        return isLegalPlacement(mv);
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

// Applies in BOTH phases. It used to return early during placement, which left a player
// with no legal placement in a position that was not terminal and returned an empty move
// list -- and the self-play driver then reported a draw that no rule produces. A player
// who cannot place is stuck in exactly the sense the immobilisation rule describes, so
// the same rule decides it: the side to move loses.
//
// That position is reachable: every empty square can be one that would complete a
// custodial capture, which placement forbids. It is rare, and it was invisible before
// only because the engine invented a draw instead of reporting it.
void Game::checkImmobilizationTerminal() {
    if (gameOver_) {
        return;
    }
    if (enumerateLegalMoves().empty()) {
        gameOver_ = true;
        winner_ = 1 - current_;  // the player to move has no legal move and loses
        winReason_ = WinReason::Immobilization;
    }
}

bool Game::applyMove(AbsGame::MoveId mv) {
    if (gameOver_) {
        return false;
    }

    if (phase_ == Phase::Placement) {
        if (!isLegalPlacement(mv)) {
            return false;
        }
        setCell(board_, mv, freeCell(current_), &hash_);
        ++placed_[current_];
        seenPositions_.insert(hash_);
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

    // Capture detection for the Pacific counter: a custodial capture binds an enemy
    // Free disc, so the opponent's Free-disc count drops. (Removal only touches Bound
    // discs, so it is tracked separately via removeSquare below.)
    const int opp = 1 - current_;
    const int oppFreeBefore = freeDiscs(opp);

    applyRemoveMoveCapturesTo(board_, removeSquare, from, to, current_, &hash_);
    seenPositions_.insert(hash_);
    recordMove(from, to, removeSquare, path);

    // Pacific (quiet-game) counter: a turn that captures (opponent's Free count drops)
    // or removes a captive (removeSquare >= 0) is "active" and resets the counter; a
    // purely positional turn advances it.
    const bool captured = freeDiscs(opp) < oppFreeBefore;
    const bool removed = removeSquare >= 0;
    if (captured || removed) {
        pacificPlies_ = 0;
    } else {
        ++pacificPlies_;
    }

    // Win by reduction: the opponent (whose captive was just removable) is down
    // to a single disc.
    if (totalDiscs(opp) <= 1) {
        gameOver_ = true;
        winner_ = current_;
        winReason_ = WinReason::Reduction;
    }

    current_ = 1 - current_;
    checkImmobilizationTerminal();  // win by immobilisation, checked at turn start

    // Quiet-game ("Pacific") termination: pacificMoveLimit consecutive plies with no
    // capture or removal. Reduction/immobilisation take precedence (guarded by
    // !gameOver_). Decided on Free-disc material plus komi for player 1, so an even
    // Free count goes to the second player and there is never a draw.
    if (!gameOver_ && pacificPlies_ >= pacificMoveLimit) {
        const double freeP0 = freeDiscs(0);
        const double freeP1 = freeDiscs(1) + komi_;
        gameOver_ = true;
        winner_ = (freeP1 > freeP0) ? 1 : 0;
        winReason_ = WinReason::QuietGame;
    }
    return true;
}

// ── Evaluation ───────────────────────────────────────────────────────────────

// See the header: one definition of "material count", shared by the terminal score and
// the leaf evaluation. Free discs count fully, Bound (immobilised) discs at
// immobilizationDiscount, and player 1 (the second player) gets komi.
double Game::effectiveMaterial(int player) const {
    double material = freeDiscs(player) + immobilizationDiscount * boundDiscs(player);
    if (player == 1) {
        material += komi_;
    }
    return material;
}

double Game::winnerScore() const {
    if (winner_ < 0) {
        throw std::logic_error("Latrunculi: winnerScore() called with no winner");
    }
    const double mine = effectiveMaterial(winner_);
    const double theirs = effectiveMaterial(1 - winner_);
    switch (payoffStyle_) {
        case PayoffStyle::Gradient:
            return materialScore(mine, theirs);
        case PayoffStyle::ConvexMargin:
            return marginScore(mine, theirs);
    }
    throw std::logic_error("Latrunculi: unknown PayoffStyle");
}

double Game::staticEval() const {
    const int me = current_;
    const int opp = 1 - me;

    // A decisive result (reduction, immobilisation, or quiet-game termination) dominates
    // the leaf range. Komi rides along inside effectiveMaterial (winnerScore). Draws no
    // longer exist, so a terminal position always names a winner; winnerScore() surfaces
    // the impossible state rather than papering over it with a fallback score.
    if (gameOver_) {
        const double terminal = kWinBase + kWinBase * winnerScore();
        return (winner_ == me) ? terminal : -terminal;
    }

    // Leaf evaluation in "disc units" (1.0 == one Free disc), from the mover's
    // perspective: material plus positional terms, each taken as the difference between
    // the two sides. See Eval.h for what the positional terms are and why the previous
    // mobility-times-material score had to go -- it rewarded dispersal and so paid both
    // sides to avoid the contact a custodial capture needs.
    //
    // Every term is a me-minus-opponent difference, so the score is antisymmetric. That
    // matters: negamax negates the child score, and a term both sides valued positively
    // (raw contact, say) would make the search incoherent rather than merely aggressive.
    //
    // Both phases use the same formula. In placement the material difference is uniform
    // across every move available at a given ply -- it tracks only who has placed more --
    // so it shifts the score without affecting the choice between siblings, while the
    // positional terms do the discriminating. Realistic magnitudes stay well under 50,
    // far inside the kWinBase (1000+) band a decisive terminal occupies, so positional
    // compensation can never outweigh a real win.
    const double material = effectiveMaterial(me) - effectiveMaterial(opp);
    const double positional =
        positionalScore(positionalTerms(board_, rows_, columns_, me, moveStyle_)) -
        positionalScore(positionalTerms(board_, rows_, columns_, opp, moveStyle_));
    return material + positional;
}

std::unique_ptr<AbsGame::Game> Game::clone() const {
    return std::make_unique<Game>(*this);
}

// Epsilon-greedy heavy playout (see kRolloutEpsilon comment + references above).
AbsGame::MoveId Game::chooseRolloutMove(const std::vector<AbsGame::MoveId>& legal,
                                        std::mt19937_64& rng) const {
    std::uniform_int_distribution<std::size_t> pick(0, legal.size() - 1);

    // Placement moves never capture (the rules forbid it); and with probability
    // kRolloutEpsilon we explore. In both cases fall back to a uniform pick.
    std::uniform_real_distribution<double> coin(0.0, 1.0);
    if (phase_ != Phase::Movement || coin(rng) < kRolloutEpsilon) {
        return legal[pick(rng)];
    }

    // Greedy branch: scan a bounded random sample for an aggressive move. Take an
    // immediate reduction win (decisive) at once; otherwise keep the first move that
    // lowers the opponent's effective material (an immobilisation or a removal).
    const int me = current_;
    const int opp = 1 - me;
    const double oppEffBefore =
        freeDiscs(opp) + immobilizationDiscount * boundDiscs(opp);
    AbsGame::MoveId capturing = -1;
    const std::size_t draws = std::min<std::size_t>(legal.size(), kRolloutSampleCap);
    for (std::size_t i = 0; i < draws; ++i) {
        const AbsGame::MoveId mv = legal[pick(rng)];
        int rem = -1, from = -1, to = -1;
        decodeMovement(mv, rem, from, to);
        std::vector<Cell> b = board_;
        applyRemoveMoveCapturesTo(b, rem, from, to, me);
        int oppFree = 0, oppBound = 0;
        for (Cell c : b) {
            if (c == freeCell(opp)) {
                ++oppFree;
            } else if (c == boundCell(opp)) {
                ++oppBound;
            }
        }
        if (oppFree + oppBound <= 1) {
            return mv;  // decisive: opponent reduced to a single disc
        }
        if (capturing < 0 &&
            oppFree + immobilizationDiscount * oppBound < oppEffBefore - 1e-9) {
            capturing = mv;
        }
    }
    return (capturing >= 0) ? capturing : legal[pick(rng)];
}

// Capture-first move ordering for negamax (see the kOrder* weights above). Scores the
// material the move takes from the opponent: resolve the move on a scratch board and
// compare the opponent's disc counts before and after.
int Game::moveOrderScore(AbsGame::MoveId mv) const {
    // Placement can never capture, so there is nothing to order on; every placement
    // scores 0 and the searcher's stable sort leaves enumeration order untouched.
    if (phase_ != Phase::Movement) {
        return 0;
    }

    const int me = current_;
    const int opp = 1 - me;

    int freeBefore = 0;
    int boundBefore = 0;
    for (Cell c : board_) {
        if (c == freeCell(opp)) {
            ++freeBefore;
        } else if (c == boundCell(opp)) {
            ++boundBefore;
        }
    }

    int rem = -1;
    int from = -1;
    int to = -1;
    decodeMovement(mv, rem, from, to);
    std::vector<Cell> b = board_;
    applyRemoveMoveCapturesTo(b, rem, from, to, me);

    int freeAfter = 0;
    int boundAfter = 0;
    for (Cell c : b) {
        if (c == freeCell(opp)) {
            ++freeAfter;
        } else if (c == boundCell(opp)) {
            ++boundAfter;
        }
    }

    if (freeAfter + boundAfter <= 1) {
        return kOrderDecisive;  // win by reduction: search this first, always
    }

    // Only a Bound disc can be removed, so a drop in the opponent's total is a removal.
    // A drop in their Free count beyond that is a disc newly bound by this move. Freeing
    // one of their captives (by pinning a flanker of mine) makes newlyBound negative, so
    // the move sorts behind the quiet ones -- which is correct, it hands material back.
    const int removed = (freeBefore + boundBefore) - (freeAfter + boundAfter);
    const int newlyBound = freeBefore - freeAfter;
    return removed * kOrderRemoval + newlyBound * kOrderBind;
}

}  // namespace Latrunculi
// Copyright Ben Paul Wise. All Rights Reserved.

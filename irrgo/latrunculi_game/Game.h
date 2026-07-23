// Copyright Ben Paul Wise. All Rights Reserved.
#pragma once

#include "AbsGame.h"

#include <cstdint>
#include <memory>
#include <unordered_set>
#include <vector>

// Latrunculi ("Ludus Latrunculorum"): two-phase play (placement then movement),
// movement geometry selected by MoveStyle (a rook-like slide, or the older step /
// own-color multi-leap), custodial
// capture -> immobilisation, mandatory remove-one-captive-then-move, and the two
// win conditions with the gradient score s = 3M / (3M + 2N), and super-ko (no
// board position may repeat). The second player gets a half-point komi in every
// disc-count score, and a quiet-game ("Pacific") rule ends a movement phase that
// runs pacificMoveLimit plies with no capture or removal. Freeing chains are a
// later stage (see doc/latrunculi-implementation-plan.md).
namespace Latrunculi {

// A board cell. "Bound" == immobilised (flipped to show the X): it cannot move
// and cannot help capture. A player owns both their Free and Bound discs.
enum class Cell : std::uint8_t { Empty, P0Free, P0Bound, P1Free, P1Bound };

enum class Phase { Placement, Movement };

// Which movement rule the game plays under. Both are Dux-free custodial-capture rule
// sets and differ only in the geometry of one move:
//
//   StepLeap — the Locus Ludi *Seneca* reading built first: one orthogonal step onto an
//              empty square, or a chain of leaps over one's own discs (plan Stage 2).
//   Slide    — *Kharebga* movement: rook-like, any distance along a rank or file,
//              stopping before the first disc of either colour. A step is the distance-1
//              case and a leap has no analogue, so this REPLACES StepLeap rather than
//              extending it.
//
// Why Slide exists: the Digital Ludeme Project searched 1006 traditional games for the
// rules actually attested for latrunculi and ran alpha-beta self-play over every intact
// Roman board size; the Kharebga ruleset won on duration, completion and branching
// factor. Their diagnosis of step movement matches the behaviour observed here — an
// engine "may have difficulty detecting a move that brings it closer to an opposing
// piece in order to make a capture if they are distant from one another". Under a slide,
// a threat can be created from across the board in a single ply, which is exactly what a
// shallow search can see.
//
// Captures are deliberately NOT changed: both styles bind custodially (Free -> Bound)
// and keep the mandatory remove-then-move rule, although the DLP's Kharebga also removes
// captures immediately. Adopting only the movement half means a run under Slide differs
// from a run under StepLeap in exactly one rule, so any change in the games is
// attributable. See doc/2026-07-21-latrunculi-dynamism-analysis.md, recommendation 3.
enum class MoveStyle : std::uint8_t { StepLeap, Slide };

// The rule set a game uses when its constructor is not told otherwise.
inline constexpr MoveStyle kDefaultMoveStyle = MoveStyle::Slide;

// How decisively the winner won, as a number in [0, 1] scaling the terminal score.
//
//   Gradient     — the original s = 3M/(3M+2N) for a winner holding M against N. CONCAVE
//                  in the lead and floored at 0.6 when M == N, so a bare win already
//                  collects most of what a rout collects.
//   ConvexMargin — s = ((M-N)/(M+N))^kMarginExponent. Convex: the marginal value of
//                  taking one more disc RISES with the lead instead of falling.
//
// Why this is selectable rather than simply replaced: the Gradient payoff made stalling
// the leader's optimal policy, provably — a quiet-game win at 10 free against 9 scored
// 0.612 against 0.950 for annihilation, a ratio of 1.55 for a win that is close to
// certain once one disc ahead. Self-play bore that out: 20 of 20 step/leap games ended on
// the quiet-game limit (doc/bench/, 2026-07-22). Keeping both means the change can be
// measured against what it replaced instead of asserted. Analysis recommendation 5(c).
enum class PayoffStyle : std::uint8_t { Gradient, ConvexMargin };

inline constexpr PayoffStyle kDefaultPayoffStyle = PayoffStyle::ConvexMargin;

// Convexity of ConvexMargin. 1.0 is linear in the disc margin; above 1 the reward for
// pressing an advantage grows faster than the advantage does. At 2.0 the two games above
// score 0.003 and 0.766 — a ratio of 291 rather than 1.55.
inline constexpr double kMarginExponent = 2.0;

// How a finished game ended (None while the game is still in progress). Reduction =
// the loser was cut to a single disc; Immobilization = the side to move had no legal
// move; QuietGame = the Pacific no-capture limit was reached and the winner was
// decided on material.
enum class WinReason { None, Reduction, Immobilization, QuietGame };

// Weight of an immobilised (Bound) disc relative to a Free disc in the search's
// material evaluation: a captured-but-not-yet-removed disc counts as this fraction
// of a full piece, so immobilising an opponent is rewarded as partial progress.
inline constexpr double immobilizationDiscount = 0.375;

// Komi credited to player 1 (the second player) in every disc-count evaluation,
// compensating for the first-move disadvantage. It must not be a whole number: disc
// counts are integers, so an integral komi would let the two sides tie exactly, and this
// engine has no draw to report the tie with.
//
// 1.5 rather than the original 0.5 as of 2026-07-22, from measurement rather than taste:
// across four 50-game batches the quiet-game tiebreak at 0.5 favoured player 0, and 1.5
// is the value that brings it closest to even in every rule set tested (see
// doc/bench/README.md). Note that this corrects only the quiet-game tiebreak. The larger
// first-player advantage lives in games decided by reduction, which komi cannot reach at
// all, and raising komi far enough to mask that would unbalance the half of the game komi
// actually governs.
inline constexpr double kDefaultKomi = 1.5;

// Throws std::invalid_argument unless `komi` is positive and not a whole number. The Game
// constructors call this; it is public so a caller parsing user input can reject a bad
// value at the point of entry rather than letting it surface from inside a constructor
// later. One definition of the rule, used by both.
void validateKomi(double komi);

// Quiet-game ("Pacific") termination: if the movement phase runs this many consecutive
// plies with no capture and no captive removal, the game ends and is decided on
// free-disc material plus komi.
inline constexpr int pacificMoveLimit = 40;

// Default board for the no-argument constructor.
inline constexpr int kDefaultRows = 8;
inline constexpr int kDefaultColumns = 10;
inline constexpr int kDefaultPerSide = 20;

// Search scoring. A decisive terminal is scaled by kWinBase so a real win/loss
// dominates the heuristic (non-terminal) leaf range: the terminal score is
// kWinBase * (1 + s), where s in [0, 1] is the decisiveness given by PayoffStyle. Any
// win therefore beats any loss whatever s is, which is what keeps a low-margin win from
// ever being preferred to a high-margin one of the opposite sign.
inline constexpr double kWinBase = 1000.0;
// Weights of the Gradient payoff only: s = (kMaterialSelfWeight*M) /
// (kMaterialSelfWeight*M + kMaterialOppWeight*N). Unused by ConvexMargin.
inline constexpr double kMaterialSelfWeight = 3.0;
inline constexpr double kMaterialOppWeight = 2.0;

// A logged ply (for the move log). During placement from == -1; removed == -1
// when no captive was removed that turn.
struct Move {
    int turn = 0;      // 1-based ply index over the whole game
    int player = 0;    // 0 or 1
    int from = -1;     // origin square in movement; -1 in placement
    int to = -1;       // placement square, or movement destination
    int removed = -1;  // enemy captive removed this turn, or -1
    // Movement path {from, landings..., to} (a slide is {from, to}, a multi-leap
    // lists each landing). Empty for placement.
    std::vector<int> path;
};

class Game : public AbsGame::Game {
public:
    // Empty board; both sides place `perSide` discs (placement phase) before
    // movement begins. Requires rows,columns >= 1 and 2*perSide <= rows*columns.
    Game(int rows = kDefaultRows, int columns = kDefaultColumns, int perSide = kDefaultPerSide,
         MoveStyle style = kDefaultMoveStyle,
         PayoffStyle payoff = kDefaultPayoffStyle,
         double komi = kDefaultKomi);
    Game(const Game&) = default;

    // Reconstruct an arbitrary mid-game position (used by the GUI's Load). `board`
    // must hold rows*columns cells. Terminal status is recomputed from the
    // position; `seen` (the super-ko history) may be empty. The Pacific (no-capture)
    // ply counter is not restored -- it resets to 0 on Load. `style` must be the rule
    // set the position was played under: a position is only reachable, and its legal
    // moves are only correct, under the movement rule that produced it.
    Game(int rows, int columns, int perSide, std::vector<Cell> board,
         Phase phase, int current, int placed0, int placed1,
         std::vector<Move> history = {},
         std::unordered_set<std::uint64_t> seen = {},
         MoveStyle style = kDefaultMoveStyle,
         PayoffStyle payoff = kDefaultPayoffStyle,
         double komi = kDefaultKomi);

    // ── AbsGame::Game overrides ──────────────────────────────────────────────
    int currentPlayer() const override { return current_; }
    std::vector<AbsGame::MoveId> getLegalMoves() const override;
    bool isLegalMove(AbsGame::MoveId mv) const override;
    bool applyMove(AbsGame::MoveId mv) override;
    bool isTerminal() const override { return gameOver_; }
    double staticEval() const override;
    std::unique_ptr<AbsGame::Game> clone() const override;
    // Informed ("heavy") MCTS playout policy: epsilon-greedy bias toward aggressive
    // moves so rollouts capture, terminate, and carry a meaningful eval. See the
    // rationale and literature references in Game.cpp.
    AbsGame::MoveId chooseRolloutMove(const std::vector<AbsGame::MoveId>& legal,
                                      std::mt19937_64& rng) const override;
    // Capture-first move ordering for the alpha-beta searcher: removals rank above
    // custodial captures, both above quiet moves, and a reduction win above everything.
    // Alpha-beta prunes far more when the strongest move is searched first. See Game.cpp.
    int moveOrderScore(AbsGame::MoveId mv) const override;

    // ── Accessors for the GUI / renderer / move log ──────────────────────────
    int rows() const { return rows_; }
    int columns() const { return columns_; }
    int squareCount() const { return squares_; }
    Phase phase() const { return phase_; }
    MoveStyle moveStyle() const { return moveStyle_; }
    PayoffStyle payoffStyle() const { return payoffStyle_; }
    double komi() const { return komi_; }
    Cell cellAt(int square) const { return board_[static_cast<std::size_t>(square)]; }
    int ownerAt(int square) const;  // -1 if empty, else the owning player (0 or 1)
    int perSide() const { return perSide_; }
    bool isOver() const { return gameOver_; }
    int winner() const { return winner_; }  // valid only when isOver()
    WinReason winReason() const { return winReason_; }  // how it ended; valid when isOver()
    // The winner's decisiveness s in [0, 1], per this game's PayoffStyle (the loser
    // scores -s). Valid only when isOver() with a winner; throws otherwise.
    double winnerScore() const;
    int totalDiscs(int player) const;
    int freeDiscs(int player) const;
    int boundDiscs(int player) const;
    const std::vector<Move>& history() const { return moveHistory_; }

    // ── Move encoding (so the GUI can build a MoveId from board clicks) ───────
    // Placement: the MoveId is just the target square.
    AbsGame::MoveId placementMove(int square) const { return square; }
    // Movement: packs an optional captive removal with the from->to move.
    AbsGame::MoveId movementMove(int from, int to, int removeSquare = -1) const;
    void decodeMovement(AbsGame::MoveId mv, int& removeSquare, int& from, int& to) const;

private:
    int rows_;
    int columns_;
    int perSide_;
    int squares_;
    MoveStyle moveStyle_ = kDefaultMoveStyle;
    PayoffStyle payoffStyle_ = kDefaultPayoffStyle;
    double komi_ = kDefaultKomi;
    std::vector<Cell> board_;
    int current_ = 0;
    Phase phase_ = Phase::Placement;
    int placed_[2] = {0, 0};
    int pacificPlies_ = 0;  // consecutive movement plies with no capture or removal
    bool gameOver_ = false;
    int winner_ = -1;
    WinReason winReason_ = WinReason::None;
    std::vector<Move> moveHistory_;
    // Super-ko: hashes of every end-of-turn board position seen this game. A move
    // that would recreate one is illegal. The board only (not the side to move) is
    // hashed, per the rule "it does not matter whose turn it is".
    std::unordered_set<std::uint64_t> seenPositions_;
    // Zobrist hash of board_, maintained incrementally by every mutation. Seeded from
    // hashBoard() in the constructors, and copied by the compiler-generated copy
    // constructor that clone() uses, so a search line inherits a correct hash.
    std::uint64_t hash_ = 0;
    // A shuffled permutation of 0..squares-1, fixed once per game (copied by
    // clone()). Move generation scans squares in this order so the engine does not
    // favour the top-left when several moves are equally good.
    std::vector<int> scanOrder_;

    int idx(int row, int column) const { return row * columns_ + column; }
    bool inBounds(int row, int column) const {
        return row >= 0 && row < rows_ && column >= 0 && column < columns_;
    }

    int enemyCaptiveCount(int me) const;
    bool freeAt(const std::vector<Cell>& b, int row, int column, int player) const;
    // True if the disc at `pos` is custodially pinned by `byPlayer`'s Free discs
    // (flanked left/right or top/bottom, or corner-trapped). Mirrors the renderer's
    // is_immobilized rule: an already-Bound flanker does not pin.
    bool pinnedOn(const std::vector<Cell>& b, int pos, int byPlayer) const;
    // Writes cell `c` to `square` of `b`. When `hash` is non-null it is updated in place
    // by XORing out the old cell's Zobrist key and XORing in the new one, which is what
    // makes a candidate move's position hash O(1) instead of O(squares). Callers that do
    // not need a hash pass nullptr.
    void setCell(std::vector<Cell>& b, int square, Cell c, std::uint64_t* hash) const;
    // Moves the disc from->to on board b and resolves the captures it triggers (no
    // removal). applyRemoveMoveCapturesTo prepends an optional captive removal. Both
    // thread `hash` through setCell; pass nullptr when the resulting hash is not wanted.
    void moveAndCapture(std::vector<Cell>& b, int from, int to, int me,
                        std::uint64_t* hash = nullptr) const;
    void applyRemoveMoveCapturesTo(std::vector<Cell>& b, int removeSquare,
                                   int from, int to, int me,
                                   std::uint64_t* hash = nullptr) const;
    // Empty squares reachable from `from` on board b, under whichever MoveStyle this
    // game plays: for StepLeap, orthogonal single steps plus own-color multi-leaps; for
    // Slide, every empty square along the four rays out of `from`. The two are
    // alternatives, never a union -- see MoveStyle.
    std::vector<bool> reachableMask(const std::vector<Cell>& b, int from, int me) const;
    void collectLeaps(const std::vector<Cell>& b, int pos, std::vector<bool>& leapt,
                      std::vector<bool>& reach, int me) const;
    // Rook rays out of `from`: walk each orthogonal direction while the squares are
    // empty and stop at the first occupied one (of either colour) or the board edge.
    void collectSlides(const std::vector<Cell>& b, int from, std::vector<bool>& reach) const;
    // Zobrist hash of the board arrangement (occupancy + bound flags), for super-ko.
    // Full recomputation, O(squares): used to seed hash_ in the constructors. Everywhere
    // else the hash is maintained incrementally through setCell.
    std::uint64_t hashBoard(const std::vector<Cell>& b) const;
    // True if moving from->to on the post-removal board `b` is legal: it neither
    // self-captures (the moved disc pinned by surviving enemy Free discs) nor
    // recreates a previously-seen position (super-ko). `baseHash` must be the Zobrist
    // hash of `b`, so the resulting position's hash follows from the move alone.
    // `scratch` is a caller-owned working board; it is overwritten, and passing the same
    // one across a loop keeps its allocation instead of reallocating per candidate.
    bool moveIsLegalOn(const std::vector<Cell>& b, int from, int to, int me,
                       std::uint64_t baseHash, std::vector<Cell>& scratch) const;
    // A representative path {from, landings..., to} for the move log.
    std::vector<int> movePath(const std::vector<Cell>& b, int from, int to, int me) const;
    bool findLeapPath(const std::vector<Cell>& b, int pos, int to,
                      std::vector<bool>& leapt, int me, std::vector<int>& path) const;
    bool isLegalMovement(int removeSquare, int from, int to) const;
    // A placement is legal iff the square is empty and the placed disc would not be
    // self-captured. No captures occur during placement, so a disc may not be set
    // down where two enemies would flank it (MD rule: no placing between enemies).
    bool isLegalPlacement(int square) const;
    std::vector<AbsGame::MoveId> enumerateLegalMoves() const;
    // One definition of "material count": free discs at full weight, Bound (immobilised)
    // discs at immobilizationDiscount, plus komi for player 1. Used by both the terminal
    // score and the leaf evaluation in staticEval.
    double effectiveMaterial(int player) const;
    void recordMove(int from, int to, int removed, const std::vector<int>& path = {});
    // Ends the game if the side to move has no legal move at all. Applies in BOTH
    // phases: a player with no legal placement is stuck for the same reason a player
    // with no legal movement is, and loses for the same reason.
    void checkImmobilizationTerminal();
    void initScanOrder();  // fill scanOrder_ with a fresh shuffle (per-game)
};

}  // namespace Latrunculi
// Copyright Ben Paul Wise. All Rights Reserved.

// Copyright Ben Paul Wise. All Rights Reserved.
#pragma once

#include "AbsGame.h"

#include <cstdint>
#include <memory>
#include <unordered_set>
#include <vector>

// Latrunculi ("Ludus Latrunculorum"): two-phase play (placement then movement),
// movement = an orthogonal step OR own-colour multi-leaps (Stage 2), custodial
// capture -> immobilisation, mandatory remove-one-captive-then-move, and the two
// win conditions with the gradient score s = 3M / (3M + 2N), and super-ko (no
// board position may repeat). Freeing chains and the draw counter are later
// stages (see doc/latrunculi-implementation-plan.md).
namespace Latrunculi {

// A board cell. "Bound" == immobilised (flipped to show the X): it cannot move
// and cannot help capture. A player owns both their Free and Bound discs.
enum class Cell : std::uint8_t { Empty, P0Free, P0Bound, P1Free, P1Bound };

enum class Phase { Placement, Movement };

// Weight of an immobilised (Bound) disc relative to a Free disc in the search's
// material evaluation: a captured-but-not-yet-removed disc counts as this fraction
// of a full piece, so immobilising an opponent is rewarded as partial progress.
inline constexpr double immobilizationDiscount = 0.375;

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
    Game(int rows = 8, int columns = 8, int perSide = 20);
    Game(const Game&) = default;

    // Reconstruct an arbitrary mid-game position (used by the GUI's Load). `board`
    // must hold rows*columns cells. Terminal status is recomputed from the
    // position; `seen` (the super-ko history) may be empty. The movement-ply / draw
    // counter is not restored (the draw rule is not yet implemented).
    Game(int rows, int columns, int perSide, std::vector<Cell> board,
         Phase phase, int current, int placed0, int placed1,
         std::vector<Move> history = {},
         std::unordered_set<std::uint64_t> seen = {});

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

    // ── Accessors for the GUI / renderer / move log ──────────────────────────
    int rows() const { return rows_; }
    int columns() const { return columns_; }
    int squareCount() const { return squares_; }
    Phase phase() const { return phase_; }
    Cell cellAt(int square) const { return board_[static_cast<std::size_t>(square)]; }
    int ownerAt(int square) const;  // -1 if empty, else the owning player (0 or 1)
    int perSide() const { return perSide_; }
    bool isOver() const { return gameOver_; }
    int winner() const { return winner_; }  // valid only when isOver()
    int totalDiscs(int player) const;
    int freeDiscs(int player) const;
    int boundDiscs(int player) const;
    const std::vector<Move>& history() const { return moveHistory_; }

    // ── Move encoding (so the GUI can build a MoveId from board clicks) ───────
    // Placement: the MoveId is just the target square.
    AbsGame::MoveId placementMove(int square) const { return square; }
    // Movement: packs an optional captive removal with the from->to step.
    AbsGame::MoveId movementMove(int from, int to, int removeSquare = -1) const;
    void decodeMovement(AbsGame::MoveId mv, int& removeSquare, int& from, int& to) const;

private:
    int rows_;
    int columns_;
    int perSide_;
    int squares_;
    std::vector<Cell> board_;
    int current_ = 0;
    Phase phase_ = Phase::Placement;
    int placed_[2] = {0, 0};
    int movementPlies_ = 0;  // movement-only ply counter (Stage 4 draw rule)
    bool gameOver_ = false;
    int winner_ = -1;
    std::vector<Move> moveHistory_;
    // Super-ko: hashes of every end-of-turn board position seen this game. A move
    // that would recreate one is illegal. The board only (not the side to move) is
    // hashed, per the rule "it does not matter whose turn it is".
    std::unordered_set<std::uint64_t> seenPositions_;
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
    // Moves the disc from->to on board b and resolves the captures it triggers (no
    // removal). applyRemoveMoveCapturesTo prepends an optional captive removal.
    void moveAndCapture(std::vector<Cell>& b, int from, int to, int me) const;
    void applyRemoveMoveCapturesTo(std::vector<Cell>& b, int removeSquare,
                                   int from, int to, int me) const;
    // Empty squares reachable from `from` on board b: orthogonal single steps plus
    // own-colour multi-leaps (hop a single own-colour disc to the empty square
    // beyond, chaining over distinct discs in any directions).
    std::vector<bool> reachableMask(const std::vector<Cell>& b, int from, int me) const;
    void collectLeaps(const std::vector<Cell>& b, int pos, std::vector<bool>& leapt,
                      std::vector<bool>& reach, int me) const;
    // FNV-1a hash of the board arrangement (occupancy + bound flags), for super-ko.
    std::uint64_t hashBoard(const std::vector<Cell>& b) const;
    // True if moving from->to on the post-removal board `b` is legal: it neither
    // self-captures (the moved disc pinned by surviving enemy Free discs) nor
    // recreates a previously-seen position (super-ko).
    bool moveIsLegalOn(const std::vector<Cell>& b, int from, int to, int me) const;
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
    void recordMove(int from, int to, int removed, const std::vector<int>& path = {});
    void checkImmobilizationTerminal();
    void initScanOrder();  // fill scanOrder_ with a fresh shuffle (per-game)
};

}  // namespace Latrunculi
// Copyright Ben Paul Wise. All Rights Reserved.

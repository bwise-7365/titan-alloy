// Copyright Ben Paul Wise. All Rights Reserved.
#pragma once

#include "AbsGame.h"

#include <cstdint>
#include <memory>
#include <vector>

// Latrunculi ("Ludus Latrunculorum"), Milestone 1: two-phase play (placement then
// movement), single-step orthogonal moves, custodial capture -> immobilisation,
// mandatory remove-one-captive-then-move, and the two win conditions with the
// gradient score s = 3M / (3M + 2N). Leaping, freeing chains, super-ko and the
// draw counter are later stages (see doc/latrunculi-implementation-plan.md).
namespace Latrunculi {

// A board cell. "Bound" == immobilised (flipped to show the X): it cannot move
// and cannot help capture. A player owns both their Free and Bound discs.
enum class Cell : std::uint8_t { Empty, P0Free, P0Bound, P1Free, P1Bound };

enum class Phase { Placement, Movement };

// A logged ply (for the move log). During placement from == -1; removed == -1
// when no captive was removed that turn.
struct Move {
    int turn = 0;      // 1-based ply index over the whole game
    int player = 0;    // 0 or 1
    int from = -1;     // origin square in movement; -1 in placement
    int to = -1;       // placement square, or movement destination
    int removed = -1;  // enemy captive removed this turn, or -1
};

class Game : public AbsGame::Game {
public:
    // Empty board; both sides place `perSide` discs (placement phase) before
    // movement begins. Requires rows,columns >= 1 and 2*perSide <= rows*columns.
    Game(int rows = 8, int columns = 8, int perSide = 20);
    Game(const Game&) = default;

    // ── AbsGame::Game overrides ──────────────────────────────────────────────
    int currentPlayer() const override { return current_; }
    std::vector<AbsGame::MoveId> getLegalMoves() const override;
    bool isLegalMove(AbsGame::MoveId mv) const override;
    bool applyMove(AbsGame::MoveId mv) override;
    bool isTerminal() const override { return gameOver_; }
    double staticEval() const override;
    std::unique_ptr<AbsGame::Game> clone() const override;

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
    void applyRemoveMoveCapturesTo(std::vector<Cell>& b, int removeSquare,
                                   int from, int to, int me) const;
    bool isLegalMovement(int removeSquare, int from, int to) const;
    std::vector<AbsGame::MoveId> enumerateLegalMoves() const;
    void recordMove(int from, int to, int removed);
    void checkImmobilizationTerminal();
};

}  // namespace Latrunculi
// Copyright Ben Paul Wise. All Rights Reserved.

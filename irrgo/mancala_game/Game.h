// Copyright Ben Paul Wise. All Rights Reserved.
#pragma once
#include "AbsGame.h"
#include <string>
#include <vector>

namespace Mancala {

// Kalah variant of Mancala.  The number of pits per player (N) is a
// constructor parameter (3–12); each game has 2N+2 board slots:
//
//   [0 .. N-1]    = Player 0 (South) pits
//   [N]           = Player 0 store  (p0Store())
//   [N+1 .. 2N]   = Player 1 (North) pits
//   [2N+1]        = Player 1 store  (p1Store())
//
// opposite(i) = 2N - i  (valid for all non-store indices).
//
// Sowing increments the slot index mod totalSlots(), skipping the
// opponent's store on each pass.
//
// Extra-turn rule: if the last sown stone lands in the mover's store,
//   currentPlayer() is unchanged for the next call to applyMove().
//   NegaMax negates scores assuming player alternation, so its evaluation
//   is approximate when extra turns are involved; MCTS handles them exactly.

class Game : public AbsGame::Game {
public:
    explicit Game(int numPits = 6, int stonesPerPit = 4);

    // AbsGame::Game interface
    int                          currentPlayer() const override;
    std::vector<AbsGame::MoveId> getLegalMoves()              const override;
    bool                         isLegalMove(AbsGame::MoveId) const override;
    bool                         applyMove(AbsGame::MoveId)         override;
    bool                         isTerminal()                 const override;
    double                       staticEval()                 const override;
    std::unique_ptr<AbsGame::Game> clone()                    const override;

    // Board layout accessors (all derived from numPits_).
    int numPits()    const { return numPits_; }
    int p0Store()    const { return numPits_; }
    int p1Store()    const { return 2 * numPits_ + 1; }
    int totalSlots() const { return 2 * numPits_ + 2; }

    // GUI helpers
    int  pit(int index)        const { return pits_[index]; }
    int  storeOf(int player)   const { return pits_[player == 0 ? p0Store() : p1Store()]; }
    int  totalStones()         const;
    bool isExtraTurnPending()  const { return extraTurnPending_; }
    std::string moveDescription(AbsGame::MoveId mv) const;

private:
    int              numPits_;
    std::vector<int> pits_;
    int              currentPlayer_;
    bool             gameOver_;
    bool             extraTurnPending_;

    int  opposite(int i)        const { return 2 * numPits_ - i; }
    bool isP0Pit(int i)         const { return i >= 0 && i < numPits_; }
    bool isP1Pit(int i)         const { return i > numPits_ && i <= 2 * numPits_; }
    bool isOwnPit(int i, int p) const { return p == 0 ? isP0Pit(i) : isP1Pit(i); }
    int  myStoreIdx(int p)      const { return p == 0 ? p0Store() : p1Store(); }
    int  oppStoreIdx(int p)     const { return p == 0 ? p1Store() : p0Store(); }

    bool sideEmpty(int player) const;
    void sweepRemaining();
    void checkTerminal();
};

} // namespace Mancala
// Copyright Ben Paul Wise. All Rights Reserved.

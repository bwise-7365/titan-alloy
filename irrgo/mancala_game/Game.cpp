// Copyright Ben Paul Wise. All Rights Reserved.
#include "Game.h"
#include <numeric>

namespace Mancala {

Game::Game(int numPits, int stonesPerPit)
    : numPits_(numPits), currentPlayer_(0), gameOver_(false), extraTurnPending_(false)
{
    pits_.assign(totalSlots(), 0);
    for (int i = 0; i < numPits_; ++i)         pits_[i]             = stonesPerPit;
    for (int i = numPits_+1; i <= 2*numPits_; ++i) pits_[i]         = stonesPerPit;
    // stores start at 0
}

int Game::currentPlayer() const { return currentPlayer_; }

std::vector<AbsGame::MoveId> Game::getLegalMoves() const {
    if (gameOver_) return {};
    std::vector<AbsGame::MoveId> moves;
    int lo = (currentPlayer_ == 0) ? 0          : numPits_ + 1;
    int hi = (currentPlayer_ == 0) ? numPits_    : 2 * numPits_ + 1;
    for (int i = lo; i < hi; ++i)
        if (pits_[i] > 0) moves.push_back(i);
    return moves;
}

bool Game::isLegalMove(AbsGame::MoveId mv) const {
    if (gameOver_ || mv < 0) return false;
    if (!isOwnPit(mv, currentPlayer_)) return false;
    return pits_[mv] > 0;
}

bool Game::applyMove(AbsGame::MoveId mv) {
    if (!isLegalMove(mv)) return false;

    extraTurnPending_ = false;

    // Save pre-move pit contents; only opposite pit matters for capture check.
    auto before = pits_;

    int stones    = pits_[mv];
    pits_[mv]     = 0;
    int cur       = mv;
    int skipStore = oppStoreIdx(currentPlayer_);
    int total     = totalSlots();

    while (stones > 0) {
        cur = (cur + 1) % total;
        if (cur == skipStore) continue;
        ++pits_[cur];
        --stones;
    }

    // Determine aftermath: extra turn, capture, or plain switch.
    int ms = myStoreIdx(currentPlayer_);
    if (cur == ms) {
        extraTurnPending_ = true;
    } else if (isOwnPit(cur, currentPlayer_)
               && pits_[cur] == 1
               && before[opposite(cur)] > 0) {
        // Capture: sweep landing pit + opposite into own store, then switch.
        pits_[ms] += pits_[cur] + pits_[opposite(cur)];
        pits_[cur]           = 0;
        pits_[opposite(cur)] = 0;
        currentPlayer_ = 1 - currentPlayer_;
    } else {
        currentPlayer_ = 1 - currentPlayer_;
    }

    checkTerminal();
    return true;
}

bool Game::isTerminal() const { return gameOver_; }

double Game::staticEval() const {
    double diff = static_cast<double>(pits_[myStoreIdx(currentPlayer_)]
                                    - pits_[oppStoreIdx(currentPlayer_)]);
    if (gameOver_)
        return diff > 0 ? 1000.0 + diff : (diff < 0 ? -1000.0 + diff : 0.0);
    return diff;
}

std::unique_ptr<AbsGame::Game> Game::clone() const {
    return std::make_unique<Game>(*this);
}

int Game::totalStones() const {
    return std::accumulate(pits_.begin(), pits_.end(), 0);
}

std::string Game::moveDescription(AbsGame::MoveId mv) const {
    if (mv < 0 || mv >= totalSlots()) return "?";
    // 1-based pit number within the player's row
    int pitNum = (mv < numPits_) ? mv + 1 : mv - numPits_;
    return "P" + std::to_string(currentPlayer_) + " pit " + std::to_string(pitNum);
}

// ── Private helpers ───────────────────────────────────────────────────────────

bool Game::sideEmpty(int player) const {
    int lo = (player == 0) ? 0          : numPits_ + 1;
    int hi = (player == 0) ? numPits_    : 2 * numPits_ + 1;
    for (int i = lo; i < hi; ++i)
        if (pits_[i] > 0) return false;
    return true;
}

void Game::sweepRemaining() {
    for (int i = 0;          i < numPits_;         ++i) { pits_[p0Store()] += pits_[i]; pits_[i] = 0; }
    for (int i = numPits_+1; i <= 2 * numPits_;    ++i) { pits_[p1Store()] += pits_[i]; pits_[i] = 0; }
}

void Game::checkTerminal() {
    if (sideEmpty(0) || sideEmpty(1)) {
        sweepRemaining();
        gameOver_ = true;
    }
}

} // namespace Mancala
// Copyright Ben Paul Wise. All Rights Reserved.

// Copyright Ben Paul Wise. All Rights Reserved.
#pragma once
#include "AbsGame.h"
#include "Graph.h"
#include "Move.h"
#include <cstdint>
#include <memory>
#include <unordered_set>
#include <utility>
#include <vector>

namespace IrrGo {

enum class Player { Black, White };

struct GameResult {
    double blackScore = 0.0;
    double whiteScore = 0.0;
    Player winner = Player::Black;
};

class Game : public AbsGame::Game {
public:
    // komi is added to White's score; handicap > 1 reduces komi to 0.5
    explicit Game(const Graph& graph, double komi = 1.5, int handicap = 0);
    Game(const Game&) = default;  // copy constructor used by clone()

    // ── IrrGo-specific interface ────────────────────────────────────────────
    bool placeStone(int nodeId);   // returns false if illegal or game over
    bool pass();

    Color colorAt(int nodeId) const { return board_[nodeId]; }
    const Graph& graph() const { return graph_; }
    GameResult score() const;
    std::string asciiBoard() const;
    void setSetupMode(bool on) { setupMode_ = on; }
    const std::vector<Move>& moveHistory() const { return moveHistory_; }

    // isGameOver() kept for backward compatibility; delegates to isTerminal()
    bool isGameOver() const { return isTerminal(); }

    // toMove() returns the Player enum for GUI / IrrGo-specific code
    Player toMove() const { return current_; }

    // ── AbsGame::Game overrides ─────────────────────────────────────────────
    // 0 = Black (first player), 1 = White (second player)
    int currentPlayer() const override { return static_cast<int>(current_); }

    std::vector<AbsGame::MoveId> getLegalMoves() const override;
    bool isLegalMove(AbsGame::MoveId mv) const override;
    bool applyMove(AbsGame::MoveId mv) override;
    bool isTerminal() const override { return passCount_ >= 2; }
    double staticEval() const override;
    double negamaxEval() const override;
    std::unique_ptr<AbsGame::Game> clone() const override;

private:
    const Graph& graph_;
    std::vector<Color> board_;
    Player current_ = Player::Black;
    int passCount_ = 0;
    double komi_;

    uint64_t hash_ = 0;
    std::unordered_set<uint64_t> history_;
    std::vector<uint64_t> zobBlack_, zobWhite_;
    std::vector<Move> moveHistory_;
    bool setupMode_ = false;

    // ── Union-Find for incremental group and liberty tracking ───────────────
    // Only occupied nodes participate meaningfully; dsu_parent_[i]==i is a root.
    // Liberty sets and member lists are stored only at the current root.
    // Union by group size (smaller absorbed into larger) without path
    // compression gives O(log N) find depth — negligible for Go board sizes.
    std::vector<int>                     dsu_parent_;
    std::vector<std::unordered_set<int>> dsu_libset_;   // empty-neighbor IDs, per root
    std::vector<std::vector<int>>        dsu_members_;  // stone IDs, per root

    void initZobrist();
    int  dsuFind(int i) const;
    void dsuUnite(int a, int b);
    void dsuCaptureGroup(int root, Color capturedColor);
    bool isLegalPlacement(int nodeId) const;
    // Raw stone counts {black, white} over the board (komi added by callers).
    std::pair<int, int> countStones() const;
};

} // namespace IrrGo
// Copyright Ben Paul Wise. All Rights Reserved.

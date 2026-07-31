// Copyright Ben Paul Wise. All Rights Reserved.
#pragma once
#include "AbsGame.h"
#include "BensonScratch.h"
#include "Graph.h"
#include "Move.h"
#include <cstdint>
#include <memory>
#include <unordered_set>
#include <utility>
#include <vector>

namespace IrrGo {

enum class Player { Black, White };

// An MCTS playout is allowed this many plies per intersection before the searcher gives
// up on it (see Game::maxPlayoutDepth).
//
// A game needs at least one ply per intersection to fill the board, and IrrGo games run
// well past that: the losing side keeps playing into empty space it cannot hold, and
// solidly filling a region of n points takes on the order of n(n-1)/2 moves. Games of
// more than twice the node count are common, so the ceiling is set at three times to sit
// clear of the tail rather than in it. Raising it costs nothing when playouts terminate
// on their own; setting it too low silently converts every playout into a staticEval()
// guess on an unfinished position.
inline constexpr int kPlayoutDepthPerNode = 3;

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

    // Scaled to the board, because an IrrGo game's length is set by the number of
    // intersections; the base class's fixed ceiling would stop every playout early on any
    // board past a few hundred points. See kPlayoutDepthPerNode.
    int maxPlayoutDepth() const override {
        return kPlayoutDepthPerNode * graph_.nodeCount();
    }

    // Uniform over the legal points that do NOT fill the mover's own eye space, and
    // passing only once no such point is left -- never while a real move is available,
    // since a mid-game pass is not a line of play worth sampling. Which points count as
    // own eye space is the compile-time choice kUseBensonEyeRule in EyeEval.h.
    AbsGame::MoveId chooseRolloutMove(const std::vector<AbsGame::MoveId>& legal,
                                      std::mt19937_64& rng) const override;
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

    // Working storage for the Benson rollout rule (EyeEval.h). Mutable because
    // chooseRolloutMove is const and this is scratch, not state; per-instance rather than
    // shared, so the copy a playout works on -- and therefore each search thread -- has
    // its own and nothing is contended. BensonScratch copies as empty, so clone() does
    // not duplicate the buffers. Unused when kUseBensonEyeRule is false.
    mutable BensonScratch bensonScratch_;

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
    // Append a move (auto-numbered); pass defaults (Empty, -1, -1, -1).
    void recordMove(Color color, int nodeId = -1, int row = -1, int col = -1);
    // Advance the side to move.
    void togglePlayer();
};

} // namespace IrrGo
// Copyright Ben Paul Wise. All Rights Reserved.

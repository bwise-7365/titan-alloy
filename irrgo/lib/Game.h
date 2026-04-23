// Copyright Ben Paul Wise. All Rights Reserved.
#pragma once
#include "Graph.h"
#include "Move.h"
#include <cstdint>
#include <unordered_set>
#include <vector>

enum class Player { Black, White };

struct GameResult {
    double blackScore = 0.0;
    double whiteScore = 0.0;
    Player winner = Player::Black;
};

class Game {
public:
    // komi is added to White's score; handicap > 1 reduces komi to 0.5
    explicit Game(const Graph& graph, double komi = 1.5, int handicap = 0);

    bool placeStone(int nodeId);   // returns false if illegal or game over
    bool pass();
    bool isGameOver() const { return passCount_ >= 2; }

    Player currentPlayer() const { return current_; }
    Color colorAt(int nodeId) const { return board_[nodeId]; }
    const Graph& graph() const { return graph_; }

    GameResult score() const;
    std::string asciiBoard() const;

    void setSetupMode(bool on) { setupMode_ = on; }
    const std::vector<Move>& moveHistory() const { return moveHistory_; }

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

    void initZobrist();
    void getGroup(int nodeId, std::vector<int>& group, std::vector<bool>& visited) const;
    int libertyCount(const std::vector<int>& group) const;
    void removeGroup(const std::vector<int>& group);
    bool isLegalMove(int nodeId) const;
};
// Copyright Ben Paul Wise. All Rights Reserved.

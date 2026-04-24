// Copyright Ben Paul Wise. All Rights Reserved.
#include "Game.h"
#include <limits>
#include <queue>
#include <random>

namespace IrrGo {

// ── Constructors ──────────────────────────────────────────────────────────────

Game::Game(const Graph& graph, double komi, int handicap)
    : graph_(graph),
      board_(graph.nodeCount(), Color::Empty),
      komi_(handicap > 1 ? 0.5 : komi)
{
    initZobrist();
    history_.insert(hash_);
}

// ── Private helpers ───────────────────────────────────────────────────────────

void Game::initZobrist() {
    std::mt19937_64 rng(0xdeadbeefcafe1234ULL);
    int N = graph_.nodeCount();
    zobBlack_.resize(N);
    zobWhite_.resize(N);
    for (int i = 0; i < N; ++i) {
        zobBlack_[i] = rng();
        zobWhite_[i] = rng();
    }
}

void Game::getGroup(int nodeId, std::vector<int>& group, std::vector<bool>& visited) const {
    Color c = board_[nodeId];
    std::queue<int> q;
    q.push(nodeId);
    visited[nodeId] = true;
    while (!q.empty()) {
        int cur = q.front(); q.pop();
        group.push_back(cur);
        for (int nb : graph_.node(cur).neighbors)
            if (!visited[nb] && board_[nb] == c) {
                visited[nb] = true;
                q.push(nb);
            }
    }
}

int Game::libertyCount(const std::vector<int>& group) const {
    int N = graph_.nodeCount();
    std::vector<bool> counted(N, false);
    int libs = 0;
    for (int id : group)
        for (int nb : graph_.node(id).neighbors)
            if (board_[nb] == Color::Empty && !counted[nb]) {
                counted[nb] = true;
                ++libs;
            }
    return libs;
}

void Game::removeGroup(const std::vector<int>& group) {
    for (int id : group) {
        if      (board_[id] == Color::Black) hash_ ^= zobBlack_[id];
        else if (board_[id] == Color::White) hash_ ^= zobWhite_[id];
        board_[id] = Color::Empty;
    }
}

bool Game::isLegalPlacement(int nodeId) const {
    if (board_[nodeId] != Color::Empty) return false;

    Color myColor = (current_ == Player::Black) ? Color::Black : Color::White;
    Color opColor = (current_ == Player::Black) ? Color::White : Color::Black;
    int N = graph_.nodeCount();

    std::vector<Color> tmp = board_;
    tmp[nodeId] = myColor;
    uint64_t tmpHash = hash_;
    tmpHash ^= (myColor == Color::Black) ? zobBlack_[nodeId] : zobWhite_[nodeId];

    auto bfsGroup = [&](int start, Color col) {
        std::vector<int> grp;
        std::vector<bool> vis(N, false);
        std::queue<int> q;
        q.push(start); vis[start] = true;
        while (!q.empty()) {
            int cur = q.front(); q.pop();
            grp.push_back(cur);
            for (int nb : graph_.node(cur).neighbors)
                if (!vis[nb] && tmp[nb] == col) { vis[nb] = true; q.push(nb); }
        }
        return grp;
    };
    auto hasLiberty = [&](const std::vector<int>& grp) {
        for (int id : grp)
            for (int nb : graph_.node(id).neighbors)
                if (tmp[nb] == Color::Empty) return true;
        return false;
    };

    std::vector<bool> visited(N, false);
    for (int nb : graph_.node(nodeId).neighbors) {
        if (tmp[nb] == opColor && !visited[nb]) {
            auto grp = bfsGroup(nb, opColor);
            for (int gi : grp) visited[gi] = true;
            if (!hasLiberty(grp))
                for (int gi : grp) {
                    tmpHash ^= (opColor == Color::Black) ? zobBlack_[gi] : zobWhite_[gi];
                    tmp[gi] = Color::Empty;
                }
        }
    }

    {
        auto grp = bfsGroup(nodeId, myColor);
        if (!hasLiberty(grp))
            for (int gi : grp) {
                tmpHash ^= (myColor == Color::Black) ? zobBlack_[gi] : zobWhite_[gi];
                tmp[gi] = Color::Empty;
            }
    }

    return setupMode_ || !history_.contains(tmpHash);
}

// ── IrrGo-specific moves ──────────────────────────────────────────────────────

bool Game::placeStone(int nodeId) {
    if (isTerminal() || !isLegalPlacement(nodeId)) return false;

    Color myColor = (current_ == Player::Black) ? Color::Black : Color::White;
    Color opColor = (current_ == Player::Black) ? Color::White : Color::Black;
    int N = graph_.nodeCount();

    hash_ ^= (myColor == Color::Black) ? zobBlack_[nodeId] : zobWhite_[nodeId];
    board_[nodeId] = myColor;

    std::vector<bool> visited(N, false);
    for (int nb : graph_.node(nodeId).neighbors) {
        if (board_[nb] == opColor && !visited[nb]) {
            std::vector<int> grp;
            getGroup(nb, grp, visited);
            if (libertyCount(grp) == 0)
                removeGroup(grp);
        }
    }

    {
        std::vector<bool> vis2(N, false);
        std::vector<int> myGrp;
        getGroup(nodeId, myGrp, vis2);
        if (libertyCount(myGrp) == 0)
            removeGroup(myGrp);
    }

    history_.insert(hash_);
    passCount_ = 0;
    moveHistory_.push_back({static_cast<int>(moveHistory_.size()) + 1,
                            myColor,
                            nodeId,
                            graph_.node(nodeId).row, graph_.node(nodeId).col});
    current_ = (current_ == Player::Black) ? Player::White : Player::Black;
    return true;
}

bool Game::pass() {
    if (isTerminal()) return false;
    ++passCount_;
    moveHistory_.push_back({static_cast<int>(moveHistory_.size()) + 1,
                            Color::Empty, -1, -1});
    current_ = (current_ == Player::Black) ? Player::White : Player::Black;
    return true;
}

// ── AbsGame::Game overrides ───────────────────────────────────────────────────

std::vector<AbsGame::MoveId> Game::getLegalMoves() const {
    if (isTerminal()) return {};
    std::vector<AbsGame::MoveId> moves;
    int N = graph_.nodeCount();
    moves.reserve(N + 1);
    for (int i = 0; i < N; ++i)
        if (isLegalPlacement(i)) moves.push_back(i);
    moves.push_back(AbsGame::kPass);
    return moves;
}

bool Game::isLegalMove(AbsGame::MoveId mv) const {
    if (isTerminal()) return false;
    if (mv == AbsGame::kPass) return true;
    if (mv < 0 || mv >= graph_.nodeCount()) return false;
    return isLegalPlacement(mv);
}

bool Game::applyMove(AbsGame::MoveId mv) {
    if (mv == AbsGame::kPass) return pass();
    return placeStone(mv);
}

double Game::staticEval() const {
    auto result = score();
    double myScore = (current_ == Player::Black) ? result.blackScore : result.whiteScore;
    double opScore = (current_ == Player::Black) ? result.whiteScore : result.blackScore;
    return myScore - opScore;
}

std::unique_ptr<AbsGame::Game> Game::clone() const {
    return std::make_unique<Game>(*this);
}

// ── Scoring ───────────────────────────────────────────────────────────────────

GameResult Game::score() const {
    int N = graph_.nodeCount();
    double blackScore = 0.0;
    double whiteScore = komi_;

    for (int i = 0; i < N; ++i) {
        if      (board_[i] == Color::Black) ++blackScore;
        else if (board_[i] == Color::White) ++whiteScore;
    }

    // Voronoi territory: each empty node belongs to whichever color can reach
    // it in fewer hops (multi-source BFS). Ties are neutral.
    constexpr int INF = std::numeric_limits<int>::max();
    auto bfsDist = [&](Color stoneColor) {
        std::vector<int> dist(N, INF);
        std::queue<int> q;
        for (int i = 0; i < N; ++i)
            if (board_[i] == stoneColor) { dist[i] = 0; q.push(i); }
        while (!q.empty()) {
            int cur = q.front(); q.pop();
            for (int nb : graph_.node(cur).neighbors)
                if (dist[nb] > dist[cur] + 1) {
                    dist[nb] = dist[cur] + 1;
                    q.push(nb);
                }
        }
        return dist;
    };

    auto distB = bfsDist(Color::Black);
    auto distW = bfsDist(Color::White);

    for (int i = 0; i < N; ++i) {
        if (board_[i] != Color::Empty) continue;
        if      (distB[i] < distW[i]) ++blackScore;
        else if (distW[i] < distB[i]) ++whiteScore;
    }

    GameResult result;
    result.blackScore = blackScore;
    result.whiteScore = whiteScore;
    result.winner = (blackScore > whiteScore) ? Player::Black : Player::White;
    return result;
}

std::string Game::asciiBoard() const {
    return graph_.asciiRepresentation();
}

} // namespace IrrGo
// Copyright Ben Paul Wise. All Rights Reserved.

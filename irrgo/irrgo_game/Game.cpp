// Copyright Ben Paul Wise. All Rights Reserved.
#include "Game.h"
#include "DVR.h"
#include "EyeEval.h"
#include <limits>
#include <queue>
#include <random>

namespace IrrGo {

// ── Constructor ───────────────────────────────────────────────────────────────

Game::Game(const Graph& graph, double komi, int handicap)
    : graph_(graph),
      board_(graph.nodeCount(), Color::Empty),
      komi_(handicap > 1 ? 0.5 : komi),
      dsu_parent_(graph.nodeCount()),
      dsu_libset_(graph.nodeCount()),
      dsu_members_(graph.nodeCount())
{
    initZobrist();
    history_.insert(hash_);
    int N = graph.nodeCount();
    for (int i = 0; i < N; ++i)
        dsu_parent_[i] = i;
    // dsu_libset_ and dsu_members_ start empty; populated when stones are placed.
}

// ── Zobrist Hashing ────────────────────────────────────────────────────────────

void Game::initZobrist() {
    std::mt19937_64 rng(1116601267); // we want this reproducible
    int N = graph_.nodeCount();
    zobBlack_.resize(N);
    zobWhite_.resize(N);
    for (int i = 0; i < N; ++i) {
        zobBlack_[i] = rng();
        zobWhite_[i] = rng();
    }
}

// ── DSU helpers ───────────────────────────────────────────────────────────────

// No path compression: tree depth is O(log N) under union-by-size.
// For N ≤ 361 (19×19) the depth is at most 9, so traversal cost is negligible.
int Game::dsuFind(int i) const {
    while (dsu_parent_[i] != i) i = dsu_parent_[i];
    return i;
}

// Union by group size: the smaller group is absorbed into the larger.
// Both the member list and the liberty set of the smaller root are merged
// into the larger root, then cleared.
void Game::dsuUnite(int a, int b) {
    int ra = dsuFind(a), rb = dsuFind(b);
    if (ra == rb) return;
    if (dsu_members_[ra].size() < dsu_members_[rb].size()) std::swap(ra, rb);
    // ra is the surviving root; rb is absorbed.
    dsu_parent_[rb] = ra;
    for (int m   : dsu_members_[rb]) dsu_members_[ra].push_back(m);
    for (int lib : dsu_libset_[rb])  dsu_libset_[ra].insert(lib);
    dsu_members_[rb].clear();
    dsu_libset_[rb].clear();
}

// Remove every stone in the group rooted at `root` from the board, update
// hash_, and refresh the liberty sets of all adjacent surviving groups.
// Two-pass design: board cells are cleared first so that the liberty-update
// pass can distinguish captured positions (Empty) from live opponent stones.
void Game::dsuCaptureGroup(int root, Color capturedColor) {
    std::vector<int> members = std::move(dsu_members_[root]);
    dsu_libset_[root].clear();

    // Pass 1: update hash, clear board, reset each stone's DSU entry.
    for (int m : members) {
        hash_ ^= (capturedColor == Color::Black) ? zobBlack_[m] : zobWhite_[m];
        board_[m]       = Color::Empty;
        dsu_parent_[m]  = m;
        dsu_libset_[m].clear();
        dsu_members_[m].clear();
    }

    // Pass 2: each freed cell becomes a liberty for every adjacent living group.
    for (int m : members)
        for (int nb : graph_.node(m).neighbors)
            if (board_[nb] != Color::Empty)
                dsu_libset_[dsuFind(nb)].insert(m);
}

// ── Legality ──────────────────────────────────────────────────────────────────

bool Game::isLegalPlacement(int nodeId) const {
    if (board_[nodeId] != Color::Empty) return false;
    if (setupMode_) return true;

    Color myColor = (current_ == Player::Black) ? Color::Black : Color::White;
    Color opColor = (current_ == Player::Black) ? Color::White : Color::Black;

    // Build the hypothetical Zobrist hash and test for suicide in one neighbour
    // scan.  No board copy or BFS is required.
    uint64_t tmpHash = hash_
        ^ ((myColor == Color::Black) ? zobBlack_[nodeId] : zobWhite_[nodeId]);

    bool willCapture = false;   // placing here captures at least one opponent group
    bool hasLiberty  = false;   // the new stone's group will have at least one liberty

    // xoredRoots guards against double-XORing the same captured group when two
    // neighbours of nodeId belong to the same opponent group (same DSU root).
    std::vector<int> xoredRoots;

    for (int nb : graph_.node(nodeId).neighbors) {
        Color nc = board_[nb];
        if (nc == Color::Empty) {
            hasLiberty = true;
        } else if (nc == opColor) {
            int root = dsuFind(nb);
            const auto& libs = dsu_libset_[root];
            if (libs.size() == 1 && libs.count(nodeId)) {
                // This opponent group's only liberty is nodeId → it would be captured.
                willCapture = true;
                bool seen = false;
                for (int r : xoredRoots) if (r == root) { seen = true; break; }
                if (!seen) {
                    xoredRoots.push_back(root);
                    for (int m : dsu_members_[root])
                        tmpHash ^= (opColor == Color::Black) ? zobBlack_[m] : zobWhite_[m];
                }
            }
        } else {  // nc == myColor
            // A same-colour neighbour contributes a liberty if its group has any
            // empty neighbour other than nodeId.
            int root = dsuFind(nb);
            const auto& libs = dsu_libset_[root];
            if (libs.size() > 1 || (libs.size() == 1 && !libs.count(nodeId)))
                hasLiberty = true;
        }
    }

    // Suicide: the new stone's group would have zero liberties after placement
    // and no captures free any adjacent space.
    if (!willCapture && !hasLiberty) return false;

    return !history_.contains(tmpHash);
}

// ── IrrGo-specific moves ──────────────────────────────────────────────────────

bool Game::placeStone(int nodeId) {
    if (isTerminal() || !isLegalPlacement(nodeId)) return false;

    Color myColor = (current_ == Player::Black) ? Color::Black : Color::White;
    Color opColor = (current_ == Player::Black) ? Color::White : Color::Black;

    hash_ ^= (myColor == Color::Black) ? zobBlack_[nodeId] : zobWhite_[nodeId];
    board_[nodeId] = myColor;

    // Initialise DSU for the new stone.  Its initial liberties are all currently
    // empty neighbours (same-colour neighbours will be united below).
    dsu_parent_[nodeId]  = nodeId;
    dsu_libset_[nodeId].clear();
    dsu_members_[nodeId] = {nodeId};
    for (int nb : graph_.node(nodeId).neighbors)
        if (board_[nb] == Color::Empty) dsu_libset_[nodeId].insert(nb);

    // nodeId is now occupied: remove it from every adjacent group's liberty set.
    for (int nb : graph_.node(nodeId).neighbors)
        if (board_[nb] != Color::Empty)
            dsu_libset_[dsuFind(nb)].erase(nodeId);

    // Capture any opponent group whose liberty set is now empty.
    // After dsuCaptureGroup sets captured cells to Empty, subsequent iterations
    // see board_[nb] == Empty and skip those cells naturally — no deduplication
    // bookkeeping required.
    for (int nb : graph_.node(nodeId).neighbors)
        if (board_[nb] == opColor && dsu_libset_[dsuFind(nb)].empty())
            dsuCaptureGroup(dsuFind(nb), opColor);

    // Merge the new stone into every adjacent same-colour group.
    for (int nb : graph_.node(nodeId).neighbors)
        if (board_[nb] == myColor)
            dsuUnite(nodeId, nb);

    // Handle suicide: if the resulting group still has no liberties, remove it.
    int myRoot = dsuFind(nodeId);
    if (dsu_libset_[myRoot].empty())
        dsuCaptureGroup(myRoot, myColor);

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

std::pair<int, int> Game::countStones() const {
    int black = 0, white = 0;
    for (Color c : board_) {
        if      (c == Color::Black) { ++black; }
        else if (c == Color::White) { ++white; }
    }
    return {black, white};
}

double Game::staticEval() const {
    // Stone count + single-point eye bonus — O(N+E), no allocation.
    // The eye bonus (weight > 1) makes filling a secure eye slightly negative
    // under Chinese area scoring, correcting the raw stone-count bias.
    auto [blackStones, whiteStones] = countStones();
    double black = blackStones;
    double white = komi_ + whiteStones;
    auto [eb, ew] = singleEyeBonus(*this);
    black += eb;
    white += ew;
    return (current_ == Player::Black) ? black - white : white - black;
}

std::unique_ptr<AbsGame::Game> Game::clone() const {
    return std::make_unique<Game>(*this);
}

double Game::negamaxEval() const {
    constexpr int    kRadius     = 3;
    constexpr double areaPremium = 0.05;

    auto [blackStones, whiteStones] = countStones();

    double blackDvr = DVR(*this, Color::Black, kRadius).size();
    double whiteDvr = DVR(*this, Color::White, kRadius).size();

    EyeEval eyes(*this);
    double blackScore = (1.0 + areaPremium) * blackDvr - areaPremium * blackStones + eyes.blackBonus();
    double whiteScore = (1.0 + areaPremium) * whiteDvr - areaPremium * whiteStones + komi_ + eyes.whiteBonus();

    return (current_ == Player::Black) ? blackScore - whiteScore : whiteScore - blackScore;
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

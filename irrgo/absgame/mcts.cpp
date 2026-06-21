// Copyright Ben Paul Wise. All Rights Reserved.
#include "Searcher.h"
#include <chrono>
//#include <cmath>
#include <cstdio>
#include <limits>
#include <random>
#include <vector>

int AbsGame::Searcher::terminalCount = 0;

// ── Internal MCTS machinery ───────────────────────────────────────────────────
namespace {

using AbsGame::MoveId;

 constexpr double kUctExpFactor    = 1.0;
 constexpr int    kMaxRolloutDepth = 200;
 // c^2 in the UCB1 exploration term: sqrt(kUctExplorationC2 * ln(N)/n) equals
 // c * sqrt(ln(N)/n) with the classical c = sqrt(2). kUctExpFactor scales it.
 constexpr double kUctExplorationC2 = 2.0;

struct MctsNode {
    std::unique_ptr<AbsGame::Game>         game;
    MoveId                                 incomingMove;
    MctsNode*                              parent;
    double                                 simReward  = 0.0;
    unsigned                               visitCount = 0;
    std::vector<MoveId>                    moves;
    std::vector<std::unique_ptr<MctsNode>> children;
    bool                                   movesPopulated = false;

    MctsNode(std::unique_ptr<AbsGame::Game> g, MoveId mv, MctsNode* p)
        : game(std::move(g)), incomingMove(mv), parent(p) {}
};

void ensureMoves(MctsNode& node) {
    if (!node.movesPopulated) {
        node.moves = node.game->getLegalMoves();
        node.children.resize(node.moves.size());
        node.movesPopulated = true;
    }
}

// UCT score of child from parent's mover perspective.
// simReward is always accumulated from root player's perspective,
// so we negate it when the parent's mover is the opponent.
double uctScore(const MctsNode& parent, const MctsNode& child,
                double expFactor, int rootPlayer) {
    double mean = child.simReward / child.visitCount;
    if (parent.game->currentPlayer() != rootPlayer)
        mean = -mean;
    double exploration = expFactor *
        std::sqrt(kUctExplorationC2 * std::log(static_cast<double>(parent.visitCount))
                      / child.visitCount);
    return mean + exploration;
}

MctsNode* expand(MctsNode& node, int moveIdx) {
    auto childGame = node.game->clone();
    childGame->applyMove(node.moves[moveIdx]);
    auto child = std::make_unique<MctsNode>(
        std::move(childGame), node.moves[moveIdx], &node);
    MctsNode* ptr = child.get();
    node.children[moveIdx] = std::move(child);
    return ptr;
}

// Select/expand down to a rollout target.
// While any child is unexpanded, pick one at random and expand it.
// Once fully expanded, recurse via UCT selection.
MctsNode* treePolicy(MctsNode& node, double expFactor,
                     int rootPlayer, std::mt19937_64& rng) {
    if (node.game->isTerminal()) return &node;
    ensureMoves(node);
    if (node.moves.empty()) return &node;

    std::vector<int> unexpanded;
    for (int i = 0; i < static_cast<int>(node.children.size()); ++i)
        if (!node.children[i]) unexpanded.push_back(i);

    if (!unexpanded.empty()) {
        std::uniform_int_distribution<int> dist(0, static_cast<int>(unexpanded.size()) - 1);
        return expand(node, unexpanded[dist(rng)]);
    }

    MctsNode* best      = nullptr;
    double    bestScore = -std::numeric_limits<double>::infinity();
    for (const auto& child : node.children) {
        double s = uctScore(node, *child, expFactor, rootPlayer);
        if (s > bestScore) { bestScore = s; best = child.get(); }
    }
    return treePolicy(*best, expFactor, rootPlayer, rng);
}

// Random playout from node's game state; returns reward from root player's view.
double rollout(const MctsNode& node, int rootPlayer, std::mt19937_64& rng) {
    auto game = node.game->clone();
    for (int d = 0; d < kMaxRolloutDepth; ++d) {
        if (game->isTerminal()) {
            ++AbsGame::Searcher::terminalCount;
            break;
        }
        auto moves = game->getLegalMoves();
        if (moves.empty()) break;
        std::uniform_int_distribution<int> dist(0, static_cast<int>(moves.size()) - 1);
        game->applyMove(moves[dist(rng)]);
    }
    double eval = game->staticEval();
    return (game->currentPlayer() == rootPlayer) ? eval : -eval;
}

// Propagate reward from selected node up to root.
void backup(MctsNode* node, double reward) {
    while (node) {
        node->visitCount += 1;
        node->simReward  += reward;
        node = node->parent;
    }
}

MctsNode* robustChild(const MctsNode& node) {
    MctsNode* best  = nullptr;
    unsigned  maxVC = 0;
    for (const auto& child : node.children) {
        if (child && child->visitCount > maxVC) {
            maxVC = child->visitCount;
            best  = child.get();
        }
    }
    return best;
}

MctsNode* bestChildByUct(const MctsNode& node, double expFactor, int rootPlayer) {
    MctsNode* best      = nullptr;
    double    bestScore = -std::numeric_limits<double>::infinity();
    for (const auto& child : node.children) {
        if (!child) continue;
        double s = uctScore(node, *child, expFactor, rootPlayer);
        if (s > bestScore) { bestScore = s; best = child.get(); }
    }
    return best;
}

void growTree(MctsNode& root, double expFactor, int rootPlayer, std::mt19937_64& rng) {
    MctsNode* selected = treePolicy(root, expFactor, rootPlayer, rng);
    double    reward   = rollout(*selected, rootPlayer, rng);
    backup(selected, reward);
}

} // anonymous namespace

// ── Searcher::mcts ────────────────────────────────────────────────────────────
namespace AbsGame {

MoveId Searcher::mcts(const Game& game, int nodeMin, int nodeMax) {
    terminalCount = 0;
    int             rootPlayer = game.currentPlayer();
    std::mt19937_64 rng(std::random_device{}());

    auto root = std::make_unique<MctsNode>(game.clone(), kPass, nullptr);
    ensureMoves(*root);
    if (root->moves.empty()) return kPass;

    // Phase 1: run at least nodeMin iterations
    while (static_cast<int>(root->visitCount) < nodeMin)
        growTree(*root, kUctExpFactor, rootPlayer, rng);

    // Phase 2: continue until robust child and best child agree, or nodeMax reached
    while (static_cast<int>(root->visitCount) < nodeMax) {
        MctsNode* rc = robustChild(*root);
        MctsNode* bc = bestChildByUct(*root, kUctExpFactor, rootPlayer);
        if (rc && bc && rc == bc) break;
        growTree(*root, kUctExpFactor, rootPlayer, rng);
    }

    fprintf(stderr, "terminalCount: %d\n", terminalCount);
    printf("terminalCount: %d\n", terminalCount);

    MctsNode* rc = robustChild(*root);
    return rc ? rc->incomingMove : kPass;
}

MoveId Searcher::mcts(const Game& game, int seconds) {
    terminalCount = 0;
    using Clock = std::chrono::steady_clock;
    const auto deadline = Clock::now() + std::chrono::seconds(seconds);

    int             rootPlayer = game.currentPlayer();
    std::mt19937_64 rng(std::random_device{}());

    auto root = std::make_unique<MctsNode>(game.clone(), kPass, nullptr);
    ensureMoves(*root);
    if (root->moves.empty()) return kPass;

    while (Clock::now() < deadline)
        growTree(*root, kUctExpFactor, rootPlayer, rng);

    fprintf(stderr, "terminalCount: %d\n", terminalCount);
    printf("terminalCount: %d\n", terminalCount);

    MctsNode* rc = robustChild(*root);
    return rc ? rc->incomingMove : kPass;
}

} // namespace AbsGame
// Copyright Ben Paul Wise. All Rights Reserved.

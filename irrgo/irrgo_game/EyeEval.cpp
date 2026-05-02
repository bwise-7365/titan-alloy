// Copyright Ben Paul Wise. All Rights Reserved.
#include "EyeEval.h"
#include <queue>
#include <vector>

namespace IrrGo {

// ── singleEyeBonus ────────────────────────────────────────────────────────────
// O(N + E), no heap allocation. Scans every empty node; if all its graph
// neighbors carry the same stone color, it is a single-point eye for that color.

std::pair<double, double> singleEyeBonus(const Game& game, double eyeWeight) {
    const Graph& g = game.graph();
    int N = g.nodeCount();
    double black = 0.0, white = 0.0;

    for (int i = 0; i < N; ++i) {
        if (game.colorAt(i) != Color::Empty) continue;
        const auto& nbrs = g.node(i).neighbors;
        if (nbrs.empty()) continue;

        bool allBlack = true, allWhite = true;
        for (int nb : nbrs) {
            Color c = game.colorAt(nb);
            if (c != Color::Black) allBlack = false;
            if (c != Color::White) allWhite = false;
            if (!allBlack && !allWhite) break;
        }
        if      (allBlack) black += eyeWeight;
        else if (allWhite) white += eyeWeight;
    }
    return {black, white};
}

// ── EyeEval ──────────────────────────────────────────────────────────────────
// BFS over empty nodes to find connected empty regions. For each region,
// examine every stone neighbor. If all stone neighbors belong to one color,
// the region is enclosed by that color and its size is credited (weighted)
// to that player. Contested regions (neighbors of both colors) receive no
// credit — they are already handled by DVR's Voronoi logic.

EyeEval::EyeEval(const Game& game, double eyeWeight)
    : toMove_(game.toMove())
{
    const Graph& g = game.graph();
    int N = g.nodeCount();
    std::vector<bool> visited(N, false);

    for (int start = 0; start < N; ++start) {
        if (game.colorAt(start) != Color::Empty || visited[start]) continue;

        std::vector<int> region;
        std::queue<int>  q;
        bool bordersBlack = false, bordersWhite = false;

        q.push(start);
        visited[start] = true;
        while (!q.empty()) {
            int cur = q.front(); q.pop();
            region.push_back(cur);
            for (int nb : g.node(cur).neighbors) {
                Color c = game.colorAt(nb);
                if (c == Color::Empty) {
                    if (!visited[nb]) { visited[nb] = true; q.push(nb); }
                } else if (c == Color::Black) {
                    bordersBlack = true;
                } else {
                    bordersWhite = true;
                }
            }
        }

        double credit = static_cast<double>(region.size()) * eyeWeight;
        if      (bordersBlack && !bordersWhite) black_ += credit;
        else if (bordersWhite && !bordersBlack) white_ += credit;
        // Contested or no stone boundary: no credit.
    }
}

double EyeEval::relativeValue() const {
    return (toMove_ == Player::Black) ? black_ - white_ : white_ - black_;
}

} // namespace IrrGo
// Copyright Ben Paul Wise. All Rights Reserved.

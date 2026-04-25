// Copyright Ben Paul Wise. All Rights Reserved.
#include "DVR.h"
#include <limits>
#include <queue>

namespace IrrGo {

DVR::DVR(const Game& game, Color color, int radius) {
    const Graph& g = game.graph();
    int N = g.nodeCount();

    constexpr int INF = std::numeric_limits<int>::max();
    std::vector<int> distMine(N, INF);
    std::vector<int> distOpp(N, INF);

    Color opColor = (color == Color::Black) ? Color::White : Color::Black;

    std::queue<int> q;

    // Multi-source BFS from own stones, expanding only within radius.
    for (int i = 0; i < N; ++i)
        if (game.colorAt(i) == color) { distMine[i] = 0; q.push(i); }
    while (!q.empty()) {
        int cur = q.front(); q.pop();
        if (distMine[cur] >= radius) continue;
        for (int nb : g.node(cur).neighbors)
            if (distMine[nb] == INF) { distMine[nb] = distMine[cur] + 1; q.push(nb); }
    }

    // Multi-source BFS from opponent stones, unlimited — gives true Voronoi distance.
    for (int i = 0; i < N; ++i)
        if (game.colorAt(i) == opColor) { distOpp[i] = 0; q.push(i); }
    while (!q.empty()) {
        int cur = q.front(); q.pop();
        for (int nb : g.node(cur).neighbors)
            if (distOpp[nb] == INF) { distOpp[nb] = distOpp[cur] + 1; q.push(nb); }
    }

    // Collect nodes reachable within radius from own stones that are strictly
    // closer to own stones than to any opponent stone.
    for (int i = 0; i < N; ++i)
        if (distMine[i] <= radius && distMine[i] < distOpp[i])
            nodes_.push_back(i);
}

} // namespace IrrGo
// Copyright Ben Paul Wise. All Rights Reserved.

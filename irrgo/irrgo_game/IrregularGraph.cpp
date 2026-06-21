// Copyright Ben Paul Wise. All Rights Reserved.
#include "IrregularGraph.h"
#include <algorithm>
#include <cmath>
#include <limits>
#include <numbers>
//#include <queue>
#include <random>
#include <sstream>

namespace IrrGo {

static constexpr double kMinAngleDeg   = 15.0;
static constexpr float  kNodeClearance = 0.3f; // min distance from segment to any non-endpoint node

// Orientation of point p relative to directed line a→b (sign of cross-product)
static double orient(float ax, float ay, float bx, float by, float px, float py) {
    return static_cast<double>(bx - ax) * (py - ay)
         - static_cast<double>(by - ay) * (px - ax);
}

int IrregularGraph::expandGrid(int n) {
    const int m = static_cast<int>(round((6.0 * n) / 5.0));
    return m;
}

bool IrregularGraph::segmentsProperlyIntersect(
    float ax, float ay, float bx, float by,
    float cx, float cy, float dx, float dy)
{
    double d1 = orient(cx, cy, dx, dy, ax, ay);
    double d2 = orient(cx, cy, dx, dy, bx, by);
    double d3 = orient(ax, ay, bx, by, cx, cy);
    double d4 = orient(ax, ay, bx, by, dx, dy);
    return ((d1 > 0 && d2 < 0) || (d1 < 0 && d2 > 0)) &&
           ((d3 > 0 && d4 < 0) || (d3 < 0 && d4 > 0));
}

bool IrregularGraph::edgeCrossesNode(int a, int b, int nodeId) const {
    if (nodeId == a || nodeId == b) return false;
    float ax = nodes_[a].x,      ay = nodes_[a].y;
    float bx = nodes_[b].x,      by = nodes_[b].y;
    float px = nodes_[nodeId].x, py = nodes_[nodeId].y;
    float dx = bx - ax, dy = by - ay;
    float len2 = dx*dx + dy*dy;
    if (len2 < 1e-9f) return false;
    float t     = std::clamp(((px-ax)*dx + (py-ay)*dy) / len2, 0.0f, 1.0f);
    float rx    = px - (ax + t*dx);
    float ry    = py - (ay + t*dy);
    return rx*rx + ry*ry < kNodeClearance * kNodeClearance;
}

bool IrregularGraph::edgesCross(int a, int b, int c, int d) const {
    if (a == c || a == d || b == c || b == d) return false;
    return segmentsProperlyIntersect(
        nodes_[a].x, nodes_[a].y, nodes_[b].x, nodes_[b].y,
        nodes_[c].x, nodes_[c].y, nodes_[d].x, nodes_[d].y);
}

double IrregularGraph::angleDeg(int center, int n1, int n2) const {
    double dx1 = nodes_[n1].x - nodes_[center].x;
    double dy1 = nodes_[n1].y - nodes_[center].y;
    double dx2 = nodes_[n2].x - nodes_[center].x;
    double dy2 = nodes_[n2].y - nodes_[center].y;
    double mag = std::sqrt((dx1*dx1 + dy1*dy1) * (dx2*dx2 + dy2*dy2));
    if (mag < 1e-9) return 0.0;
    double cosA = std::clamp((dx1*dx2 + dy1*dy2) / mag, -1.0, 1.0);
    return std::acos(cosA) * (180.0 / std::numbers::pi);
}

bool IrregularGraph::canAddEdge(int a, int b) const {
    if (static_cast<int>(nodes_[a].neighbors.size()) >= maxDegree_) return false;
    if (static_cast<int>(nodes_[b].neighbors.size()) >= maxDegree_) return false;

    for (const auto& [ea, eb] : edges_)
        if (edgesCross(a, b, ea, eb)) return false;

    int N = nodeCount();
    for (int i = 0; i < N; ++i)
        if (edgeCrossesNode(a, b, i)) return false;

    for (int nb : nodes_[a].neighbors)
        if (angleDeg(a, b, nb) < kMinAngleDeg) return false;

    for (int nb : nodes_[b].neighbors)
        if (angleDeg(b, a, nb) < kMinAngleDeg) return false;

    return true;
}

IrregularGraph::IrregularGraph(int m, int n, int maxDegree, uint64_t seed)
    : m_(m), n_(n), maxDegree_(maxDegree),
      baseRows_(expandGrid(m)), baseCols_(expandGrid(n))
{
    seed_ = seed;
    std::mt19937_64 rng(seed);

    int totalBase = baseRows_ * baseCols_;
    int cornerIdx[4] = {
        0,
        baseCols_ - 1,
        (baseRows_ - 1) * baseCols_,
        (baseRows_ - 1) * baseCols_ + baseCols_ - 1
    };

    std::vector<bool> isCorner(totalBase, false);
    for (int ci : cornerIdx) isCorner[ci] = true;

    std::vector<int> candidates;
    candidates.reserve(totalBase - 4);
    for (int i = 0; i < totalBase; ++i)
        if (!isCorner[i]) candidates.push_back(i);

    int needed = m * n - 4;
    std::ranges::shuffle(candidates, rng);
    if (needed > static_cast<int>(candidates.size()))
        needed = static_cast<int>(candidates.size());
    candidates.resize(needed);

    auto makeNode = [&](int gridIdx, int nodeId) {
        int gr = gridIdx / baseCols_;
        int gc = gridIdx % baseCols_;
        nodes_.emplace_back(nodeId, gr, gc, gc * kColSpacing, gr * kRowSpacing);
    };

    int id = 0;
    for (int ci : cornerIdx) makeNode(ci, id++);
    for (int ci : candidates)  makeNode(ci, id++);

    int N = nodeCount();

    struct PotEdge { int a, b; float len; };
    std::vector<PotEdge> potEdges;
    potEdges.reserve(static_cast<size_t>(N) * (N - 1) / 2);
    for (int i = 0; i < N; ++i)
        for (int j = i + 1; j < N; ++j) {
            float dx = nodes_[j].x - nodes_[i].x;
            float dy = nodes_[j].y - nodes_[i].y;
            potEdges.push_back({i, j, std::sqrt(dx*dx + dy*dy)});
        }
    std::ranges::sort(potEdges,
                      [](const PotEdge& a, const PotEdge& b) { return a.len < b.len; });

    // Connect each corner to its nearest horizontal and vertical game-node neighbor
    for (int c = 0; c < 4; ++c) {
        const Node& cn = nodes_[c];
        int hNbr = -1; float hDist = std::numeric_limits<float>::max();
        int vNbr = -1; float vDist = std::numeric_limits<float>::max();
        for (int i = 0; i < N; ++i) {
            if (i == c) continue;
            const Node& nn = nodes_[i];
            if (nn.row == cn.row) {
                float d = std::abs(nn.x - cn.x);
                if (d < hDist) { hDist = d; hNbr = i; }
            }
            if (nn.col == cn.col) {
                float d = std::abs(nn.y - cn.y);
                if (d < vDist) { vDist = d; vNbr = i; }
            }
        }
        if (hNbr >= 0) { addEdge(c, hNbr); edges_.emplace_back(c, hNbr); }
        if (vNbr >= 0 && vNbr != hNbr) { addEdge(c, vNbr); edges_.emplace_back(c, vNbr); }
    }

    // Greedily add remaining edges shortest-first
    for (const auto& pe : potEdges) {
        if (pe.a < 4 && static_cast<int>(nodes_[pe.a].neighbors.size()) >= 2) continue;
        if (pe.b < 4 && static_cast<int>(nodes_[pe.b].neighbors.size()) >= 2) continue;

        bool already = false;
        for (int nb : nodes_[pe.a].neighbors)
            if (nb == pe.b) { already = true; break; }
        if (already) continue;

        if (canAddEdge(pe.a, pe.b)) {
            addEdge(pe.a, pe.b);
            edges_.emplace_back(pe.a, pe.b);
        }
    }
}

std::string IrregularGraph::asciiRepresentation() const {
    int gridH = baseRows_ * 2 - 1;
    int gridW = baseCols_ * 4 - 3;
    std::vector<std::string> grid(gridH, std::string(gridW, ' '));

    auto dispR = [](int r) { return r * 2; };
    auto dispC = [](int c) { return c * 4; };

    for (const auto& nd : nodes_)
        grid[dispR(nd.row)][dispC(nd.col)] = '+';

    int hCount = 0, vCount = 0, diagCount = 0;
    for (const auto& [a, b] : edges_) {
        const Node& na = nodes_[a];
        const Node& nb = nodes_[b];
        if (na.row == nb.row) {
            ++hCount;
            int r  = dispR(na.row);
            int c1 = dispC(std::min(na.col, nb.col));
            int c2 = dispC(std::max(na.col, nb.col));
            for (int c = c1 + 1; c < c2 && c < gridW; ++c)
                grid[r][c] = '-';
        } else if (na.col == nb.col) {
            ++vCount;
            int c  = dispC(na.col);
            int r1 = dispR(std::min(na.row, nb.row));
            int r2 = dispR(std::max(na.row, nb.row));
            for (int r = r1 + 1; r < r2 && r < gridH; ++r)
                grid[r][c] = '|';
        } else {
            ++diagCount;
        }
    }

    std::ostringstream oss;
    for (const auto& row : grid)
        oss << row << '\n';

    int total = static_cast<int>(edges_.size());
    oss << "Edges: " << total << " total  ("
        << hCount << " horizontal, " << vCount << " vertical, "
        << diagCount << " diagonal)\n";

    oss << "Nodes (" << nodeCount() << "):\n";
    for (int i = 0; i < nodeCount(); ++i) {
        const Node& nd = nodes_[i];
        oss << "  [" << i << "] " << nd.label
            << "  deg=" << static_cast<int>(nd.neighbors.size()) << '\n';
    }

    oss << "Edges:\n";
    for (const auto& [a, b] : edges_)
        oss << "  " << nodes_[a].label << " -- " << nodes_[b].label << '\n';

    return oss.str();
}

} // namespace IrrGo
// Copyright Ben Paul Wise. All Rights Reserved.

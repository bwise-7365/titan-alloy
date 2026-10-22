// Copyright Ben Paul Wise. All Rights Reserved.
#include "EyeEval.h"
#include <algorithm>
#include <cstddef>
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

// ── Rollout eye rule ─────────────────────────────────────────────────────────

bool isSinglePointEye(const Game& game, int nodeId, Color color) {
    if (game.colorAt(nodeId) != Color::Empty) {
        return false;
    }
    const std::vector<int>& nbrs = game.graph().node(nodeId).neighbors;
    if (nbrs.empty()) {
        return false;  // an isolated point encloses nothing
    }
    for (int nb : nbrs) {
        if (game.colorAt(nb) != color) {
            return false;
        }
    }
    return true;
}

namespace {

// Refill a scratch buffer to `size` copies of `value` without releasing its storage.
// assign() keeps the existing capacity, so after the first call on a given board this
// costs a memset and no allocation.
template <typename T>
void refill(std::vector<T>& buffer, std::size_t size, T value) {
    buffer.assign(size, value);
}

}  // namespace

const std::vector<char>& bensonPassAliveTerritory(const Game& game, Color color,
                                                 BensonScratch& s) {
    const Graph& g = game.graph();
    const int n = g.nodeCount();
    const auto asIdx = [](int i) { return static_cast<std::size_t>(i); };

    // ── Chains: connected components of `color` stones ───────────────────────
    refill(s.chainOf, asIdx(n), -1);
    int chainCount = 0;
    for (int start = 0; start < n; ++start) {
        if ((game.colorAt(start) != color) || (s.chainOf[asIdx(start)] >= 0)) {
            continue;
        }
        // A vector consumed by read index rather than a std::queue: the deque behind a
        // queue allocates a block per traversal, which is exactly what this call is
        // trying to stop doing.
        s.frontier.clear();
        s.frontier.push_back(start);
        s.chainOf[asIdx(start)] = chainCount;
        for (std::size_t read = 0; read < s.frontier.size(); ++read) {
            const int cur = s.frontier[read];
            for (int nb : g.node(cur).neighbors) {
                if ((game.colorAt(nb) == color) && (s.chainOf[asIdx(nb)] < 0)) {
                    s.chainOf[asIdx(nb)] = chainCount;
                    s.frontier.push_back(nb);
                }
            }
        }
        ++chainCount;
    }

    // ── Regions: connected components of everything NOT `color` ──────────────
    // Each is maximal, so every node adjacent to a region but outside it is `color`:
    // every region is `color`-enclosed by construction.
    refill(s.regionOf, asIdx(n), -1);
    s.regionEmpty.clear();
    s.regionEmptyStart.clear();
    s.regionChain.clear();
    s.regionChainStart.clear();
    int regionCount = 0;
    for (int start = 0; start < n; ++start) {
        if ((game.colorAt(start) == color) || (s.regionOf[asIdx(start)] >= 0)) {
            continue;
        }
        s.regionEmptyStart.push_back(static_cast<int>(s.regionEmpty.size()));
        s.regionChainStart.push_back(static_cast<int>(s.regionChain.size()));
        const std::size_t chainsBegin = s.regionChain.size();

        s.frontier.clear();
        s.frontier.push_back(start);
        s.regionOf[asIdx(start)] = regionCount;
        for (std::size_t read = 0; read < s.frontier.size(); ++read) {
            const int cur = s.frontier[read];
            if (game.colorAt(cur) == Color::Empty) {
                s.regionEmpty.push_back(cur);
            }
            for (int nb : g.node(cur).neighbors) {
                if (game.colorAt(nb) == color) {
                    // Linear dedup: a region borders a handful of chains, so a set would
                    // cost more than the scan.
                    const int id = s.chainOf[asIdx(nb)];
                    if (std::find(s.regionChain.begin() + static_cast<std::ptrdiff_t>(chainsBegin),
                                  s.regionChain.end(), id) == s.regionChain.end()) {
                        s.regionChain.push_back(id);
                    }
                } else if (s.regionOf[asIdx(nb)] < 0) {
                    s.regionOf[asIdx(nb)] = regionCount;
                    s.frontier.push_back(nb);
                }
            }
        }
        ++regionCount;
    }
    // Closing sentinels, so region r is always [start[r], start[r+1]).
    s.regionEmptyStart.push_back(static_cast<int>(s.regionEmpty.size()));
    s.regionChainStart.push_back(static_cast<int>(s.regionChain.size()));

    // ── Vitality, computed once ──────────────────────────────────────────────
    // Region r is vital to chain x when EVERY empty point of r is a liberty of x. A
    // region with no empty points is vital to nothing: it offers the chain no eye.
    // Precomputed rather than recomputed inside the fixed point below, which is what
    // turns that loop from O(chains x regions x region size) into O(vital pairs).
    s.regionVital.clear();
    s.regionVitalStart.clear();
    for (int r = 0; r < regionCount; ++r) {
        s.regionVitalStart.push_back(static_cast<int>(s.regionVital.size()));
        const int emptyBegin = s.regionEmptyStart[asIdx(r)];
        const int emptyEnd   = s.regionEmptyStart[asIdx(r + 1)];
        if (emptyBegin == emptyEnd) {
            continue;
        }
        for (int ci = s.regionChainStart[asIdx(r)]; ci < s.regionChainStart[asIdx(r + 1)]; ++ci) {
            const int chain = s.regionChain[asIdx(ci)];
            bool vital = true;
            for (int ei = emptyBegin; (ei < emptyEnd) && vital; ++ei) {
                bool touches = false;
                for (int nb : g.node(s.regionEmpty[asIdx(ei)]).neighbors) {
                    if (s.chainOf[asIdx(nb)] == chain) {
                        touches = true;
                        break;
                    }
                }
                vital = touches;
            }
            if (vital) {
                s.regionVital.push_back(chain);
            }
        }
    }
    s.regionVitalStart.push_back(static_cast<int>(s.regionVital.size()));

    // ── Fixed point ──────────────────────────────────────────────────────────
    // A chain survives while at least two surviving regions are vital to it; a region
    // survives while every chain on its boundary survives. Iterate until neither set
    // changes: what remains is unconditionally alive.
    refill(s.chainLives, asIdx(chainCount), static_cast<char>(1));
    refill(s.regionLives, asIdx(regionCount), static_cast<char>(1));
    bool changed = true;
    while (changed) {
        changed = false;

        refill(s.vitalCount, asIdx(chainCount), 0);
        for (int r = 0; r < regionCount; ++r) {
            if (!s.regionLives[asIdx(r)]) {
                continue;
            }
            for (int vi = s.regionVitalStart[asIdx(r)]; vi < s.regionVitalStart[asIdx(r + 1)]; ++vi) {
                ++s.vitalCount[asIdx(s.regionVital[asIdx(vi)])];
            }
        }
        for (int x = 0; x < chainCount; ++x) {
            if (s.chainLives[asIdx(x)] && (s.vitalCount[asIdx(x)] < 2)) {
                s.chainLives[asIdx(x)] = 0;
                changed = true;
            }
        }
        for (int r = 0; r < regionCount; ++r) {
            if (!s.regionLives[asIdx(r)]) {
                continue;
            }
            for (int ci = s.regionChainStart[asIdx(r)]; ci < s.regionChainStart[asIdx(r + 1)]; ++ci) {
                if (!s.chainLives[asIdx(s.regionChain[asIdx(ci)])]) {
                    s.regionLives[asIdx(r)] = 0;
                    changed = true;
                    break;
                }
            }
        }
    }

    refill(s.territory, asIdx(n), static_cast<char>(0));
    for (int r = 0; r < regionCount; ++r) {
        if (!s.regionLives[asIdx(r)]) {
            continue;
        }
        for (int ei = s.regionEmptyStart[asIdx(r)]; ei < s.regionEmptyStart[asIdx(r + 1)]; ++ei) {
            s.territory[asIdx(s.regionEmpty[asIdx(ei)])] = 1;
        }
    }
    return s.territory;
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

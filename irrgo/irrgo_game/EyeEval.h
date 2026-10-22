// Copyright Ben Paul Wise. All Rights Reserved.
#pragma once
#include "BensonScratch.h"
#include "Game.h"
#include <utility>
#include <vector>

namespace IrrGo {

// ── Rollout eye rule ─────────────────────────────────────────────────────────
//
// THE SWITCH. false = the cheap single-point test, true = Benson's algorithm.
// Compile-time so exactly one of the two is in the binary and the unused branch costs
// nothing; flip it, rebuild, and compare the two on the same seed.
//
// Both answer the same question for Game::chooseRolloutMove: which points must a playout
// refuse to play because they are the mover's OWN eye space. A playout that fills its own
// eyes kills groups that were alive, so the game it finishes is a real terminal position
// but the wrong one -- and MCTS averages exactly those outcomes. Refusing the move up
// front is the standard fix (Bouzy & Helmstetter 2003; Gelly & Silver, MoGo); scoring it
// afterwards, as singleEyeBonus and EyeEval do for staticEval/negamaxEval below, cannot
// undo a playout that has already been corrupted by it.
//
// How they differ, which is the point of being able to compare them:
//
//   false -- single-point test. A point is an eye for C when every neighbour is C. Cheap
//            (O(degree) per candidate, no allocation) and it bites from the first eye
//            that forms. It is a heuristic: it misses multi-point eyes, and it treats a
//            false eye as real.
//   true  -- Benson. Computes the chains that are unconditionally alive -- alive against
//            any sequence of opponent moves -- and forbids playing inside their vital
//            regions. Exact, and it covers multi-point eyes the cheap rule misses. But it
//            is conservative in a way that matters here: on an empty or lightly-played
//            board no chain is yet pass-alive, so it forbids NOTHING and early playouts
//            fill eyes freely. It also costs a fixed-point iteration over the whole graph
//            per playout ply, against the cheap rule's handful of neighbour reads.
inline constexpr bool kUseBensonEyeRule = true;

// Cheap rule: true when `nodeId` is empty and every neighbour carries `color`.
// A node with no neighbours is not an eye.
bool isSinglePointEye(const Game& game, int nodeId, Color color);

// Benson's rule: fills `scratch.territory` with 1 at every EMPTY point inside `color`'s
// unconditionally-alive territory -- the vital regions of its pass-alive chains -- and 0
// elsewhere. Returns a reference to it, valid until the next call with the same scratch.
//
// Benson, "Life in the Game of Go" (Information Sciences, 1976). Chains of `color` and
// the maximal connected regions of not-`color` are alternately eliminated: a chain
// survives only while at least two surviving regions are vital to it (every empty point
// of the region is a liberty of the chain), and a region survives only while every chain
// on its boundary survives. What remains cannot be killed however the opponent plays.
const std::vector<char>& bensonPassAliveTerritory(const Game& game, Color color,
                                                 BensonScratch& scratch);

// Allocation-free single-point eye bonus.
// For each empty node whose every neighbor is the same color, credit eyeWeight
// to that color. Weight >= 1.0 makes filling a single-point eye slightly
// worse than not filling it under Chinese area scoring (stone +1, eye bonus -w).
// Suitable for use in staticEval() where no heap allocation is desired.
std::pair<double, double> singleEyeBonus(const Game& game,
                                          double eyeWeight = 1.05);

// Flood-fill enclosed-region evaluator.
// Finds every connected empty region whose entire stone boundary belongs to
// one color (i.e., fully enclosed territory) and credits each empty node in
// that region to the enclosing color. This captures multi-point eyes and
// fully enclosed groups that lie outside the DVR radius, complementing the
// DVR-based territory estimate in negamaxEval().
//
// The default eyeWeight (0.3) is intentionally small relative to DVR's
// areaPremium to avoid double-counting nodes already inside the DVR.
class EyeEval {
public:
    explicit EyeEval(const Game& game, double eyeWeight = 0.3);

    double blackBonus() const { return black_; }
    double whiteBonus() const { return white_; }

    // Value from the perspective of the player currently to move.
    double relativeValue() const;

private:
    double black_  = 0.0;
    double white_  = 0.0;
    Player toMove_;
};

} // namespace IrrGo
// Copyright Ben Paul Wise. All Rights Reserved.

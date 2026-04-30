// Copyright Ben Paul Wise. All Rights Reserved.
#pragma once
#include "Game.h"
#include <utility>

namespace IrrGo {

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

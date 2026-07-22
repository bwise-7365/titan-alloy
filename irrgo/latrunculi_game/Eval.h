// Copyright Ben Paul Wise. All Rights Reserved.
#pragma once

#include "Game.h"

#include <vector>

// Pure positional evaluation for Latrunculi. Every function here reads a board array and
// its dimensions and nothing else -- no game state, no side effects -- so each can be
// reasoned about and tested in isolation (CLAUDE.md: prefer referential transparency).
//
// Why these terms exist: the previous leaf evaluation scored mobility x material for each
// side, which collapses algebraically to "maximise my own move count times my own
// material". That is maximised by spreading out into empty space -- i.e. by AVOIDING the
// contact a custodial capture requires -- while the capture that contact enables sits
// beyond the search horizon. Both engines were being paid to run away from each other.
// The terms below instead score the things that actually precede a capture: threats and
// pincer-shaped formations. Raw mobility survives only as a small tie-breaker.
// See doc/2026-07-21-latrunculi-dynamism-analysis.md, recommendation 2.
namespace Latrunculi {

// ── Cell <-> player helpers (shared by Game.cpp and Eval.cpp) ────────────────

// The owning player of a cell, or -1 for Empty.
inline int playerOf(Cell c) {
    switch (c) {
        case Cell::P0Free:
        case Cell::P0Bound:
            return 0;
        case Cell::P1Free:
        case Cell::P1Bound:
            return 1;
        default:
            return -1;
    }
}

inline Cell freeCell(int player) {
    return (player == 0) ? Cell::P0Free : Cell::P1Free;
}

inline Cell boundCell(int player) {
    return (player == 0) ? Cell::P0Bound : Cell::P1Bound;
}

// ── Positional features ──────────────────────────────────────────────────────

// Positional features of one side, counted over a board arrangement. All four are
// defined in both phases, so the evaluation no longer goes blind during placement.
struct PositionalTerms {
    // Enemy Free discs this side half-pins: this side holds one end of an orthogonal
    // axis through the enemy disc with a Free disc, and the square at the other end is
    // on the board and empty. One move into that square completes the custodial capture,
    // so a threat is material that is one tempo away.
    int threats = 0;
    // Orthogonally adjacent pairs of this side's own Free discs, each counted once. A
    // pair is half of a pincer and a launch platform for a leap, and it is the smallest
    // unit of structure a placement phase can build.
    int pairs = 0;
    // A cheap stand-in for mobility, summed over this side's Free discs and measured in
    // whatever a move actually is under the game's MoveStyle:
    //   StepLeap - empty orthogonal neighbours. Ignores leap chains, which is part of why
    //              the term carries only a small weight.
    //   Slide    - empty squares along the four rays out of the disc, i.e. exactly the
    //              destinations a slide can reach, so under this rule set the proxy is
    //              the real move count.
    // The two are on different scales: a disc has at most 4 empty neighbours but can have
    // up to (rows-1)+(columns-1) slide destinations. That is a property of the rule sets,
    // not a defect, but it means kMobilityWeight does NOT mean the same thing in both --
    // the weights want separate calibration per style.
    int openNeighbours = 0;
    // Summed centrality of this side's Free discs; see centrality() in Eval.cpp.
    double centrality = 0.0;
};

// Count `player`'s positional features over `cells`, a rows x columns board in row-major
// order, under the movement rule `style` (which only affects openNeighbours; the other
// three terms are pure board shape). Throws std::invalid_argument if rows/columns are not
// positive or do not match cells.size(), rather than quietly scoring a malformed board.
// `style` has no default: a caller that does not know which rule set it is scoring cannot
// produce a meaningful mobility figure, and should not be handed a plausible one.
PositionalTerms positionalTerms(const std::vector<Cell>& cells, int rows, int columns,
                                int player, MoveStyle style);

// The positional terms combined into one score in "disc units", where 1.0 is worth one
// Free disc of material. Weights are declared and justified in Eval.cpp.
double positionalScore(const PositionalTerms& terms);

}  // namespace Latrunculi
// Copyright Ben Paul Wise. All Rights Reserved.

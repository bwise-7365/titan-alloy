// Copyright Ben Paul Wise. All Rights Reserved.
#pragma once

#include "EvalWeights.h"
#include "Game.h"

#include <cstddef>
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

// ── Orthogonal neighbour offsets (shared by Game.cpp and Eval.cpp) ───────────

// The four orthogonal directions, in one fixed order. Custodial capture, movement,
// reachability and every neighbour scan index this table; a local copy that reordered
// or extended it would disagree with the others without saying so.
inline constexpr int kDRow[4] = {-1, 1, 0, 0};
inline constexpr int kDColumn[4] = {0, 0, -1, 1};

// One representative direction per orthogonal AXIS, for scans that must visit each axis
// once rather than each direction twice (a pin is a property of an axis, not a direction).
inline constexpr int kAxisDRow[2] = {1, 0};
inline constexpr int kAxisDColumn[2] = {0, 1};

// ── Board addressing (shared by Eval.cpp and PlacementEval.cpp) ──────────────

inline std::size_t cellIndex(int row, int column, int columns) {
    return static_cast<std::size_t>(row) * static_cast<std::size_t>(columns) +
           static_cast<std::size_t>(column);
}

inline bool onBoard(int row, int column, int rows, int columns) {
    return row >= 0 && row < rows && column >= 0 && column < columns;
}

// ── Shared flanking predicates ───────────────────────────────────────────────
// One definition each, used by both the movement terms here and the placement terms in
// PlacementEval.cpp. The improvements doc flags duplicated ray walks as a defect; these
// were file-local in Eval.cpp until the placement evaluation needed them too.

// If the disc at (row, column) is half-pinned along the axis (dRow, dColumn) by
// `flanker` -- one end of the axis holds a flanker disc and the other end is on the board
// and empty -- returns the index of that empty square, the one whose occupation completes
// the custodial capture. Returns -1 if the disc is not half-pinned on this axis.
int halfPinCompletion(const std::vector<Cell>& cells, int rows, int columns,
                      int row, int column, int dRow, int dColumn, Cell flanker);

// True if `player` has a Free disc that can move onto the empty square `square` in one
// move under `style`. This is what separates a threat from a shape that merely looks
// like one; see the definition in Eval.cpp for what is and is not checked.
bool canOccupy(const std::vector<Cell>& cells, int rows, int columns, int square,
               int player, MoveStyle style);

// True if the disc at (row, column) -- an enemy disc from `player`'s point of view -- is
// one completable move from being custodially captured by `player`, on either axis.
// Does NOT consider the corner-trap rule; PlacementEval.cpp layers that on separately.
bool isThreatened(const std::vector<Cell>& cells, int rows, int columns,
                  int row, int column, int player, MoveStyle style);

// ── Positional features ──────────────────────────────────────────────────────

// Positional features of one side, counted over a board arrangement. All four are
// defined in both phases, so the evaluation no longer goes blind during placement.
struct PositionalTerms {
    // Enemy Free discs this side threatens: this side holds one end of an orthogonal axis
    // through the enemy disc with a Free disc, the square at the other end is on the board
    // and empty, AND this side has a Free disc that can move onto that square under the
    // game's MoveStyle. The last condition is what makes the count mean something -- a
    // half-pin whose completing square no disc can reach is not a threat, and counting it
    // mispriced every position where that square was walled off. See canOccupy() in
    // Eval.cpp for what the reachability test does and does not check.
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
// Free disc of material. The defaulted weights reproduce the engine's standard
// behavior; the bench injects swept candidates.
double positionalScore(const PositionalTerms& terms,
                       const EvalWeights& weights = EvalWeights{});

}  // namespace Latrunculi
// Copyright Ben Paul Wise. All Rights Reserved.

// Copyright Ben Paul Wise. All Rights Reserved.
#pragma once

#include "Game.h"

#include <string>
#include <vector>

// Phase-boundary report for latrunculi_bench: how exposed each army is at the instant
// placement ends, and how many captures each side then makes in the first movement
// plies. Measures the surplus of captures at the start of movement described in
// doc/2026-09-03-latrunculi-placement-capture-surplus.md (section 10.2) rather than
// inferring it from whole-game capture totals. Bench-only code, kept out of bench.cpp
// for the same reason BenchAb is.
//
// Everything here is computed by replaying a game's move history on a fresh board, as
// GameStats does, so it is a pure function of the game and adds nothing to the play
// loop. Unlike GameStats it does NOT require the game to be over: a run capped at a ply
// count (bench plies=) produces unfinished games on purpose, and the boundary is fully
// determined once the early movement plies are in the history.
namespace Latrunculi::Bench {

// How many movement plies after the flip are counted as "early". Twenty is ten moves a
// side on the standard board, long enough for every one-move capture that existed at
// the flip to be taken or defended against.
inline constexpr int kEarlyMovementPlies = 20;

struct BoundaryStats {
    // False if the game never reached the movement phase (ended, or was capped,
    // during placement); every other field is then meaningless and printed as "-".
    bool reachedMovement = false;

    // Per side, indexed by player, measured on the board at the flip: the position
    // after the last placement and before the first movement ply.
    int exposed[2] = {0, 0};         // PlacementTerms::oneMoveCapturable
    int threats[2] = {0, 0};         // PositionalTerms::threats
    int notchExposure[2] = {0, 0};   // PlacementTerms::notchExposure
    int vulnerableAxes[2] = {0, 0};  // PlacementTerms::vulnerableAxes

    // Captures MADE BY each side over the early movement plies (an event per ply, as
    // GameStats counts them). `earlyPlies` is how many movement plies were actually
    // observed, at most kEarlyMovementPlies; fewer means the game ended or was capped
    // inside the window, and the counts cover only those plies.
    int captures[2] = {0, 0};
    int earlyPlies = 0;
};

// Replay `game` from an empty board and measure its boundary. Throws
// std::invalid_argument if the history does not replay legally.
BoundaryStats analyseBoundary(const Game& game);

// Fixed-width columns appended to a per-game row, and their header.
std::string boundaryColumns(const BoundaryStats& b);
std::string boundaryHeader();

// Multi-line summary over a run: per-side means with standard errors, and the count of
// games measured.
std::string formatBoundarySummary(const std::vector<BoundaryStats>& all);

}  // namespace Latrunculi::Bench
// Copyright Ben Paul Wise. All Rights Reserved.

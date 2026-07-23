// Copyright Ben Paul Wise. All Rights Reserved.
#pragma once

#include "Game.h"

#include <vector>

// Game-quality metrics for a finished Latrunculi game.
//
// Why these particular numbers: the Digital Ludeme Project chose between candidate
// latrunculi rule sets by running batches of self-play and comparing on completion,
// duration, decisiveness and lead change rather than by watching games. The same
// measures apply here, and until they exist every claim about whether a change made the
// game better is a viewing rather than a result. Metric definitions follow Browne's
// thesis; see doc/2026-07-22-latrunculi-further-improvements.md section 1.
//
// Everything here is computed by replaying a finished game's move history on a fresh
// board, so it is a pure function of the game and adds nothing to the play loop.
namespace Latrunculi {

struct GameStats {
    // ── Duration ────────────────────────────────────────────────────────────
    int plies = 0;           // total plies, both phases
    int placementPlies = 0;  // plies spent in the placement phase
    int movementPlies = 0;   // plies spent in the movement phase

    // ── Activity ────────────────────────────────────────────────────────────
    // A capture binds an enemy Free disc; a removal deletes a Bound one. Both are
    // counted as events, not as discs: one ply removes at most one captive, and a
    // single move can bind more than one enemy at once.
    int captures = 0;
    int removals = 0;
    // 1-based ply of the last capture or removal, or 0 if the game had neither. The
    // gap between this and `plies` is the dead tail -- the "front-loaded then quiet"
    // complaint this whole thread started from, expressed as a number.
    int lastActivePly = 0;
    // Longest run of consecutive movement plies with neither a capture nor a removal.
    // Reaching pacificMoveLimit here means the quiet-game rule is what ended the game.
    int longestQuietRun = 0;

    // ── Decisiveness ────────────────────────────────────────────────────────
    int winner = -1;
    WinReason reason = WinReason::None;
    // Free-disc counts at the end. The margin is freeP0 - freeP1 from player 0's side;
    // a game won by one disc is not the same result as a rout, and averaging the two
    // hides which happened.
    int freeP0 = 0, freeP1 = 0;
    int boundP0 = 0, boundP1 = 0;
    int finalMargin = 0;  // freeP0 - freeP1

    // ── Drama ───────────────────────────────────────────────────────────────
    // Sign changes in the free-disc margin over the movement phase. Zero means whoever
    // drew first blood kept the lead to the end, which is the shape of a dull game
    // however long it ran.
    int leadChanges = 0;
    // Largest margin either side held at any point, in discs. A game whose final margin
    // is 1 but whose peak was 6 was a comeback; both numbers are needed to tell.
    int peakMargin = 0;
};

// Replay `finished` from an empty board and measure it. The game must be over --
// statistics on an unfinished game would describe nothing, so this throws
// std::invalid_argument rather than returning a partial answer. Throws if the history
// does not replay legally, which would mean the recorded game and the rules disagree.
GameStats analyseGame(const Game& finished);

// One line per game, for a batch run: fixed-width columns, no header.
std::string formatStatsRow(const GameStats& s, int gameIndex);
// The header matching formatStatsRow.
std::string statsHeader();
// Aggregate summary over a batch.
std::string formatStatsSummary(const std::vector<GameStats>& all);

}  // namespace Latrunculi
// Copyright Ben Paul Wise. All Rights Reserved.

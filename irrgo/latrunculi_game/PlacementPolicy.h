// Copyright Ben Paul Wise. All Rights Reserved.
#pragma once

#include "AbsGame.h"
#include "Game.h"

#include <cstdint>
#include <random>
#include <vector>

namespace Latrunculi {

// Opening-variety policy for the placement phase, shared by the self-play driver and the
// GUI so the two cannot drift apart.
//
// Each side plays its FIRST placement at random and every later placement searched. The
// two random stones are the whole perturbation: they give each game a different seed
// pair, and the searched placements then build real formations around it, so games from
// different seeds diverge without the opening degenerating into noise. (An earlier
// version re-randomised every third or fourth placement; that left about 30 per cent of
// all placements unsearched and the armies badly exposed when movement began, so it was
// cut back to the first stone per side. See doc/2026-09-03-latrunculi-placement-
// capture-surplus.md, section 8.)
//
// The random stones are not uniform over the whole board: they land off the border, and
// the second lands on a different row AND column from the first and not diagonally next
// to it, so the two seeds are genuinely apart. See pickRandomPlacement for the exact
// preference order.
//
// One seeded RNG drives the random squares, so recording the seed reproduces an opening
// exactly.
class PlacementPolicy {
public:
    explicit PlacementPolicy(std::uint64_t seed);

    // True if `player`'s next placement should be random rather than searched: true for
    // that side's first placement since construction or reset, false ever after. This
    // advances that player's state, so call it exactly once per placement. Throws
    // std::invalid_argument unless player is 0 or 1.
    bool nextIsRandom(int player);

    // Pick one of `moves` (which must all be legal placements for `game`) at random,
    // preferring squares well apart from whatever is already on the board. The
    // preference is a strict fallback ladder, taken uniformly within the first tier that
    // is non-empty:
    //   1. off the border, on no row or column holding a disc, and not diagonally
    //      adjacent to any disc;
    //   2. off the border;
    //   3. anything legal.
    // On an empty board tier 1 is simply the interior. Tiers 2 and 3 are reachable
    // positions, not error paths: a board under 3x3 has no interior at all, and a caller
    // that randomises every placement (bench's placement=random) soon runs out of clear
    // rows and columns.
    // Throws std::invalid_argument if `moves` is empty rather than inventing a move for
    // a position that has none.
    AbsGame::MoveId pickRandomPlacement(const Game& game,
                                        const std::vector<AbsGame::MoveId>& moves);

    // Restart the policy -- both sides' first placements random again -- on a fresh seed.
    void reset(std::uint64_t seed);

private:
    std::mt19937_64 rng_;
    // True once the side has taken its random first placement.
    bool placedFirst_[2] = {false, false};
};

}  // namespace Latrunculi
// Copyright Ben Paul Wise. All Rights Reserved.

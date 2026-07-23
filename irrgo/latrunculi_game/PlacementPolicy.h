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
// A coarse epsilon-greedy. Each side plays its FIRST placement at random, then has its
// placements searched for a run of kRunMin or kRunMin+1 (50/50, drawn fresh each time),
// then places at random again, and so on. The random placements are the perturbation
// that stops openings repeating; the searched runs let the engine build real formations
// around them, so two games from different seeds diverge without either degenerating
// into noise. Each side keeps its own counter and draws its own run lengths, so the two
// sides fall out of step with each other as soon as their coins differ.
//
// The random placements are not uniform over the whole board: they prefer squares off
// the border and clear of existing pieces, so each one lands as a fresh seed with room
// to grow rather than on an edge or welded to what is already there. See
// pickRandomPlacement for the exact preference order.
//
// One seeded RNG drives both the run lengths and the random squares, so recording the
// seed reproduces an opening exactly.
class PlacementPolicy {
public:
    // Shortest run of searched placements between two random ones.
    static constexpr int kRunMin = 2;

    explicit PlacementPolicy(std::uint64_t seed);

    // True if `player`'s next placement should be random rather than searched. This
    // advances that player's state, so call it exactly once per placement. Throws
    // std::invalid_argument unless player is 0 or 1.
    bool nextIsRandom(int player);

    // Pick one of `moves` (which must all be legal placements for `game`) at random,
    // preferring well-spaced interior squares. The preference is a strict fallback
    // ladder, taken uniformly within the first tier that is non-empty:
    //   1. off the border AND orthogonally clear of every piece already on the board;
    //   2. off the border;
    //   3. anything legal.
    // Tiers 2 and 3 are reachable positions, not error paths: a crowded board runs out
    // of isolated squares, and a board under 3x3 has no interior at all.
    // Throws std::invalid_argument if `moves` is empty rather than inventing a move for
    // a position that has none.
    AbsGame::MoveId pickRandomPlacement(const Game& game,
                                        const std::vector<AbsGame::MoveId>& moves);

    // Restart the policy -- both sides random again -- on a fresh seed.
    void reset(std::uint64_t seed);

private:
    std::mt19937_64 rng_;
    // Searched placements still owed in the current run, per side. Both start at 0, so
    // each side's first placement is random and draws that side's first run length.
    int optimalRemaining_[2] = {0, 0};
};

}  // namespace Latrunculi
// Copyright Ben Paul Wise. All Rights Reserved.

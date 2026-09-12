// Copyright Ben Paul Wise. All Rights Reserved.
#pragma once

// The evaluation's tunable weights, in their own header because both Game.h (which
// stores a set per game) and Eval.h / PlacementEval.h (whose scoring functions take
// one) need the type, and Eval.h already includes Game.h.
namespace Latrunculi {

// Every tunable weight of the evaluation, in "disc units": what one unit of each raw
// term is worth against one Free disc of material. The defaults ARE the engine's
// behavior -- Game evaluates with a default-constructed EvalWeights unless a caller
// injects another via Game::setEvalWeights -- so the GUI and selfplay play exactly the
// pre-struct engine. The struct exists so the bench can fight two weight sets against
// each other at runtime instead of recompiling per candidate (tools/latrunculi-sweep.ps1).
//
// Weights are SIGNED multipliers on non-negative counts: a penalty is a negative
// weight, not a negated term. A sweep may legitimately probe a sign flip, so no
// validation constrains sign -- only finiteness (validateEvalWeights).
struct EvalWeights {
    // ── Movement/positional terms (PositionalTerms, Eval.h) ──
    // A threat is the most valuable positional asset because it is one move from
    // becoming material. A pair is a threat in waiting. Mobility and centrality are
    // small tie-breakers whose job is to choose between otherwise equal quiet moves --
    // they must never add up to a real capture, which is why they are an order of
    // magnitude smaller.
    double threat = 0.25;
    // Lowered 0.10 -> 0.02 on 2026-07-22 after self-play under both movement rules
    // produced two large mutually-defending blobs and ran out the quiet-game limit.
    // `pairs` counts EVERY adjacency between own discs, so the bonus grows
    // quadratically with clumping while the mobility penalty grows only linearly: nine
    // discs in a solid 3x3 block hold 12 pairs against 12 open neighbours, where the
    // same nine dispersed hold 0 pairs and 36 open neighbours. At 0.10 that was +1.20
    // against +0.24 -- the engine was paid roughly three quarters of a captured disc,
    // every ply, to build a fortress, and the blob was the eval's stated optimum rather
    // than a search artifact. At 0.02 the block scores +0.24 against +0.72 and
    // dispersal wins outright. This rebalances the two weights; it does not fix the
    // term's shape (see doc/latrunculi-implementation-plan.md, Stage 7).
    double pair = 0.02;
    double mobility = 0.02;
    double centre = 0.05;

    // ── Placement terms (PlacementTerms, PlacementEval.h) ──
    // First guesses awaiting the tuning campaign (doc/2026-08-24-latrunculi-placement-
    // heuristics.md, section 5); the magnitudes were chosen so a fully realized
    // structural advantage stays under one disc of material.
    double vulnerableAxes = -0.03;     // penalty per own open flanking axis (F1-F3)
    double oneMoveCapturable = -0.9;   // penalty per own en-prise disc, seam-ramped (A1)
    double spearheadPairs = 0.15;      // bonus per backed pair aimed at an enemy (F4)
    double diagonalSupport = 0.06;     // bonus per diagonal with open notches (F6)
    double deniedSquares = 0.04;       // bonus per square the enemy can never use (F5)
    double strikers = 0.05;            // bonus per mobile attacker, saturating (H6)
    double notchExposure = -0.12;      // penalty per own disc in an enemy notch (A2/F7)
};

// Throws std::invalid_argument if any field is non-finite (NaN or infinity). Negative
// values are allowed -- see the sign-flip note on EvalWeights. Public so a caller
// parsing user input (the bench's wA./wB. keys) can reject a bad value at the point of
// entry; follows the validateKomi precedent. Defined in Eval.cpp.
void validateEvalWeights(const EvalWeights& weights);

}  // namespace Latrunculi
// Copyright Ben Paul Wise. All Rights Reserved.

// Copyright Ben Paul Wise. All Rights Reserved.
#pragma once

#include "Eval.h"

#include <vector>

// Placement-phase evaluation for Latrunculi. The placement phase has no material signal
// at all -- no move captures, so every sibling position holds the same disc counts --
// and the movement-oriented positional terms in Eval.h see only part of what placement
// is really about: building structures that cannot be captured later while keeping the
// mobile attackers that captures will need. The terms here score exactly that. Each is
// a pure function of a board arrangement, like everything in Eval.h, and each
// implements one derived fact or heuristic from
// doc/2026-08-24-latrunculi-placement-heuristics.md (the F/H/A labels below).
namespace Latrunculi {

// Placement-phase features of one side, counted over a board arrangement. All counts
// are non-negative; whether a term is a bonus or a penalty is carried by the sign of
// its weight in EvalWeights, so a sweep can probe either direction.
struct PlacementTerms {
    // Axes on which one of this side's Free discs could eventually be flanked: both
    // opposite squares are on the board and are each empty or an enemy Free disc, so
    // nothing (edge, friend, or dead enemy) blocks a future sandwich. A corner disc has
    // no such axis but is exposed to the corner-trap rule instead; that exposure counts
    // here as one axis when both squares beside the corner are open in the same sense.
    // Facts F1-F3: safety is exactly the count of axes an edge or a friend blocks.
    int vulnerableAxes = 0;
    // This side's Free discs the enemy can capture with ONE move right now: half-pinned
    // with the completing square empty and reachable by an enemy Free disc (or the
    // corner-trap equivalent). Penalty A1 -- and the reason it is scored separately
    // from vulnerableAxes is the phase seam (F10): at the end of placement this is not
    // a liability but a dead disc, which is why its weight is ramped by progress.
    int oneMoveCapturable = 0;
    // Pairs of this side's Free discs aimed at an adjacent enemy: the pattern
    // [own][own][enemy Free][empty], scanned along the pair's own axis in both
    // directions (a pair with an enemy at each end counts twice). The backer makes the
    // contact one-sided -- the enemy cannot flank the attacker on that axis -- which is
    // the "spearhead" of fact F4, and the shape the improvements doc (section 2.1)
    // wanted the raw pairs term restricted to.
    int spearheadPairs = 0;
    // Diagonal adjacencies between this side's Free discs whose two "notch" squares
    // (the orthogonal squares between the diagonal ends) are both empty. Fact F6: an
    // enemy entering a notch is half-pinned on two axes at once, so an open diagonal
    // projects cross-fire without the discs blocking each other's slides.
    int diagonalSupport = 0;
    // Empty squares flanked on one axis by two of this side's Free discs. The enemy can
    // neither place there (walking into a flank) nor move there (suicide), while this
    // side still can; each is a square of space denied at no ongoing cost (fact F5).
    // Counted once per square even when both axes deny it.
    int deniedSquares = 0;
    // This side's Free discs mobile enough to spring a trap: under Slide, at least two
    // rays with two or more empty squares; under StepLeap, at least two empty
    // orthogonal neighbours. Captures only happen when a mover reaches a completing
    // square (fact F9), so an army needs a floor of these however safe its walls are.
    // The raw count is reported; placementScore saturates it (heuristic H6).
    int strikers = 0;
    // This side's Free discs half-pinned on BOTH axes: each axis holds an enemy Free
    // disc on one side and an empty square on the other -- standing in an enemy notch
    // (penalty A2, fact F7). Adjacency alone is not exposure; a friend or edge on the
    // far side closes that axis for good, which is how a 2x2 block sits safely against
    // four attackers. Distinct from oneMoveCapturable, which also demands a mover able
    // to reach a completing square this turn.
    int notchExposure = 0;
};

// Count `player`'s placement features over `cells`, a rows x columns board in row-major
// order, under movement rule `style` (which affects the reachability tests inside
// oneMoveCapturable and the striker mobility test; the other terms are pure board
// shape). Throws std::invalid_argument on a malformed board or player, exactly as
// positionalTerms does.
PlacementTerms placementTerms(const std::vector<Cell>& cells, int rows, int columns,
                              int player, MoveStyle style);

// The seam ramp applied to the oneMoveCapturable weight: mild early in placement (the
// enemy must still spend placements to exploit a loose disc) and rising to full
// strength as the phase ends, where a one-move capture is executed immediately by the
// first movement ply (fact F10). Cubic so the last few placements carry most of the
// climb: kSeamRampFloor + (1 - kSeamRampFloor) * progress^3. Exposed so tests and the
// ordering path share one definition.
double seamRamp(double progress);

// The placement terms combined into one score in "disc units", where 1.0 is worth one
// Free disc of material. `progress` is placed/perSide for the side being scored, in
// [0, 1]; `perSide` sets the striker saturation cap max(3, perSide/5). Throws
// std::invalid_argument if progress is outside [0, 1] (NaN included) or perSide < 1:
// a caller that does not know how far placement has advanced cannot ask for a
// seam-ramped score, and should not be handed a plausible one.
double placementScore(const PlacementTerms& terms, double progress, int perSide,
                      const EvalWeights& weights = EvalWeights{});

}  // namespace Latrunculi
// Copyright Ben Paul Wise. All Rights Reserved.

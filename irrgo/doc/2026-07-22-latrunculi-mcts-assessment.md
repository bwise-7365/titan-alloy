<!-- Copyright Ben Paul Wise. All Rights Reserved. -->

# MCTS for Latrunculi — assessment and deferred plan

Written 2026-07-22, after the negamax measurement programme (`doc/bench/README.md`)
produced numbers that change what MCTS work would be worth doing.

**DECISION, 2026-07-22 (Ben): not worthwhile now.** MCTS remains selectable in the GUI
and is left as it stands. This document exists so the reasoning does not have to be
rebuilt if that changes. Nothing below has been implemented.

Read first: `doc/2026-07-21-latrunculi-dynamism-analysis.md` (the research pass, with the
UCT literature), `doc/bench/README.md` (the measurements this rests on), and
`absgame/mcts.cpp` (all line references are to that file).

## Why the answer is "not now"

The strongest argument against investing here is not that the MCTS implementation is
weak — it is, and fixably so — but that **this game's rules make rollouts nearly
worthless**, so a correct implementation would still be fighting the domain.

The Pacific (quiet-game) rule ends a movement phase after `pacificMoveLimit` = 40
capture-free plies and decides it on material. A rollout captures rarely, so it
terminates in roughly 40 plies with a result that reflects the material balance at the
moment the rollout *started*. A playout is therefore a slow, noisy re-derivation of
something `staticEval()` computes directly and precisely. The measurements support this
from the other direction: even under searched play, the two step/leap batches ended
50/50 and 50/50 on the quiet-game rule with a dead tail of exactly 40 plies every time.

Negamax, by contrast, suits the game well. It assumes the most threatening reply, and the
data says sustained pressure is what produces decisive wins: 52% of slide+convex games
ended by reduction, meaning a player was ground down over many plies rather than losing
to a single tactic.

## Defects in the current implementation

These are real regardless of the decision above, and are recorded so they need not be
rediscovered.

### 1. Reward scale breaks UCB1 (the blocking defect)

`rollout` (line 97) returns raw `staticEval()`: 1000–2000 for a terminal position, single
digits for a non-terminal one. `uctScore` (line 48) adds an exploration term of about
1–4. UCB1 assumes a bounded reward range; this violates it by three orders of magnitude.
The consequence is not a subtle bias — one rollout that happens to terminate outweighs a
hundred that do not, so the tree is dragged toward whichever branch terminated rather
than whichever is good. **Nothing else in this list can be evaluated until this is
fixed**, because every other change would be measured through a broken selection rule.

Shape of the fix: map a terminal to `sign x (0.5 + 0.5*s)`, so any win scores at least
0.5 and a rout approaches 1.0; squash the non-terminal leaf through `tanh(eval / k)`
bounded into (-0.5, 0.5) so a heuristic estimate can never outrank a real win. This
preserves the ConvexMargin payoff's ordering, which was measured as worth 42% -> 52%
decisive finishes, while making the arithmetic valid.

### 2. Full expansion before any UCT comparison

`treePolicy` (lines 78-85) expands EVERY child before UCT runs once. Branching under the
slide rule is 150-250. At the root that is 250 rollouts spent before the first selection
decision. A capture needs two plies to cash and three to be safe, so at any GUI-scale
budget the tree never reaches the depth at which a capture becomes visible.

Fix: progressive widening, allowing `ceil(C * N^0.5)` children with C ~ 2, and expanding
in `Game::moveOrderScore` order. That virtual already exists and already ranks removals
above new binds above quiet moves — the ordering MCTS needs is written and tested.

### 3. `kMaxRolloutDepth` = 200 was set without knowing game length

Measured mean total plies: 171.8 (slide+gradient) to 189.6 (slide+convex). The cap sits
just above that, so rollouts from early nodes routinely hit it and fall back to
`staticEval()` on a non-terminal position — feeding defect 1 its worst case.

Given the Pacific-rule argument above, the better fix is not a larger cap but a much
smaller one: truncate at 10-20 plies and return the normalised evaluation. Little is
lost, because the remainder of the playout was mostly re-deriving current material, and
an order of magnitude more iterations is bought.

NOTE this reverses the ordering given in Stage 6 of the implementation plan, which had
truncated rollouts as a later refinement behind full heavy playouts. The Pacific-rule
interaction was not understood when that list was written.

### 4. `robustChild` strict `>` (lines 125-135)

Returns `children[0]` on an all-tied root — a fixed `scanOrder_` bias for the whole game.
This is the same bug class as the negamax root-selection defect fixed on 2026-07-21. With
few iterations and many children, ties are common.

### 5. No tree reuse between plies

Both `Searcher::mcts` overloads build a fresh root each call (lines 164, 195). In GUI
auto-play the tree is discarded every ply, when descending into the played child would
retain a subtree already worth 1/B of the work. Cheap, and it compounds with fix 2.

### 6. `chooseRolloutMove` is expensive per rollout ply

`Latrunculi::Game::chooseRolloutMove` copies the board and resolves a move for each of up
to `kRolloutSampleCap` = 12 sampled candidates, every rollout ply. The Zobrist and
scratch-board work done for legal-move enumeration on 2026-07-22 did not touch this path,
so it is now the remaining hot spot in any rollout-heavy search.

## If it is ever taken up

Order matters; 1 gates everything.

1. Normalise rewards to [-1, 1].
2. Add `algo=mcts` to `latrunculi_bench`. Until this exists none of the rest is
   verifiable, and MCTS tuning stays a matter of opinion. Small change to `bench.cpp`,
   which already takes named arguments.
3. Progressive widening ordered by `moveOrderScore`.
4. Truncated rollouts (10-20 plies) returning the normalised evaluation.
5. `robustChild` tie-break.
6. Tree reuse across plies.
7. Only if MCTS still underperforms after all of the above: implicit minimax backups
   (Lanctot et al., linked in the analysis document). MCTS's averaging backup
   systematically under-values a forcing line where most continuations are mediocre and
   one is winning — which is exactly the shape of a custodial capture. A max/average
   blend is the standard remedy and the right one to reach for, rather than more
   iterations.

## Cautions

- **Expect MCTS to remain weaker than negamax on this game even done well.** The
  Pacific rule is the reason, and no amount of implementation quality removes it. Decide
  whether that is acceptable before starting, not after.
- **Do not tune MCTS against negamax self-play numbers.** The `doc/bench/` figures were
  produced by negamax and describe how negamax plays. An MCTS batch needs its own
  baseline.
- **A rules change would invalidate this analysis.** Most of the argument above rests on
  the Pacific rule at 40 plies. If `pacificMoveLimit` changes materially, or the
  quiet-game termination is replaced, the "rollouts are worthless" conclusion has to be
  re-derived rather than assumed.

<!-- Copyright Ben Paul Wise. All Rights Reserved. -->

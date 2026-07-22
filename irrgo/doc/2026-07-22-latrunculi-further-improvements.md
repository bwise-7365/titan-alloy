<!-- Copyright Ben Paul Wise. All Rights Reserved. -->

# Latrunculi — suggested further improvements

Written 2026-07-22, at the point where self-play stopped producing two large uncapturable
blobs. This is a parking document: it records what is still known to be wrong, what was
changed without being measured, and what the next moves are if the games turn dull again.
It is not a plan of record — `doc/latrunculi-implementation-plan.md` is that, and Stages 6
and 7 there cover what has already been built.

Background reading, in order:

1. `doc/2026-07-21-latrunculi-dynamism-analysis.md` — the five-agent research pass, 44
   sources, 40 verified open access. The five numbered recommendations referenced below
   are that document's.
2. `doc/latrunculi-implementation-plan.md`, Stages 6 and 7.
3. `design_resources/Latrunculi rules - BPW.md` — the rules source. The `.txt` version's
   appended variant rules are out of scope.

## Where things stand

Three changes landed within a day of each other, and the improvement cannot be attributed
between them:

- Kharebga slide movement (`MoveStyle::Slide`, now the default).
- `kPairWeight` 0.10 → 0.02.
- `PositionalTerms::openNeighbours` made style-aware, which multiplied that term's scale
  by roughly three under Slide without the weight changing.

The first of these on its own made the blobbing *worse*. So the eval, not the movement
rule, was the dominant term, and the combination is what worked. If any of the three is
ever reverted, expect the other two to behave differently than they do now.

Nothing here has been measured against anything. There are no numbers, only a viewing.

**Update, same day.** Three items from this document were then applied — the placement dead
end (§5), incremental Zobrist hashing (§3.2) and the threats legality filter (§2.3) — on the
principle that anything changing how the engine plays must land before a baseline is taken.
They are struck through below and written up as Stage 8 of the plan document. Everything
else stands.

## 1. Measure before tuning again  — DONE (Stage 9)

Built as `latrunculi_bench` (`latrunculi_game/bench.cpp`) with the metric definitions in
`GameStats.{h,cpp}`. The rest of this section is the specification it was built from and
is kept for the reasoning.

Everything below is guesswork until self-play is instrumented. The Digital Ludeme Project
chose the Kharebga ruleset by running 100 alpha-beta playouts per candidate and comparing
on completion rate, duration, decisiveness, lead change and drama; the same metrics apply
here and would settle several open arguments at once.

Minimum useful instrumentation, in the self-play driver:

- Plies to termination, and which `WinReason` ended it. A high share of `QuietGame` is the
  symptom this whole thread has been chasing.
- Captures and removals per game, and the ply index of the last one. "Front-loaded then
  quiet" shows up here directly.
- Final material difference. A game decided by one disc is not the same as a rout.
- Lead changes: how often the sign of the material difference flips after the opening.

Run it over a batch of seeds, not one game, and print a summary table. The
`PlacementPolicy` seed is already printed for reproduction, so a suspicious game can be
replayed exactly.

References for the metric definitions: Browne's thesis, the Foundations of Digital
Archæoludology paper and the Ludii system paper, all linked in the analysis document.

## 2. Evaluation

### 2.1 The pair term's shape

`kPairWeight` was lowered, but `pairs` still counts every adjacency between own discs, so
the bonus grows quadratically with clumping while the mobility penalty grows linearly.
Nine discs in a solid block hold 12 pairs; the same nine dispersed hold none. Lowering the
weight rebalanced that without removing it, and the incentive returns the moment the board
gets crowded or the weight gets raised for another reason.

Two ways to fix the shape rather than the magnitude:

- Saturate: count each disc in at most one pair, so nine discs score at most 4 regardless
  of arrangement.
- Restrict: count only pairs on an axis that actually points at an enemy disc, which is
  what `Eval.h` already claims the term measures — "half of a pincer". A pair with nothing
  between it and empty space is not half of anything.

The second is closer to the term's stated intent and is the better change, but it costs a
scan per pair.

### 2.2 Weight calibration

`kThreatWeight` (0.25), `kPairWeight` (0.02), `kMobilityWeight` (0.02) and
`kCentreWeight` (0.05) are first guesses that have been nudged once. They have never been
fitted. With the instrumentation from §1 in place, a coarse sweep over one weight at a
time would be cheap and would replace the guessing.

Note that the weights do not mean the same thing under both `MoveStyle` values —
`openNeighbours` is on a roughly 3× larger scale under Slide. If both rule sets are to
stay playable, the weight set should eventually be per-style rather than shared.

### 2.3 Threats are counted, not valued  — ~~OPEN~~ DONE (Stage 8)

`threats` counted enemy discs one move from being custodially captured, each worth 0.25
regardless of whether anything could actually reach the completing square. `canOccupy()`
in Eval.cpp now requires a Free disc of the right colour with a clear line to it, under
the game's MoveStyle.

Still not checked, and deliberately: whether the completing move would self-capture, or
would repeat a position. Both need game state Eval does not hold. If the term ever needs
to be exact, the check belongs in Game, not here.

## 3. Search

### 3.1 Placement is searched without ordering

`Latrunculi::Game::moveOrderScore` returns 0 during placement, so alpha-beta gets no
ordering for the first 40 plies and iterative deepening reaches only two or three ply in a
second. Ranking placements by their positional delta — the same `Eval` terms the leaf
already computes — would deepen the opening search substantially for little work.

### 3.2 Per-node cost is now the binding constraint  — PARTLY DONE (Stage 8)

Flagged in Stage 2 as "optimise before heavy MCTS"; the slide made it current. Branching
went from roughly 4–9 destinations per disc to as many as 16 on the 8×10 board.

- ~~Incremental Zobrist hashing instead of hashing the full board per candidate.~~ DONE.
- ~~Reuse one scratch board across candidates rather than copying per call.~~ DONE — one
  scratch vector now spans the whole enumeration, so the copy stays but the allocation
  goes.
- STILL OPEN: hoist the reachability computation. `reachableMask` is recomputed per
  removal candidate even though the removal rarely changes the rays, and it allocates a
  `vector<bool>` per call. With R removal candidates and D discs that is R×D masks per
  enumeration where D would usually do.
- STILL OPEN: the candidate loop scans all `squares_` destinations per origin to test a
  mask that is mostly false. Iterating the rays directly would skip that.
- UNMEASURED: none of this has been profiled. The two items above are the obvious
  remaining costs by inspection, not by measurement, and inspection has been wrong before
  in this codebase.

### 3.3 MCTS — assessed and shelved

Superseded by `doc/2026-07-22-latrunculi-mcts-assessment.md`, which is now the single
place this lives: six identified defects, an ordered plan if it is ever taken up, and
Ben's decision on 2026-07-22 that the work is not worthwhile now.

The short version: the reward scale genuinely breaks UCB1 and blocks everything else, but
the deeper problem is the domain. The Pacific rule makes a rollout a slow, noisy
re-derivation of the material balance the rollout started from, so even a well-built MCTS
would be fighting the rules. Expect it to stay weaker than negamax on this game.

## 4. Rules still on the table

None of these are needed unless the games go dull again. In rough order of size:

- **Convex capture payoff** — ~~OPEN~~ DONE and MEASURED (Stage 10). Shipped as
  `PayoffStyle::ConvexMargin`, now the default. It helps, but LESS than this document
  predicted: with the slide it takes decisive finishes 42% -> 52% and the dead tail
  23.2 -> 18.4 plies; with step/leap it changes the final margin and nothing else,
  0/50 decisive either way. It was not "the highest leverage per line changed" — the
  movement rule was, by a wide margin. Full table in `doc/bench/README.md`.
- **Komi.** NEW, surfaced by the 2x2. Player A wins above chance in all four cells
  (34, 36, 42, 35 of 50; p ~ 0.003 for the mildest of them) and did at 300 ms too, so
  komi 0.5 is undersized. This predates every change made today. It is also the easiest
  thing here to settle numerically: sweep komi and pick the value that brings A/B closest
  to even. Note the convex payoff appears to AMPLIFY the bias where the game cannot be
  decided on the board (step+convex was the worst cell at 42/50), so komi should be
  re-checked against whatever payoff is finally chosen, not tuned once and forgotten.
- **Immediate removal** — the other half of the Kharebga ruleset, deliberately not taken
  so that the slide could be evaluated alone. Adopting it deletes the Bound state, the
  mandatory remove-then-move, `immobilizationDiscount` and the X-mark rendering, and
  changes the save format. It also makes Stage 3 moot.
- **Seneca → Piso** (recommendation 4). Same deletion of the Bound state, arrived at from
  the Locus Ludi rules rather than from Kharebga, and keeping step movement. Now largely
  redundant with the slide already in place; listed for completeness.
- **Stage 3, freeing chains** — the one unimplemented plan stage. Read the caveat in the
  plan document first: it adds a fourth brake on captures and works against liveliness.
  Fidelity to the Seneca variant is the only argument for it.

## 5. Known defects

- **Placement dead end.** ~~A placement position with no legal placement is not terminal
  and returns an empty move list.~~ DONE (Stage 8): the phase restriction on
  `checkImmobilizationTerminal` is gone and the driver's two silent paths are now
  diagnostics. One thing to confirm: treating "cannot place" as a LOSS for the side to
  move is a rules reading, not something `Latrunculi rules - BPW.md` states. It is the
  minimal reading consistent with the engine's own immobilisation rule, and the
  alternative — passing — would need placement-pass encoding and a super-ko interaction.
  Worth a decision rather than leaving it as my inference.
- **Removal proviso unchecked.** `isLegalMovement` verifies only that the removal target
  is a Bound enemy disc. The Seneca rule requires both flanking discs to still be free.
  Half of Stage 3, and cheap to fix independently of the cascade.
- **`Eval::slideDestinations` duplicates `Game::collectSlides`.** Two copies of the same
  blocking rule, in different translation units, with only a comment tying them together.
  `Eval::canOccupy` now walks the same rays a third time, in the opposite direction. If
  the slide's blocking behaviour ever changes, all three must change together. This has
  crossed the threshold where it should be one shared ray-walk.

<!-- Copyright Ben Paul Wise. All Rights Reserved. -->

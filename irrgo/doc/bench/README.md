<!-- Copyright Ben Paul Wise. All Rights Reserved. -->

# Latrunculi benchmark runs

Raw output from `latrunculi_bench`, kept so later runs have something to be compared
against. One file per batch; the filename records date, rule set, game count and time
budget. Metric definitions are in `latrunculi_game/GameStats.h`.

Not every experiment leaves its raw files here; the placement experiment below was
summarised and its ten 10-game data files discarded, since it is unlikely to be rerun.
The command line and seeds are recorded so it can be regenerated if that changes.

## 2026-08-25..27 — weight-tuning campaign, rounds 1-2

The first use of the A-vs-B machinery (`latrunculi_bench pairs=/wA./wB.` +
`tools/latrunculi-sweep.ps1`): can self-play matches fit the eleven `EvalWeights`
fields better than the hand-picked values? Protocol per the approved plan: a
per-round incumbent-vs-incumbent baseline sets the noise floor; a lenient coarse
filter (40 pairs per candidate, keep at >= 55% wins AND quiet share <= baseline + 5)
nominates; a 310-pair confirmation (620 games; promote at >= 331 wins AND quiet <=
baseline + 3 AND captures >= 0.9x baseline) decides. All matches 8x10x20, slide +
convex, komi 1.5, 500 ms/ply, colors mirrored within each seed pair. Raw data:
`sweeps/2026-08-24/` (round 1) and `sweeps/2026-08-26-round2/`.

**Result: no change ships.** `centre` 0.05 -> 0.2 was the campaign's single
confirmed promotion, and round 2 promoting nothing stopped the descent per
protocol — but the final size-robustness gate then rejected it: pooled 6x6 wins
198/400 against a pre-committed floor of 200. The defaults stand in full, now
validated rather than guessed.

| stage | candidates | outcome |
|---|---|---|
| round-1 coarse (x1/4..x4, 44) | 6 kept | best coarse figures did not survive |
| round-1 confirm (5 distinct) | centre 0.2: **342/620**; spearheadPairs 0.6: 332/620 | 2 promoted, 3 rejected |
| composed {centre, spearhead} | 384/620 + replication 389/620 | rejected: pooled quiet 77.5% > 75.5% |
| round-2 coarse (x0.7/x1.4, 22) | 5 kept | all between 55% and 60% |
| round-2 confirm (5) | best 329/620 | all rejected -> descent stops |
| size robustness, centre 0.2 | 6x6: 94/200; 8x10: 110/200; 10x12: 121/200 | 6x6 cell fails the >=50% rule |
| 6x6 tie-break replication | 104/200; pooled 198/400 vs floor 200 | **centre 0.2 rejected; 0.05 stands** |

Findings:

- **The liveliness guardrail is load-bearing, not decorative.** Three separate
  candidates won overwhelmingly and were refused for dulling the game: `mobility`
  0.005 (62.1% wins, quiet 81.5%), the composed centre+spearhead vector (62.3%
  pooled over 1,240 games, quiet 77.5%), and implicitly the spearhead half of that
  composition. In this rule set there is a standing exchange rate between win rate
  and turtling, and an unguarded sweep would have bought strength with exactly the
  dullness the last two months of rule work removed.
- **The composed rejection needed a replication to be honest.** Its first
  confirmation missed the quiet limit by 0.15 points against sampling noise of
  +/-1.7; a pre-committed second 620-game run at a fresh seed block came in at
  79.35%, settling it. One 100-minute replication converted a coin-flip verdict
  into a clear one.
- **Coarse figures regress hard.** `vulnerableAxes` nominated at 66.3% and 60.0% in
  the two rounds and confirmed at 53.1% and 50.0%. Forty pairs nominate; only 620
  games decide. No coarse number should ever be quoted as a result.
- **The hand-picked weights largely survived.** `threat` = 0.25 is sharply optimal
  (x2 -> 21% wins, x4 -> 1.3%); every placement-term default resisted all
  perturbations tried at both granularities. The campaign's value was as much
  validating those numbers as improving one.
- Why `centre` x4 helps is not measured, only plausible: with capture geometry
  priced by the other terms, a stronger centre pull concentrates discs where
  flanks and denials actually form instead of along the safe but inert rim.

- **The centre gain is monotone in board size, which is why it died.** 47.0% on
  6x6/12, 55.0% on 8x10/20 (independently replicating the 55.2% confirmation),
  60.5% on 10x12/30. The summed centrality term scales with disc count and board
  geometry, so one weight cannot mean the same thing on every board — the
  pre-written "suspect mobility/centre scaling" caveat, now with data. The
  principled follow-up is a size-normalized centrality term, recorded in
  `doc/2026-07-22-latrunculi-further-improvements.md` §2.4; re-tune centre only
  after that lands.
- **Komi 1.5 is board-size-dependent too.** In the two 6x6/12 robustness matches,
  the second player won ~73% of games regardless of weight set (the candidate went
  29/100 as P0 and 75/100 as P1 in the replication). Mirrored pairs cancelled it
  from every verdict above, but any single-color measurement on a small board is
  currently measuring komi as much as skill. Same lesson as the 2x2's komi
  finding: re-check komi per configuration, never assume it transfers.

The planned 1000 ms budget-sanity match was mooted by the rejection and not run.

## 2026-08-24 — placement heuristics: before/after

The engine gained a placement-phase evaluation (`PlacementEval.h`: seven terms derived
in `doc/2026-08-24-latrunculi-placement-heuristics.md`) wired into `staticEval` and,
for the first time, into `moveOrderScore` during placement — which previously returned
0 for every placement, leaving alpha-beta nothing to prune on for the first 40 plies.
This run measures that change and nothing else: same command, same seeds, before
(HEAD) and after (working tree). Raw outputs are the two
`2026-08-24-bench-placement-*.txt` files here.

10 games, 8x10x20, slide + convex, komi 1.5, policy placement, 1000 ms/ply, seed
777001, 8 threads.

| | before | after |
|---|---|---|
| mean opening depth | 3.43 | **3.60** |
| mean captures | 27.6 | 30.6 |
| quiet game | 9 (90%) | 9 (90%) |
| decisive (reduction) | 1 (10%) | 1 (10%) |
| mean dead tail | 36.0 | 36.0 |
| games with a lead change | 3/10 | 4/10 |
| mean peak margin | 4.4 | 3.4 |
| mean final margin | 3.7 | 2.3 |
| won by A / B | 5/5 | 3/7 |
| wall clock | 244 s | 308 s |

Findings:

- **Opening search got deeper, and the gain is robust.** 3.43 -> 3.60 over the same
  678 searched opening plies, and a second, independent 'after' run agreed to a
  hundredth (3.61). The gain comes despite each placement node getting more expensive
  — placement ordering now scores every candidate square — so ordering paid for its
  own cost. This settles the question the change was made for, and leaves no case for
  the deferred search-width cap.
- **Capture activity edges up** (27.6 -> 30.6, and 30.8 in the second after-run):
  placements now build the structures captures feed on.
- **The quiet-game share is untouched.** 9/10 both sides, dead tail 36.0 both sides.
  The placement heuristics alone, at untuned first-guess weights, do not fix the
  movement-phase stall — as expected; that is what the weight-tuning campaign is for.

Caveats:

- **A first 'after' run was invalid and briefly looked spectacular.** The machine
  hibernated ~80 minutes mid-run; `steady_clock` advanced through it, so the plies in
  flight had their budgets destroyed and completed at trivial depth. That run showed
  6/10 reductions, dead tail 16.0, quiet share 40% — retracted in full once a clean
  rerun reproduced the before-run's 9/10 quiet endings. The lesson is quantitative: a
  handful of depth-1 plies flipped half the outcomes at n=10. Clock noise is the
  dominant noise source in these benches, and n=10 outcome shares should never be
  read as signal.
- The margin and A/B rows differ between the columns, but at n=10 they are noise (see
  above, forcefully).

Regenerate: build at the respective revision, then

    latrunculi_bench games=10 ms=1000 seed=777001 threads=8

(the after-revision also reseeds the move-generation scan order from the game seed —
`Game::reseedScanOrder` — so its runs are seed-reproducible in a way the before
revision's were not; see the reproducibility note in bench.cpp).

## 2026-07-22 (evening) — does searched placement create the first-player bias?

The 2x2 below left player A winning above chance in every cell, unrelated to any change
made that day. This experiment isolates the cause. `latrunculi_bench` gained a
`placement=policy|random` option: `policy` is the shared PlacementPolicy (each side plays
one random opening placement, then runs of searched ones); `random` makes every placement
random for both sides, so neither gains anything from SEARCHING the opening. If the bias
comes from player 0 optimising its first placements onto a nearly empty board, it should
vanish under `random`; if it is inherent first-move tempo, it should survive.

50 games per condition (five 10-game runs, seeds 777001 + 10*i so the two conditions are
paired game-for-game), 8x10x20, slide + convex, **komi 1.5**, 1000 ms/ply.

| | policy (searched opening) | random opening |
|---|---|---|
| overall A/B | **33 / 17** | **25 / 25** |
| decisive (reduction) | 20 (40%) | 13 (26%) |
| — A/B among decisive | 17 / 3 | 9 / 4 |
| quiet game | 30 (60%) | 37 (74%) |
| — A/B among quiet | 16 / 14 | 16 / 21 |
| mean captures | 29.6 | 23.7 |
| games with a lead change | 18 / 50 | 25 / 50 |

Findings:

- **The first-player advantage is created by searching the opening, not by moving first.**
  Searched placement gives A 33/17 (two-sided binomial p ~ 0.02); random placement gives
  25/25, dead even. Player 0's edge is that it optimises its opening structure on an
  emptier board, and it disappears when neither side searches the opening.
- **Random placement fixes fairness by removing what makes the game good.** Decisive
  finishes fall 40% -> 26% and captures 30 -> 24: the searched opening builds the
  structure later captures feed on. Fair opening OR lively opening, not both for free.
- **Komi 1.5 is well-sized for the quiet-game tiebreak.** Combined across both conditions,
  quiet games are 32/35 -- near even. The direction flips by condition (policy 16/14,
  random 16/21), both within noise. No reason to change it. This is the first confirmation
  from live play, rather than replayed records, that 1.5 does what it was chosen to do.

Caveats: the decisive subset still leans A under random placement (9/4), hinting at a
small residual first-move edge, but that is 13 games (p ~ 0.16) -- indistinguishable from
noise here. And random openings produce MORE lead changes (25 vs 18) despite fewer
decisive games, so activity and drama again diverge, as they did at 300 ms.

Design read (not acted on): do NOT switch to random placement to chase fairness -- it
costs exactly the game quality the slide bought. The searched-placement edge (66% for A)
is within the range komi is meant to cover for material-decided games; it cannot touch
decisive games, and no scalar can. If the decisive tilt matters later, the lever is the
placement phase itself -- mirror player 1's opening search, or give player 1 a small
structural compensation -- not komi and not random openings.

Regenerate with, for i in 0..4 and both placement values:

    latrunculi_bench games=10 ms=1000 seed=$((777001+10*i)) style=slide payoff=convex komi=1.5 placement=policy

Every batch prints its base seed, and game `g` uses `base + g`, so any single row can be
reproduced on its own:

```
latrunculi_bench <games> <msPerMove> <seed> <style> [rows] [columns] [perSide]
```

## 2026-07-22 (later) — the 2x2: movement rule x payoff

Fifty games per cell, **the same fifty seeds in every cell**, 8x10, 20 discs a side,
1000 ms per searched ply. The two factors are `MoveStyle` (slide / step+leap) and
`PayoffStyle` (ConvexMargin / Gradient). Everything else is identical.

| | slide+convex | slide+gradient | step+convex | step+gradient |
|---|---|---|---|---|
| **decisive (reduction)** | **26 (52%)** | 21 (42%) | 0 | 0 |
| immobilization | 1 (2%) | 0 | 0 | 0 |
| quiet game | 23 (46%) | 29 (58%) | 50 (100%) | 50 (100%) |
| mean plies | 189.6 | 182.9 | 119.8 | 125.2 |
| mean captures | 29.7 | 29.0 | 10.9 | 11.5 |
| mean dead tail | **18.4** | 23.2 | 40.0 | 40.0 |
| mean longest quiet run | 32.0 | 33.3 | 40.0 | 40.0 |
| lead changes (games with any) | 22/50 | 22/50 | 13/50 | 12/50 |
| mean peak margin | 5.2 | 4.2 | 2.5 | 2.7 |
| mean final margin | **4.9** | 3.6 | 2.2 | 1.8 |
| won by A / B | 34/16 | 36/14 | 42/8 | 35/15 |

### What the design is

Each factor is isolated because the other is held fixed and the seeds are paired. Read
across a row for the movement effect, down a column for the payoff effect.

### Movement rule: the whole difference

Under **either** payoff, step/leap produced **0 decisive finishes in 100 games** and a
dead tail of exactly `pacificMoveLimit` every single time. The slide produces 42-52%.
This is not a matter of degree: the step move makes a defensive mass unreachable, so no
reward structure can make breaking it worthwhile. Captures roughly triple (29 vs 11) and
lead changes nearly double (22/50 vs 12-13/50).

### Payoff: a real but secondary effect, and only where it has something to work with

Holding the slide fixed, convex raises decisive finishes 42% -> 52%, cuts the dead tail
23.2 -> 18.4 plies, and raises the final margin 3.6 -> 4.9 and peak margin 4.2 -> 5.2.
The leader presses, exactly as intended.

Holding step/leap fixed, convex changes the final margin 1.8 -> 2.2 and nothing else that
matters: still 0/50 decisive. The incentive landed; there was no move available to act on
it. **The payoff multiplies the slide rather than substituting for it.**

### First-player advantage: a pre-existing problem this surfaced

Player A wins 34/50, 36/50, 42/50, 35/50 — every cell above chance, and the 300 ms runs
agreed (14/20, 13/20). Under a fair coin, 35/50 alone is p ~ 0.003. **Komi 0.5 is
undersized**, and this has nothing to do with today's changes; the harness simply made it
visible. Note the step+convex cell at 42/50 is the worst of the four, so a convex payoff
appears to amplify the first-move advantage where the game cannot be decided on the board.

### Caveats

- 50 games per cell. The 42% vs 52% decisive-rate difference is roughly one standard
  error and should NOT be treated as established; the 0% vs ~47% movement effect is far
  beyond sampling noise.
- 1000 ms per ply is still a weak engine next to the 90 s Ben plays in the GUI.
- Captures FELL between the 300 ms and 1000 ms runs (14.8 -> 11.5 for step+gradient), so a
  stronger engine plays more cautiously. Activity counts are budget-dependent and are not
  comparable across budgets.
- Lead changes at 300 ms looked flat between rule sets; at 1000 ms they clearly are not.
  The earlier reading was an artifact of the weak budget.

## 2026-07-22 (earlier) — slide vs step/leap, first measured comparison

Twenty games per rule set, **the same twenty seeds**, same 8x10 board, 20 discs a side,
same evaluation, 300 ms per searched ply. The only difference between the two batches is
`MoveStyle`. Engine state: Stage 8 (Zobrist hashing, threats legality filter, placement
dead-end fix), `kPairWeight` = 0.02, style-aware mobility.

| | slide | step/leap |
|---|---|---|
| decisive finishes (reduction) | **7 / 20 (35%)** | 0 / 20 (0%) |
| ended by quiet game | 13 / 20 (65%) | 20 / 20 (100%) |
| mean captures | **29.9** | 14.8 |
| mean removals | 31.2 | 15.9 |
| mean dead tail (plies after the last capture) | **26.0** | 40.0 |
| mean longest quiet run | 34.2 | 40.0 |
| mean plies | 171.8 | 143.7 |
| mean lead changes | 1.2 (10/20 games) | 0.9 (9/20) |
| mean peak margin | 3.8 | 2.4 |
| mean final margin (absolute) | 3.3 | 1.8 |
| wall clock | 951 s | 784 s |

Reading:

- **Step/leap never once produced a decisive finish.** Twenty games, twenty quiet-game
  terminations, and a dead tail of exactly 40 plies — `pacificMoveLimit` — in every one.
  The original complaint that started this thread is not a matter of taste; it is what
  the rule set does, every time.
- **The slide roughly doubles capture activity** (29.9 against 14.8) and converts a third
  of games into real conclusions.
- Two thirds of slide games still end on the quiet-game rule, so the drift is reduced,
  not eliminated. This is what the convex-payoff item in
  `doc/2026-07-22-latrunculi-further-improvements.md` §4 is aimed at.
- Lead changes barely move (1.2 vs 0.9), so the slide makes games more *active* without
  making them more *contested*. Games are still largely decided early and then confirmed.

Caveats, which matter for anything built on these numbers:

- **300 ms per ply is a weak engine.** Ben plays the GUI at 90 s. Both batches are
  equally weak, so the comparison between them holds, but the absolute figures are not
  what a strong engine would produce, and the drift could look different at depth.
- Twenty games is a small sample. The reduction rate of 35% has a wide interval on n=20.
- Player A won 13/20 (slide) and 14/20 (step/leap). Komi is meant to offset the
  first-move advantage; whether 0.5 is the right value is now a measurable question and
  has not been measured.

<!-- Copyright Ben Paul Wise. All Rights Reserved. -->

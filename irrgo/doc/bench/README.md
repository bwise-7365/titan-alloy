<!-- Copyright Ben Paul Wise. All Rights Reserved. -->

# Latrunculi benchmark runs

Raw output from `latrunculi_bench`, kept so later runs have something to be compared
against. One file per batch; the filename records date, rule set, game count and time
budget. Metric definitions are in `latrunculi_game/GameStats.h`.

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

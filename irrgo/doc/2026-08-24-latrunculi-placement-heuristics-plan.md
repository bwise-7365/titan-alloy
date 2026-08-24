# Latrunculi placement heuristics: implementation, weight tuning, movement reuse

## Context

The placement phase (~40 plies on the default 8x10) gives search nothing to work
with: `Game::moveOrderScore` returns 0 for every placement (Game.cpp:886-888),
and the negamax searcher fully stable-sorts each node's legal moves on that int
(absgame/negamax.cpp:20-39), so up to ~140 placements per node are searched in
raw enumeration order and iterative deepening completes only 2-3 ply. The
analysis in `doc/2026-08-24-latrunculi-placement-heuristics.md` (facts F1-F11,
heuristics H1-H8, penalties A1-A6) derived what placement should optimize. This
plan implements those heuristics (Stage A), builds the machinery to fit their
weights by self-play (Stage B), runs the tuning campaign (Stage C), and stages
movement-phase reuse as a measured follow-up (Stage D). Decisions confirmed
with Ben: no searcher width cap yet (defer until bench depth data says
otherwise), and movement reuse lands separately from placement so improvements
stay attributable.

Verified integration facts the plan relies on:
- Ordering is the only per-move hook the searcher consumes (no top-K, no
  killer/history/TT). Root PV rotation between iterations; an incomplete
  iteration falls back to the best-ordered root move, so placement ordering
  also fixes the shallow-search fallback.
- `staticEval` = material diff + positional(me) - positional(opp); during
  placement material is uniform across siblings, so only positional terms
  discriminate.
- Eval weights are `constexpr` in an anonymous namespace (Eval.cpp:18-34):
  today a sweep means a recompile per point.
- bench.cpp has a key=value CLI with deterministic seed = base + g independent
  of thread count; GameStats already computes the guardrail metrics. BUT
  `initScanOrder` seeds from the clock (Game.cpp:213-220), so equal-score
  tie-breaks differ run to run — bench's reproducibility claim is currently
  false. Stage B fixes this.
- GUI, selfplay, and bench all funnel into `Searcher::bestMove` and share
  `PlacementPolicy`; changes land identically in all three. GUI and selfplay
  compile unchanged throughout (weights default to current behavior).

## Stage A — placement heuristics implementation

### A1. New pure eval terms: `PlacementEval.h` / `PlacementEval.cpp` (new files)

Promote the shared helpers `halfPinCompletion`, `canOccupy`, `isThreatened`
(currently file-local in Eval.cpp) to declarations in Eval.h so PlacementEval
reuses them — no duplicated ray walks (the improvements doc flags duplication
as a defect).

`struct PlacementTerms` + `placementTerms(cells, rows, columns, player, style)`:
- `vulnerableAxes` (F1-F3): per own Free disc, axes where both opposite squares
  are on-board and not friendly-blocked. Penalty.
- `oneMoveCapturable` (A1): own Free discs the enemy can flank-complete right
  now — `isThreatened` with colors swapped. Penalty, ramped by progress.
- `spearheadPairs` (F4): own pairs whose axis extension hits an enemy Free disc
  with an empty on-board square beyond. Bonus.
- `diagonalSupport` (F6): own diagonal adjacencies with both notch squares
  empty. Bonus.
- `deniedSquares` (F5): empty squares flanked by two own Free discs. Bonus.
- `strikers` (H6): own Free discs with >= 2 open rays (length >= 2 under
  Slide; >= 2 empty neighbours under StepLeap); bonus saturating at
  ~perSide/5.
- `notchExposure` (A2/F7): own Free discs adjacent to >= 2 enemy Free discs on
  different axes. Penalty.

`placementScore(terms, progress, weights)`: linear blend in disc units;
`oneMoveCapturable` weight ramps steeply as `progress -> 1` (the seam, F10).
`progress = placed/perSide`, passed by the caller. Initial weights are first
guesses commented as such; Stage C fits them.

### A2. Wire into Game (Game.h / Game.cpp)

- `staticEval`: during Placement add `placementScore(me) - placementScore(opp)`
  to the positional part.
- `moveOrderScore`: during Placement return the *local delta* of
  placementScore for placing on that square (terms touched within radius 2
  plus canOccupy rays — never a full-board rescan per candidate), scaled by
  `kPlacementOrderScale = 1000` via `std::lround`. No collision with the
  movement branch's 10000/10/5 scale: siblings are always one phase; say so in
  the comment.
- `PlacementPolicy` unchanged.

### A3. Tests (new target `latrunculi_test`, plain CTest like palette_core)

One fixture per diagram in the heuristics doc (edge wall, 2x2, gapped pair,
diagonal notch, spearhead, corner half-pin, seam one-mover): hand-built boards
asserting each term's exact count. Plus a property test: local ordering delta
== full-board recompute difference, over randomized boards/squares/styles.

## Stage B — runtime weights + A-vs-B bench (tuning machinery)

Design settled; details below are the plan of record.

### B1. `EvalWeights` (Eval.h / Eval.cpp)

Struct with the 4 existing fields (`threat, pair, mobility, centre`, defaults =
current constants, justification comments migrate to the struct) plus the 7
placement fields. `validateEvalWeights` throws on non-finite values (negative
allowed — sweeps probe sign flips); follows the validateKomi precedent.
`positionalScore(terms, weights = EvalWeights{})` and
`placementScore(terms, progress, weights = EvalWeights{})` — defaulted
parameter, one body, existing callers compile unchanged. Delete the
anonymous-namespace constants.

### B2. Game plumbing (Game.h / Game.cpp)

- `EvalWeights evalWeights_{}` member; `evalWeights()` accessor;
  `setEvalWeights()` validates then stores. Settable anytime, mid-game
  included: weights touch only staticEval/moveOrderScore, never legality,
  hashing, super-ko, or terminal state — the header comment states this as a
  deliberate exception to construction-time immutability. `clone()` needs no
  change (default copy ctor propagates weights, which gives each search tree
  the searching side's weights — the standard self-play convention).
- `reseedScanOrder(std::uint64_t seed)`: public, rebuilds `scanOrder_`
  deterministically. Fixes the reproducibility bug; scan order affects only
  tie-break enumeration, never legality or hashing.

### B3. bench A-vs-B mode (bench.cpp)

- New keys: `pairs=` (turns A/B mode on; mutually exclusive with `games=`,
  throw on both), `wA.<field>=` / `wB.<field>=` (unknown field throws;
  unspecified fields keep defaults so candidate-vs-incumbent needs no wB.*;
  wA./wB. without pairs= throws), `csv=<path>` (append one machine-readable
  summary line; header written on first use).
- Scheduling: N = 2*pairs games through the existing atomic counter;
  `pair = g/2`, `flip = g%2`; seed = base + pair (both games of a pair share
  the PlacementPolicy seed); flip decides which set holds player 0. Call
  `reseedScanOrder(seed)` after construction so pair-mates break ties
  identically. Thread-count independence preserved (everything a game uses is
  a function of g).
- Per ply: `game.setEvalWeights(weightsFor(game.currentPlayer(), flip))` before
  `Searcher::bestMove` — no clones needed; the search clones inherit the
  mover's weights.
- Reporting: two extra per-game columns (`AasP`, `wSet`) appended bench-side
  (GameStats untouched); summary block with set-A/set-B wins, per-color
  breakdown, quiet-game share per winning set (the turtle tell), and the
  printed significance threshold `ceil(N/2 + 1.6449*sqrt(N/4))`. CSV line
  carries full wA/wB vectors so the file is self-describing.
- Fix the bench reproducibility comment (bench.cpp:44-50).

### B4. Sweep driver: `tools/latrunculi-sweep.ps1` (new)

Windows PowerShell 5.1-safe wrapper (repo precedent: tools/merge-ico.ps1).
Parameters: `-Stage baseline|coarse|confirm|robust`, `-Bench <exe>`,
`-Incumbent <.psd1 weight table>`, `-Threads`, `-OutDir` (default
`doc\bench\sweeps\<date>\`). Composes wA./wB. args, invokes bench with csv=,
reads the CSV back, applies the stage's filter/promotion thresholds. Experiment
policy lives in the script; bench stays a measurement tool.

## Stage C — the tuning campaign (how good weights get determined)

Throughput basis: ~22-26 s/game single-thread at ms=200; ~1,700-2,000
games/hour at 12 threads. Binomial sizing: distinguishing 55% from 50% at
one-sided alpha=0.05, power 80% needs ~620 games (310 mirrored pairs);
promotion threshold at 620 games is >= 331 wins.

- **Stage 0 — baseline** (per round): incumbent vs incumbent, pairs=100.
  Records the noise floor: quiet-game %, mean captures, lead changes, residual
  color imbalance. All guardrails are relative to this.
- **Stage 1 — coarse coordinate filter**: each of the ~12 weights at
  multipliers {x1/4, x1/2, x2, x4}; ~48 candidates at pairs=40 (~2-2.5 h
  total). Keep iff win rate >= 55% AND quiet-game share <= baseline + 5 pts.
- **Stage 2 — confirmation**: each survivor vs incumbent at pairs=310.
  Promote iff wins >= 331/620 AND quiet share <= baseline + 3 pts AND mean
  captures >= 0.9x baseline. Survivors touching different weights are composed
  into one vector and re-confirmed once (effects are not additive); same
  weight -> take the best.
- **Stage 3 — iterate**: promoted vector becomes incumbent; repeat Stage 1
  with narrower multipliers {x0.7, x1.4}. Stop when a round promotes nothing
  or after 3 rounds (beyond that is fitting noise at these game counts).
- **SPSA**: skipped for round 1 — needs thousands of games and yields a vector
  with no per-weight evidence, which fights this codebase's justified-comment
  style. Revisit only if coordinate descent stalls on interactions.
- **Robustness (final)**: promoted vector vs original incumbent at 6x6/12,
  8x10/20, 10x12/30, pairs=100 each; require >= 50% on every size and pooled
  significance. A size-dependent failure points at `mobility`/`centre`, whose
  raw terms scale with board dimensions.
- Fixed throughout: komi=1.5, style=slide, payoff=convex, placement=policy,
  ms=200. StepLeap gets its own weight set later or never.

## Stage D — movement-phase reuse (separate follow-up, after C's baseline)

Confirmed: lands separately so strength/liveliness changes stay attributable.
- `spearheadPairs` replaces the raw `pairs` count in the movement positional
  terms — this IS the improvements-doc 2.1 "restrict" fix, shipped as a shared
  term.
- Quiet-move ordering: movement `moveOrderScore` currently gives all quiet
  moves 0 under the 10/5 capture scale; add the positional delta as a sub-unit
  tie-break so quiet maneuvering (where blobbing lives) gets ordered.
- The me-minus-opp structure already prices my en-prise discs via the
  opponent's `threats`; runtime weights make asymmetric defense/offense
  weighting possible later. `deniedSquares`/`strikers` stay meaningful in
  movement; per-phase weight values, same terms.
- Each change gets its own bench before/after at fixed seeds.

## Deferred (explicitly out of scope)

- Top-K search-width cap (AbsGame API change): only if bench's opening-window
  `depth` column still shows shallow search after Stage A. Confirmed deferral.
- MCTS rollout changes (MCTS shelved by prior decision).
- GameXml extension for A/B color mapping (derivable from flip = g%2).

## File changes (ordered)

1. `latrunculi_game/Eval.h` — expose shared helpers; `EvalWeights`;
   `validateEvalWeights`; weighted signatures.
2. `latrunculi_game/Eval.cpp` — delete constexpr weights; blend from
   parameter; implement validation.
3. `latrunculi_game/PlacementEval.h` / `.cpp` (new) — PlacementTerms,
   placementTerms, placementScore, local-delta scorer.
4. `latrunculi_game/Game.h` / `Game.cpp` — evalWeights_ + setter/accessor;
   reseedScanOrder; staticEval and moveOrderScore placement wiring.
5. `latrunculi_game/CMakeLists.txt` — add PlacementEval sources +
   `latrunculi_test` CTest target.
6. `latrunculi_game/test_placement_eval.cpp` (new) — diagram fixtures +
   delta property test.
7. `latrunculi_game/bench.cpp` — pairs/wA./wB./csv keys, mirroring,
   per-ply weight flip, ab reporting, comment fix.
8. `tools/latrunculi-sweep.ps1` (new) — stage driver.

GUI, selfplay.cpp, GameStats.*, GameXml.*, absgame: untouched.

## Verification

- `latrunculi_test` green (Ben runs builds: hand off
  `./cmake-build-debug/latrunculi_game/latrunculi_test.exe` style commands).
- bench before/after Stage A on identical seeds: opening-window `depth` column
  rises; quiet-game share does not.
- B: `pairs=2 wA.threat=0.5 csv=...` smoke run — mirrored seeds confirmed in
  the table, CSV line well-formed; same-seed rerun reproduces game rows
  (reseedScanOrder fix verified).
- C: incumbent-vs-incumbent sanity (win rate ~50% within threshold); tuned
  vector promoted only through the stated gates.

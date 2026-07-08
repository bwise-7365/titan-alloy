<!-- Copyright Ben Paul Wise. All Rights Reserved. -->
# Fleet solver performance: diagnosis and fix plan (2026-07-08)

Status board for the fleet-solver performance work. Written 2026-07-08 after
the user reported that the fleet_viewer default problem (banded laydown 1,
limited fleet) ran about one hour in a Release build before he stopped it.
This file is expected to become obsolete once the FN track below closes; it
is dated so that its claims are read as claims about the code of 2026-07-08.

## 1. The problem

Observed: fleet_viewer, default profile (70 nodes = 20 supply-only / 20 both /
30 demand-only / 0 transit; 4 asset types x 3 vehicle types from the fixed
catalogs; seed 20260704), banded laydown 1, limited fleet. The "Optimal Fleet
Plan" solve ran about one hour in Release without finishing.

Required: the intended production problems are roughly 200-250 nodes, 10-15
vehicle types, and 10-15 asset types. The gap between the observed hour at
70 x 4 x 3 and an acceptable time at that scale is several orders of
magnitude, so parameter tuning cannot close it; the fix must change the
per-iteration linear algebra and remove the screen/certificate re-solves.

## 2. What runs today

The viewer calls `solveFleetPlan(inst, FleetSolveParams{})`
(gui/fleetmainwindow.cpp:606). The defaults (fleetsolve.hpp):

- engine `"ipm"`: `mehrotraIpm` with an EMPTY `NewtonSolverFactory`
  (fleetsolve.cpp:173-176), i.e. the generic dense-LU factory. Each IPM
  iteration factors the full Newton matrix at cost dim^3. The structured
  factory that solved the single-commodity banded case was deliberately not
  ported; the ledger records it as future work G5h, with sparse machinery as
  G5i (network/plan.md).
- per-asset screen `maxSourcesPerSink = 6` plus a certificate loop of up to
  10 rounds; every round is a complete cold-start IPM solve (the IPM cannot
  warm-start) at a dimension that only grows.
- `lcp.M` is a dense `MatrixXd` (fleetlcp.cpp:136).

## 3. Diagnosis

Dimension arithmetic for the default profile (asset presence 0.8): each asset
has about 32 sources and 40 sinks; a kept pair enters with all capable
vehicle types at once, so round 0 has about 40 x 6 x 3 x 4 = 2,880
y-variables (~3,000 total with mu and lambda), and keep-all is about 15,500
(dense M at that size is ~1.9 GB).

Two effects compound, and both were measured on the single-commodity
200-banded case in early July (network/doc/performance.md P1-P9):

1. Banded geometry defeats the count screen. Near-tied round-trip costs
   leave the excluded pairs at near-zero reduced cost, so each certificate
   round admits another large installment of them; the kept set climbs from
   ~3k toward keep-all over a few rounds, and every round pays a full IPM
   solve.
2. Dense LU is cubic in that growing dimension. Round 0 at dim ~3k costs
   about 1-3 s per factorization, ~35 iterations, on the order of a minute.
   At dim ~10k the measured single-commodity datum is ~50 s per iteration,
   i.e. ~30 minutes per round. One ballooned round plus round 0 accounts for
   the observed hour in Release.

The interior-point choice itself is working as intended. Degenerate flat
optimal faces stall projection-contraction methods (contraction rate near
1); the central path is insensitive to them, and the measured iteration
counts confirm it (35 iterations at dim 10,838 and 36 at dim 14,601 on the
single-commodity case). The cost is per-iteration linear algebra times
certificate re-solves, not iteration count.

A note on sparse-matrix libraries: applied to the current Newton matrix they
do not help, because each demand cell contributes a DENSE rank-one block
(Q_cell times the all-ones matrix over that cell's variables). Sparsity in
the assembled matrix appears only after a reformulation that introduces the
cell total as an explicit variable. The alternative, taken here, is to
exploit the rank-one structure analytically (Sherman-Morrison), which is the
proven single-commodity route (NS2/NS3: ~394x, and the keep-all answer was
0.4% better than the screened one because nothing was excluded).

## 4. FP0 — the bounded probe (confirm attribution before building)

Instrument the pipeline and measure one bounded run, per the IP4 protocol.
Instrumentation (behavior unchanged when the hooks are empty):

- `FleetSolveParams` gains `logger` (the shared `IterationLogger`, handed to
  the engine so `iterFreq` produces visible, flushed heartbeats) and two
  per-round hooks: `roundStartLogger(round, keptPairs, dim)` fired after
  assembly and before the solve (so a killed run still reports the ballooned
  dimension), and `roundEndLogger(round, vi, milliseconds)` fired after the
  solve.
- New plain executable `fleet_benchmark` (network/src/fleetbenchmark.cpp,
  pattern of network_benchmark): builds the exact viewer default profile,
  prints the instance shape (per-asset sources/sinks, capable types,
  keep-all pair count), runs `solveFleetPlan` with heartbeats at iterFreq 1,
  and prints one row per certificate round. Positional arguments override
  laydown, certificate-round cap, screen size, catalog sizes, and seed.

Probe run (Release): `fleet_benchmark 1 1 6 4 3 20260704` — banded, screen 6,
certificate rounds capped at 1. Round 1 may be killed once a few heartbeats
have shown the per-iteration cost; the flushed round-start line already
carries the ballooned dimension.

Falsifiable predictions:

- P1: round 0 converges in 25-45 IPM iterations.
- P2: per-iteration wall time scales as dim^3 (against the measured
  single-commodity datum: ~50 s at dim 10,838).
- P3: the first certificate check admits a large fraction of the excluded
  pairs; dimension at round 1 at least doubles.
- P4: iteration count stays roughly flat as the dimension grows
  (degeneracy-insensitivity of the central path).

If P1-P4 hold, the plan below proceeds. If instead the iteration count
explodes, the diagnosis is wrong and the plan stops for review.

## 5. The fix: FN track (structured factory + matrix-free M)

- **FN1 — Maxima check first** (`network/doc/fleet-newton-check.mac`,
  mirroring ns2-newton-check.mac). The fleet Newton matrix
  K = M + diag(sOverY) has, per demand cell (sink, asset), the block
  Q_cell * ones + diag(Dt) — invertible per cell in O(cell size) by
  Sherman-Morrison — and thin skew borders: one mu column per (source,
  asset) supply cell (one nonzero per variable) and K budget columns
  (entries rho_p). Eliminating the y-block gives a Schur complement of size
  numSupplyCells + K (~131 for the default profile, ~1.2k at target scale);
  the skew signs cancel as in the single-commodity case, so the Schur
  matrix should be SPD (LLT). The .mac verifies: the per-cell inverse, the
  two-part Schur assembly (diagonal accumulation O(numVars) plus per-cell
  rank-one subtraction O(support^2), support = distinct sources + distinct
  types in the cell), SPD, recipe-equals-direct-solve on an exact-rational
  small shape, and the cell-size-1 edge case.
- **FN2 — `makeFleetNewtonFactory(lcp)`**
  (network/include/fleetnewton.hpp + lib), transcribing the FN1 algebra;
  `FleetSolveParams` gains `ipmNewton = "dense" | "fleet"` (default stays
  "dense" until FN3 certifies the flip). Parity tests mirror
  flownewton_test: backward-error bars on nondimensionalized fixtures,
  end-to-end VIResult parity against the dense factory, cell-size-1 and
  extreme-Q edge cases.
- **MF1 — matrix-free M.** At target scale keep-all is ~2 million
  y-variables; dense M is impossible (tens of TB) and even the screened
  problem (~100k+ dimension) is far beyond dense LU, so this stage is
  required regardless of FN2. `mehrotraIpm` gains an overload taking an
  apply-M operator (residuals through the operator; a Newton factory is
  then required since the generic dense factory has no matrix;
  newtonCheckTol checks through the operator). The existing dense path
  must remain bit-for-bit unchanged (the NS1 acceptance standard).
  `FleetLcp` gains an O(numVars) structure-based apply and an assembly mode
  that skips the dense M entirely.
- **FN3 — keep-all benchmark and default flip.** fleet_benchmark keep-all
  (screen 0) banded run under ipmNewton "fleet" + matrix-free M; compare
  against the FP0 datum. Expected shape, by the NS3 precedent: no screen,
  no certificate rounds, ~35 iterations, seconds at 70 nodes; then the
  fleet production default becomes keep-all + fleet factory, and
  performance.md gains the addendum. Ledger: G5h closes; G5i stays open
  for the full link-coupled model.

Deferred alternatives, recorded with reasons: sparse reformulation + sparse
LDLT (G5i) is the general route and the full link-coupled fleet QP will
need it, but it is slower to build and slower to run than the analytic
factorization on the current conservative model; OSQP (ADMM) is a
first-order method and degenerate banded faces are precisely where
first-order tails are slow, it needs the same reformulation to see
sparsity, and it adds a dependency; screen/parameter tuning is measured to
be insufficient on banded geometry (performance.md P4).

## 6. Gates

| Gate | Content | Status |
|------|---------|--------|
| FP0 | Plan doc; round/iteration hooks + hook test; fleet_benchmark; Ben's Release probe run vs P1-P4 | DONE 2026-07-08: all four predictions confirmed (rounds 0/1 = 24/32 iters; 0.95 vs ~64 s/iter = dim^3 to a few percent; balloon 4.2x to 81% of keep-all in one round; certified NO at cap 1; full run 2,070 s; all three budgets bind) |
| FN1 | fleet-newton-check.mac all checks pass | done 2026-07-08 (ALL 9 CHECKS PASS, Maxima 5.49) |
| FN2 | fleet factory + parity tests green | DONE 2026-07-08: NetworkFleetNewton 6/6, full ^Network 103/103 |
| MF1 | matrix-free overload; dense path bit-for-bit; fleet no-dense-M mode | DONE 2026-07-08: 182/182 green (bit-for-bit gate passed); keep-all banded 0.1 s wall (~2.5 ms/iteration at dim 14,579), identical iterations and objective. Full ladder: 90+ min projected -> 2,070 s -> 6.1 s -> 0.1 s. performance.md P14 |
| FN3 | keep-all banded benchmark; default flip; performance.md addendum; ledger update | DONE 2026-07-08: dim 14,579 in 39 iters / 6.1 s / certified / 3.0e-15, shortfall 65.386 vs screened 65.568; defaults flipped (ipmNewton fleet + keep-all); performance.md P10-P13; G5h closed. Report Part III case study recorded as a pending task per Ben |

Review stops after FP0 (probe results + token usage), after FN2 (parity
green), and after FN3 (benchmark + default flip), with additional stops on
any surprise. Each stage is sized so a stop loses at most one stage of work.

<!-- Copyright Ben Paul Wise. All Rights Reserved. -->

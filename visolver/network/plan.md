<!-- Copyright Ben Paul Wise. All Rights Reserved. -->
# Flow-planning solver: project plan and progress ledger

This is the working ledger for the network flow-planning problem
(`../flow-planning-problem.txt`). The PERT chart is `pert.puml`; this file is
the authoritative status record. **Protocol:** work proceeds one task (box) at
a time; after each task completes, work PAUSES for user review (progress +
usage budget) before the next box starts. Any session resuming this project
starts by reading this file.

## Problem in one paragraph

Central planner on M nodes (20-200) chooses supplies `S_i` in `[0, C_i]`,
resupplies `R_i >= 0`, and arc flows `f_ij >= 0` to minimize weighted squared
fractional shortfall `sum_i P_i ((D_i - R_i)/D_i)^2` subject to per-node flow
balance `sum_i f_ij + S_j = R_j + sum_k f_jk` and a global ton-mile budget
`sum_ij c_ij f_ij <= L`. Costs `c_ij > 0` are asymmetric, roughly
distance-like; `L` is calibrated to ~80% of a greedy notional plan's usage.
Convex QP; unique optimal `R*`; flows possibly degenerate.

## Agreed approach

1. **Reduce before solving.** All-pairs shortest distances `d_mn`
   (Floyd-Warshall). Lemma (to be proved, task A3): some optimal plan routes
   only along shortest paths from supply nodes (`C>0`) to demand nodes
   (`D>0`), so it suffices to optimize direct shipments `t_mn` over
   source-sink pairs. Links with `c_ij > d_ij` are provably unused;
   epsilon-optimality bounds prune source-sink pairs further. 70-node example:
   ~2,000 vars instead of 5,040.
2. **Solve the KKT system as an affine mixed LCP with existing VINCP tools.**
   Quadratic objective + linear constraints => KKT is affine in
   `z = (x, y)`; monotone (convex QP). One `bsHe94b` call (factors `(M+I)`
   once); no Josephy-Newton outer loop. Matrix generator packs the reduced
   problem into `(M, q)` + mixed projector; unpacker expands `z` back to
   `(S, R, f)` on the full network by walking shortest paths.
3. **Greedy planner (C++ port of the user's Java algorithm)** generates the
   notional plan, calibrates `L`, and provides the quality floor: the
   optimizer must never be worse.
4. **Report grows with the work**, not at the end: LaTeX skeleton early; the
   mathematical appendix is written as the formalization/proof tasks complete.

Interface verdict: visolver suffices post-reduction (dense `MatrixXd` fine at
this scale). Contingencies if scale grows: sparse `bsHe94b` overload behind
the `InnerSolver` seam, or Lagrangian dual on the budget row (matrix-free).

## Task ledger

Status: `todo` / `in progress` / `done (date)` / `descoped`.
Size: S (small), M (medium), L (large) — relative effort/usage.

### Phase A — Formalization and analysis

| ID | Task | Size | Depends on | Status |
|----|------|------|-----------|--------|
| A1 | **Formalize the problem.** Precise statement: variables, constraints, objective; resolve edge semantics (`D_i = 0` nodes: term omitted from objective and `R_i = 0` fixed; overshoot `R_i > D_i` never optimal — show it); existence/uniqueness (unique `R*`, degenerate `f`, tie-break option). Full mis-type audit of the greedy spec. Output: `doc/formulation.md` + LaTeX-ready math. | M | — | done (2026-07-03) |
| A2 | **Targeted literature pass.** Confirm/cite: shortest-path reduction (transportation problem folklore), convex QP over transportation polytope, monotone affine LCP + projection-contraction convergence (He 1994), budget-constrained min-cost flow duals. Handoff doc already has Vegh/OSQP/convex-network-flows; this fills gaps and pins citations for Part III. | S | — | done (2026-07-03) |
| A3 | **Reduction and pruning, with proofs.** Prove the shortest-path routing lemma; dominated-link elimination (`c_ij > d_ij`); epsilon-optimal source-sink pruning bound (dual/reduced-cost argument); complexity accounting. These proofs go straight into the report appendix. | M | A1, A2 | done (2026-07-03) |

### Phase B — Instances and baseline

| ID | Task | Size | Depends on | Status |
|----|------|------|-----------|--------|
| B1 | **Scaffolding + instance generator.** `network/` CMake target in the visolver build; `Instance` / `Plan` data structures; random generator matching the spec profile (70 nodes: 20 supply-only, 20 both, 30 demand-only; cost ranges, asymmetry 1-10%). GoogleTest wiring. | M | A1 | done (2026-07-03) |
| B2 | **Greedy planner.** Phase 1 rationing (closed-form lambda with `R_i >= 0` clamping) + Phase 2 greedy loop (incl. the 0.01% scale-down guard); `L` calibration at 80%; unit tests on hand-checkable instances. | M | B1 | done (2026-07-03) |
| B3 | **Swap post-processing (optional).** Busiest-node O(N^3) flow-swap improvement. Candidate for descoping — cosmetic for the baseline, not needed for `L` calibration. | S | B2 | descoped (gate 5; may revisit) |

### Phase C — Optimizer

| ID | Task | Size | Depends on | Status |
|----|------|------|-----------|--------|
| C1 | **Preprocessing implementation.** Floyd-Warshall all-pairs distances + predecessor matrix; dominated-link pruning; reduced source-sink problem construction. Tests: distances vs. brute-force path enumeration on tiny graphs. | M | A3, B1 | done (2026-07-03) |
| C2 | **Matrix generator + unpacker.** KKT of the reduced QP -> affine mixed LCP `(M, q)`, packed as `z = (x, y)` per the VINCP convention; documented block layout. Unpacker: `z -> t_mn -> (S, R, f)` via shortest-path expansion. Tests: pack/unpack round-trip, `M` monotonicity check. | M | C1 | todo |
| C3 | **Reference oracle.** Tiny (3-6 node) instances solved independently (active-set enumeration / dense KKT solve) to give exact answers. Every later box validates against it mechanically. | S | B1 | todo |
| C4 | **Solve via VINCP.** Wire `(M, q)` + mixed projector into `bsHe94b` (single affine solve, factor-once). Validate against C3 oracle; verify KKT residuals; confirm never-worse-than-greedy on B2 instances. | M | C2, C3 | todo |
| C5 | **Scale and performance.** 70- and 200-node runs; timings; tolerance tuning (`magTol` is a SQUARED norm); objective vs. greedy across a batch of random instances; document speed/quality. | M | C4, B2 | todo |

### Phase D — Technical report (LaTeX, 20-50 pp)

| ID | Task | Size | Depends on | Status |
|----|------|------|-----------|--------|
| D1 | **Skeleton + Part I (management overview).** Three-part LaTeX scaffold; Part I: problem, solution approach, challenges (scale -> reduction; degeneracy; budget calibration). | M | A1 | todo |
| D2 | **Part II (developers' manual).** Data structures, APIs, block layouts, expected usage walkthrough (generate -> greedy -> reduce -> solve -> unpack), build instructions. | M | C4 | todo |
| D3 | **Part III (mathematical appendix).** Full formalization; algorithm specification; proofs: convexity + existence/uniqueness of `R*`, reduction lemma, KKT <-> mixed LCP equivalence, monotonicity, convergence of the projection-contraction method (cited + conditions verified). | L | A3, C4 | todo |
| D4 | **Assembly and final pass.** Merge, cross-reference, numbers from C5, page-count check (20-50), consistency read. | M | D1, D2, D3, C5 | todo |

## Review gates

15 gates: one after each box above. At each gate: what was produced, what
changed in the plan (if anything), and a usage check-in so billing stays
predictable. Descoping candidates if budget tightens: B3 first, then C5
shrinks to 70-node only.

## Decisions log

- 2026-07-03: Plan approved as gate 0. Approach = reduce-then-VINCP-LCP
  (option 3 of the 2026-07-01 handoff, made viable by the shortest-path
  reduction). OSQP not used (no new dependency). Lagrangian dual kept as
  contingency.
- 2026-07-03: A1 done (`doc/formulation.md`). Pending user confirmation at
  gate 1: F1 self-supply fix (add `R_j <= sum_i f_ij`), `R_i = 0` at
  no-demand nodes, G6 greedy-comparison direction (check at `L = L_greedy`;
  sandwich bounds at `0.8 L_greedy`), whether a min-ton-mile tie-break for a
  determinate `f` is wanted.
- 2026-07-03: Gate 1 passed. All four A1 items confirmed by user: F1 fix
  adopted; R_i = 0 convention adopted; G6 check direction fixed; min-ton-mile
  tie-break to be included (form chosen in A3). Rationing clamp
  (exclude-and-resolve loop) confirmed as intended algorithm, goes into B2.
- 2026-07-03: A2 done (`doc/literature.md`). Reduction is classical (Orden
  1956); He 1992 is direct precedent for QP -> LCP -> projection-contraction.
  Plan unchanged.
- 2026-07-03: A3 done (`doc/reduction.md`). Lemma R1 exact reduction to
  source-sink shipments at shortest distances; R2 exact arc pruning
  (`c_ij > d0_ij`); R3 pair pruning with a posteriori KKT certificate
  (column generation, exact); R4 tie-break = epsilon linear ton-mile term,
  bound `theta <= theta* + eps*L`, eps = 1e-8 * sum(P)/L. KKT will be packed
  as a PURE NCP (R substituted out; no free block). Sizing: 70-node fits
  dense without R3; 200-node needs R3 to stay dense-feasible.
- 2026-07-03: B1 done. New `vincpnet` static library in `network/`
  (`include/instance.hpp`, `include/plan.hpp`, `lib/*.cpp`), nested namespace
  `VINCP::Network` (inherits the Eigen imports from `VINCP`; keeps generic
  names like `Instance` out of the solver namespace). Geometric cost model:
  uniform points in a square, `c_ij = (floor + scale*dist) * jitter`, additive
  floor = per-move handling charge (near-metric; asymmetry from independent
  per-direction jitter). `checkPlan` enforces the F1 (delivery) inequality.
  Tests: `instance_test`, `plan_test` (root CMakeLists now
  `add_subdirectory(network)`). Not yet compiled by Claude — user builds.
- 2026-07-03: Gate 4 passed (user built; instance_test + plan_test green).
- 2026-07-03: B2 done. `greedy.hpp`/`greedy.cpp`: `rationTargets` (water-fill
  with exclude-and-resolve clamp per gate-1 ruling; `lambda <= 0` round means
  remaining active demand fits — targets = D there), `greedyPlan` (direct-arc
  cheapest-source loop, 0.9999 scale-down, iteration cap guard), calibration
  `suggestedLimit = 0.8 * tonMilesUsed`. `shortfallOfResupply` extracted from
  `shortfallObjective` so rationing targets can be scored without a Plan.
  Tests: `greedy_test` (5 cases incl. hand-checked clamp and boundary G1).
- 2026-07-03: Gate 5 passed (greedy green). B3 descoped for now per user.
- 2026-07-03: C1 done. `reduction.hpp`/`reduction.cpp`: `computeShortestRoutes`
  (Floyd-Warshall + successor matrix + diagonal self-supply correction with
  `selfVia`), `routeNodes` (canonical path recovery incl. closed self-supply
  routes), `countDominatedArcs` (R2 diagnostic), `makeReducedProblem`
  (source/sink ids, `shipCost` = d-hat, per-sink k-cheapest `kept` lists for
  the R3 screen; k=0 keeps all). Tests: `reduction_test` (hand triangle with
  2 dominated arcs, diagonal correction both ways, brute-force cross-check on
  6 nodes, route-cost == distance on all 70x70 pairs, screen properties).
- 2026-07-03: User moved `flow-planning-problem.txt` into `network/` and added
  `network/alternate-laydown.txt`. Generator amended (B1 scope addition):
  `InstanceProfile::laydownType` — 0 = original random square, 1 = banded
  rectangles per alternate-laydown.txt (bare Euclidean cost, +/-5% symmetric
  directional jitter, bands A [0,240] / B [160,400] / C [320,560], y
  [100,200]), 2+ rejected by validateProfile. INTERPRETATION FLAGGED: the doc
  describes group C as "C_i = 0 and D_i = 0"; read as demand-only
  (C_i = 0, D_i > 0) to mirror the existing classes — confirm with user.
  New tests: Type1LaydownBandsAndJitter, UndefinedLaydownTypesRejected.

<!-- Copyright Ben Paul Wise. All Rights Reserved. -->

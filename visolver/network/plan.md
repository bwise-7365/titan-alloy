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
| C2 | **Matrix generator + unpacker.** KKT of the reduced QP -> affine mixed LCP `(M, q)`, packed as `z = (x, y)` per the VINCP convention; documented block layout. Unpacker: `z -> t_mn -> (S, R, f)` via shortest-path expansion. Tests: pack/unpack round-trip, `M` monotonicity check. | M | C1 | done (2026-07-04) |
| C3 | **Reference oracle.** Tiny (3-6 node) instances solved independently (active-set enumeration / dense KKT solve) to give exact answers. Every later box validates against it mechanically. | S | B1 | done (2026-07-04) |
| C4 | **Solve via VINCP.** Wire `(M, q)` + mixed projector into `bsHe94b` (single affine solve, factor-once). Validate against C3 oracle; verify KKT residuals; confirm never-worse-than-greedy on B2 instances. | M | C2, C3 | done (2026-07-04) |
| C5 | **Scale and performance.** 70- and 200-node runs; timings; tolerance tuning (`magTol` is a SQUARED norm); objective vs. greedy across a batch of random instances; document speed/quality. | M | C4, B2 | done (2026-07-04) |

### Phase E — Hybrid engine + runtime controls (added at gate 11, user request)

Motivated by the 200-node laydown-1 stall: round-0 bsHe94b hit the 150k
iteration cap (~6 min) before printing a row. Projection-contraction's
iteration count on near-degenerate (banded) problems is the binding
constraint; screens are second-order there.

| ID | Task | Size | Depends on | Status |
|----|------|------|-----------|--------|
| E1 | **Runtime controls + gap-rule screen.** All solver/screen knobs adjustable WITHOUT recompilation: simple key=value config file loaded into `FlowPlanParams` + generator profile; `network_benchmark` takes a config path argument. Screen gains a gap rule (keep sources within a cost margin of the sink's cheapest; count rule kept as an option). Also expose the tie-break epsilon experiment (R4 bounds its cost; anti-degeneracy lever for banded cases). | M | C5 | done (2026-07-04) |
| E2 | **Solodov-Svaiter global solver.** The 1999 double-projection (hyperplane) method as a third inner solver behind the same `InnerSolver`-style seam (x0, M, q, Pr, tolerances) — globally convergent under pseudomonotonicity, matrix-free (no factorization), the safe fallback of the hybrid. Unit tests on the lane + LCP suites. | M | — | todo |
| E3 | **Smoothing-Newton local engine + switching.** Rui-Xu-style inexact smoothing Newton on the natural map (mu > 0 keeps the Jacobian nonsingular; drive mu -> 0 with the residual); accept on Armijo decrease of the natural-map merit, else fall back to an E2 projection step. This is the superlinear tail that replaces thousands of contraction steps on flat optimal faces. | L | E2 | todo |
| E4 | **Hybrid integration + banded benchmark.** Wire the hybrid as an alternative engine in `solveFlowPlan` (runtime-selected per E1); rerun the benchmark including 200-node laydown-1; performance.md addendum (P6+). Success criterion: 200-banded solves in minutes with certified (or bounded-suboptimal) results. | M | E1, E2, E3 | todo |

### Phase D — Technical report (LaTeX, 20-50 pp)

| ID | Task | Size | Depends on | Status |
|----|------|------|-----------|--------|
| D1 | **Skeleton + Part I (management overview).** Three-part LaTeX scaffold; Part I: problem, solution approach, challenges (scale -> reduction; degeneracy; budget calibration). | M | A1 | todo |
| D2 | **Part II (developers' manual).** Data structures, APIs, block layouts, expected usage walkthrough (generate -> greedy -> reduce -> solve -> unpack), build instructions. | M | C4 | todo |
| D3 | **Part III (mathematical appendix).** Full formalization; algorithm specification; proofs: convexity + existence/uniqueness of `R*`, reduction lemma, KKT <-> mixed LCP equivalence, monotonicity, convergence of the projection-contraction method (cited + conditions verified). Now also: hybrid engine theory (Solodov-Svaiter convergence, smoothing-Newton local rate, switching soundness — mostly cited). | L | A3, C4, E4 | todo |
| D4 | **Assembly and final pass.** Merge, cross-reference, numbers from C5, page-count check (20-50), consistency read. | M | D1, D2, D3, C5 | todo |

## Review gates

19 gates (15 original + 4 phase-E): one after each box. At each gate: what
was produced, what changed in the plan (if anything), and a usage check-in so
billing stays predictable. Descoping candidates if budget tightens: B3
(already descoped), then E-phase shrinks (E1 alone still delivers the
runtime-controls requirement + gap screen).

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
- 2026-07-04: User confirmed group-C reading (demand-only; doc typo). Two
  generator amendments: `numNeither` inert-node class (C=0=D, corner-case
  testing; inert x spans the whole band range in type 1) and a type-1
  per-ordered-pair off-diagonal cost floor U[bandMinCostLo=5.0,
  bandMinCostHi=10.0]. Ratio test guarded where clamping may bite. Spec
  corrections also recorded in the cross-session memory file
  `project_flowplan_network.md` per user request.
- 2026-07-04: C2 done. `flowlcp.hpp`/`flowlcp.cpp`: `buildFlowLcp` assembles
  the KKT complementarity system, layout `z = [t | mu | lambda]` (pure
  non-negative block; empty free block), sink-major t enumeration following
  `reduced.kept`; q carries `-2P/D + eps*d`, caps, L; M = Q-blocks + skew
  incidence/budget pairs (monotone by construction, PSD-checked in tests).
  `defaultTieBreakEpsilon` = 1e-8 * sumP / L (R4). `unpackFlowLcp` expands t
  along `routeNodes` to a full `(S, R, f)` Plan. Tests: `flowlcp_test`
  (layout + eigenvalue monotonicity; hand-derived KKT points, unconstrained
  and budget-bound, verified through `M z + q`; multi-hop unpack over the
  dominated-arc triangle; random type-1 + inert-node unpack feasibility;
  input guards).
- 2026-07-04: Round-off fix: random-instance greedy feasibility check uses
  kFeasTol=1e-9 (builder/checker Eigen sums differ in association order).
  Aggregate CMake targets `network_tests` / `run_network_tests` added for
  CLion (ctest -R ^Network; excludes base-library suites).
- 2026-07-04: C3 done (design confirmed by user: full-formulation oracle).
  `oracle.hpp`/`oracle.cpp`: `buildOracleKkt` assembles the ORIGINAL
  (S, R, f) formulation's KKT as a mixed LCP — free block = balance
  multipliers nu, y = [f | S | R | delta | mu | lambda]; no reduction, no
  tie-break. `solveOracle` = dHan06 over `makeMixedProjector` (different
  solver AND formulation from production), capped at maxNodes=8.
  `lanesupport.hpp` shared test fixture (flowlcp_test refactored to use it).
  Tests: `oracle_test` — lane closed forms (unconstrained + budget-bound),
  KKT monotonicity, size/calibration guards, and the KEY cross-check:
  production path (reduce -> bsHe94b) vs oracle agree on theta and R* on a
  gentle 5-node random instance (validates Lemma R1 end to end). f is NOT
  compared (degenerate face; oracle has no tie-break).
- 2026-07-04: SolvesLaneBudgetBound failed to converge (25s, 300k iters):
  the RAW KKT mixes ~1e3 budget/cost rows with ~1e-4 multipliers; projection
  methods stall on it. Fix: solveOracle NONDIMENSIONALIZES (tons / largest
  demand, miles / largest cost — exact unit change, optimum invariant),
  solves the O(1) system, scales the plan back; default oracle magTol
  1e-14 -> 1e-12 (scaled units). IMPORTANT FOR C4: the production solveFlowPlan
  wrapper must apply the same nondimensionalization around bsHe94b — at full
  70-node scale (tons ~5e3, miles ~2e3, L ~1e8) the raw system is far worse.
- 2026-07-04: Scaling fix confirmed (lane oracle 25s -> 0.04s; solvers agree).
  Remaining failure was the SANDWICH slack: the rationing lower bound is
  exact only for exactly-feasible plans; solver iterates are feasible to
  ~residual scale and can undercut it by (dtheta/dR ~ 2P/D) * slack. Lesson
  for ALL C4/C5 bound checks: slack = feasibility tolerance x gradient scale,
  never 1e-9. Test now uses kSandwichSlack = 1e-4.
- 2026-07-04: C4 done. `flowplan.hpp`/`flowplan.cpp`: `solveFlowPlan` =
  nondimensionalize -> computeShortestRoutes -> makeReducedProblem (optional
  k-cheapest screen) -> buildFlowLcp -> bsHe94b -> R3 certificate loop
  (violated excluded pairs appended to kept, re-solve; certifiedP honest) ->
  unpack -> rescale. Returns shortfall, real ton-miles, REAL-unit budget
  shadow price (lambda/(tonScale*mileScale)), kept/total pairs, rounds.
  `gentlesupport.hpp` shared fixture (oracle_test refactored). Tests:
  `flowplan_test` — oracle agreement (certified, keep-all), screen k=3 vs
  exact keep-all on a 26-node real-scale instance (R3 recovers the same
  optimum with fewer pairs), G6 baseline bounds (never worse than greedy at
  L_greedy; greedy is a lower bound at 80%; monotone in L; positive shadow
  price when scarce), input guards.
- 2026-07-04: FINDING from the failed shadow-price assertion: at 80% of
  greedy ton-miles the budget was NOT scarce on the 26-node instance — the
  optimizer's shortest-path routing + assignment saves >20% over greedy
  (lambda = 0, full rationed targets delivered). Report-worthy (quantifies
  greedy inefficiency); C5 should measure the savings ratio across a batch.
  Test now asserts complementarity always, and positivity only on a FORCED
  scarce budget (0.5 x working optimum's own usage).
- 2026-07-04: C5 code delivered (box closes when benchmark output is recorded
  in doc/performance.md). `scale_test` — 70-node screened solve in the ctest
  suite. `network_benchmark` exe (network/src/benchmark.cpp) — 4 configs x 5
  seeds: 70-node keep-all (exact reference), 70-node screen k=10, 70-node
  laydown-1 screen, 200-node (50/50/95+5) screen k=10; reports kept pairs,
  certificate rounds, iterations, wall ms, theta_ration/theta*, delivered
  fraction, ton-mile fraction vs greedy, shadow price. Run in RELEASE.
- 2026-07-04: FINDING (user run): 70-node KEEP-ALL made no visible progress
  (single core pegged; 2041 unknowns x ~4ms/iter x 500k cap = potentially
  30+ min before honest failure). The R3 screen is an ITERATION-COUNT
  necessity, not just memory relief: certified screening IS the production
  configuration; keep-all is a small-instance reference only. Benchmark
  restructured: 26-node keep-all + screened cross-check, 70-node (both
  laydowns) and 200-node screened; iterMax capped at 150k so failures print
  honest cert=NO rows. Goes into performance.md and Part I challenges.
- 2026-07-04: The keep-all 70-node solve DID converge — reference datum
  (seed 20260704, laydown 0, keep-all): 43,247 iters, 144.2 s, certified;
  th_ration 2.25076, th_star 2.70043, dlv 0.997, tm/tmG 0.800 (budget BINDS
  at 70 nodes on this seed, lambda 6.23e-07), vs the 26-node case where the
  optimizer absorbed the whole 20% cut. Both budget regimes are real.
  Screened-vs-keep-all speedup to be quantified by the restructured
  benchmark (expected ~two orders of magnitude).
- 2026-07-04: C5 CLOSED — full benchmark results in `doc/performance.md`.
  Headlines: certificate exact in practice (5/5 configs certified, theta*
  matches keep-all to 5-6 digits); screen = ~160x at 70 nodes (0.9s vs 144s);
  three budget regimes observed (bound / comfortable / rationing-dominated,
  readable from lambda, dlv, tm/tmG); banded laydown is the hard case (kept
  set balloons to ~1650/2000 over 2-3 rounds, ~74 s, and economically ~2x
  worse theta* on identical tonnages); 200 nodes ~45 s/solve. Future levers
  recorded: gap-based screen; the deferred Solodov-Svaiter/smoothing hybrid.
  Phase C complete.
- 2026-07-04: 200-node laydown-1 run ABANDONED by user after 10 min, zero
  rows: round-0 bsHe94b (k=10, ~1550 unknowns) hit the 150k cap (~6 min)
  without converging — PC iteration count on near-degenerate banded problems
  is the binding constraint; screen choice is second-order at this size.
- 2026-07-04: Gate 11. SCOPE ADDED at user request: Phase E (E1 runtime
  controls + gap-rule screen; E2 Solodov-Svaiter global solver; E3
  smoothing-Newton + switching; E4 hybrid integration + banded benchmark).
  The deferred hybrid-engine design (CLAUDE.md / 2026-06-29 handoff) is now
  explicitly requested. D3 gains dependency on E4. USER REQUIREMENT recorded:
  all tuning knobs (screen rule/size, slacks, epsilon, solver selection,
  tolerances) must become runtime controls — no recompilation (E1). Screen
  ranking for the record: gap rule > slack-loosening > count-k; but the
  hybrid is the real lever for 200-banded. Tie-break epsilon flagged as a
  cheap anti-degeneracy experiment (R4 bounds its cost).
- 2026-07-04: E1 done. `config.hpp`/`config.cpp`: strict key=value loader
  (comments/trim; malformed, duplicate, junk-valued, and UNKNOWN keys all
  throw — a typo can never silently keep a default), typed consume* helpers,
  `applyFlowPlanConfig` (solver.*/screen.* keys incl. solver.epsilon) and
  `applyProfileConfig` (all InstanceProfile fields). `ScreenParams` gap rule
  in makeReducedProblem (union-of-prefixes semantics; count overload kept);
  `FlowPlanParams.gapFraction`. `network_benchmark <cfg>` runs ONE
  file-described config (benchmark.name/instances/seedBase); no-arg = C5
  sweep. `benchmark-example.cfg` = the 200-banded experiment ready to run.
  Tests: `config_test` (parse/apply/typo-guard/file), GapRuleScreens in
  reduction_test, gap-screen certification in flowplan_test.

<!-- Copyright Ben Paul Wise. All Rights Reserved. -->

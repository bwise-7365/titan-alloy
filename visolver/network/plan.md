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
| B3 | **Swap (2-exchange) engine + interactive GUI.** `swap.{hpp,cpp}`: `bestSwapAtNode` (v1, one swap), `swapNodeToLocalOptimum` (intermediate: one node iterated), `bestSwap` (v2, global one), `swapToLocalOptimum` (v3, global iterate), `applySwap`; `swap_test`. Viewer: right-click a node = its best swap, shift+right-click = that node to its optimum; "Best swap" / "Swap to optimum" / "Reset plan" buttons; result popup (edges, tons, saving) + magenta arc highlight; working plan persists until Regenerate. | M | B2, F1 | in progress (2026-07-05) |

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
| E2a | **Solodov-Svaiter solver + LCP tests.** The 1999 double-projection (hyperplane) method as a third inner solver behind the same seam (x0, M, q, Pr, magTol, iterMax, iterFreq, params, logger) — globally convergent under pseudomonotonicity, matrix-free (no factorization), the safe fallback of the hybrid. Known-solution tests mirroring lcp_psd_test. | M | — | done (2026-07-04) |
| E2b | **Ellipsoid LVI tests (user-requested).** Solodov-Svaiter on the linear-objective-over-ellipsoid problem with the closed-form optimum, same setup/printout as lvi_ellipsoid_test; plus a three-way dHan06 / bsHe94b / solodovSvaiter comparison in the han_vs_he style. The user recalls LVI-on-ellipsoid difficulties with SS from past work — this is the targeted regression. | S | E2a | done (2026-07-04) |
| E3a | **Chained two-phase solver (user proposal, gate 14).** Solodov-Svaiter to a LOOSE tolerance (global, matrix-free, any start), then bsHe94b warm-started from its iterate for the tight finish. One `InnerSolver`-seam adapter; tests: ellipsoid three-way gains a chain row that must hit the tight 1e-6 bar, LCP mirror at tight tolerance. | S | E2a, E2b | done (2026-07-05) |
| E3b | **Smoothing-Newton local engine + switching (contingent).** DEFERRED pending E4: only if the E3a chain fails to crack the 200-node banded benchmark (i.e. bsHe94b's stall there is rate-dominated, not transit-dominated). Rui-Xu-style inexact smoothing Newton on the natural map; Armijo acceptance; SS fallback. | L | E4 verdict | deferred |
| E4 | **Engine integration + banded benchmark.** Wire engine selection into `solveFlowPlan` (runtime key `solver.engine` = bshe94b / chain, per E1); rerun the benchmark including 200-node laydown-1; performance.md addendum (P6+). Success criterion: 200-banded solves in minutes with certified (or bounded-suboptimal) results. Verdict decides E3b. | M | E1, E3a | todo |

### Phase F — Testing and visualization (added at gate 20, user request)

Sits between the hybrid engine (Phase E) and the report (Phase D): tools to
*see* what the generator and solver produce, feeding the Part IV testing
appendix (D5). The instance viewer is the first cut; flow overlay and an
interactive screen/tolerance explorer follow.

| ID | Task | Size | Depends on | Status |
|----|------|------|-----------|--------|
| F1 | **Instance viewer + node coordinates.** Retain node `(x, y)` on `Instance` (obligatory field, populated by `makeRandomInstance` for both laydowns; `validateInstance` checks it only when present, so abstract hand-built instances still pass). Qt6 `network_viewer` (`network/gui/`): draws a generated instance as a map — nodes coloured by class (supply-only / both / demand-only / **transit**), sized by tonnage, with a legend. Mouse **pan (drag) + wheel zoom**, a **Recenter** button restoring the fitted view, and a 0..N-1 **"closest links"** spinner drawing orange links from each node to its k cheapest neighbours by `(c_ij + c_ji)/2`. Seed 0 = reroll (fresh time-based seed written back to the field). Instances only (no greedy/solver overlay yet). Optional target behind `VINCPNET_BUILD_GUI`, self-disabling if Qt6 is absent so the solver/test build never depends on Qt. | M | B1 | done (2026-07-05) |
| F2 | **Plan / flow overlay.** Draw greedy and optimal flows on the same map (arc width ~ flow), toggle instance / greedy / optimal, and show theta and ton-miles; a visual check of solver output against the geometry. Greedy overlay + mode radio done (2026-07-05); optimal (`solveFlowPlan`) overlay pending. | M | F1, B2, C4 | in progress |
| F3 | **Interactive screen / tolerance explorer (optional).** Vary the k-cheapest / gap screen, tie-break epsilon, and tolerances at runtime and watch kept pairs, certificate rounds, and iterations — a visual companion to the E1 config controls and the C5/E4 performance study. | S | F1, E1 | todo |

### Phase D — Technical report (LaTeX, 20-50 pp)

| ID | Task | Size | Depends on | Status |
|----|------|------|-----------|--------|
| D1 | **Skeleton + Part I (management overview).** Three-part LaTeX scaffold; Part I: problem, solution approach, challenges (scale -> reduction; degeneracy; budget calibration). | M | A1 | todo |
| D2 | **Part II (developers' manual).** Data structures, APIs, block layouts, expected usage walkthrough (generate -> greedy -> reduce -> solve -> unpack), build instructions. | M | C4 | todo |
| D3 | **Part III (mathematical appendix).** Full formalization; algorithm specification; proofs: convexity + existence/uniqueness of `R*`, reduction lemma, KKT <-> mixed LCP equivalence, monotonicity, convergence of the projection-contraction method (cited + conditions verified). Now also: chained-solver rationale (SS global convergence + the O(1/sqrt(k)) tail analysis motivating the chain). | L | A3, C4, E4 | todo |
| D5 | **Part IV (testing appendix; added gate 15 at user request).** How thoroughly the result was tested: known-solution unit tests; brute-force cross-checks (Floyd-Warshall vs path enumeration); the INDEPENDENT oracle (different formulation AND solver, validating Lemma R1 end to end); hand-derived KKT points pushed through `M z + q`; R3 certificates as per-run optimality proofs; the sandwich bounds; feasibility checkers (incl. the F1 shortcut test); the SS calibration story (honest failure -> globalization-grade bars); benchmark methodology; the visual evidence from the Phase-F viewer/overlay. | M | C3, C5, E4, F1 | todo |
| D4 | **Assembly and final pass.** Merge, cross-reference, numbers from C5, page-count check (20-50), consistency read. | M | D1, D2, D3, D5, C5 | todo |

## Review gates

22 gates (15 original + 4 phase-E + 3 phase-F): one after each box. At each
gate: what was produced, what changed in the plan (if anything), and a usage
check-in so billing stays predictable. Descoping candidates if budget tightens:
B3 (already descoped), then F3 and E-phase shrinks (E1 alone still delivers the
runtime-controls requirement + gap screen; F1 alone delivers the viewer).

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
- 2026-07-04: E2 split at user request (E2a solver + LCP tests; E2b ellipsoid
  LVI tests — user recalls SS difficulties on ellipsoidal K from past work).
- 2026-07-04: E2a done. ROOT-library `solodovsvaiter.hpp`/`.cpp` (namespace
  VINCP, beside dhan06/bshe94b, same seam): double-projection with
  Armijo-style hyperplane search; matrix-free (no factorization); projects
  x0 onto K first; squared-residual convention; NaN/divergence/linesearch
  guards throw. Tests `ss_lcp_test` mirroring lcp_psd_test
  (makeComplementaryPair + printConstructed + expectSolvePasses +
  checkCloseToKnown), plus a hand 1-D LCP from an infeasible start and
  param/input guard cases. Root CMake updated.
- 2026-07-04: E2b done. `makeSolodovSvaiterSolver` adapter added beside the
  other two in josephynewton.hpp/.cpp (same InnerSolver seam).
  `ss_ellipsoid_test`: (1) SS DIRECTLY on the affine VI (M=0, q=c) over the
  ellipsoid projector — flat field, curved boundary, the recalled trouble
  regime; (2) three-way dHan06/bsHe94b/solodovSvaiter through Josephy-Newton
  with printSolveStats per solver for side-by-side comparison. Same seed
  (makeSeed(20260703)) and printout as lvi_ellipsoid_test so logs align.
  iterMax 500k, magTol 1e-14 (squared): if SS crawls on the boundary the
  honest converged flag + stats will show it.
- 2026-07-05: E2b FINDING (user runs: all SS tests red at tight tolerances,
  matching his past experience). Root cause is structural, not a bug: the
  hyperplane step lambda = <F(y), x-y>/||F(y)||^2 ~ ||r||^2/||F||^2, so
  whenever ||F(x*)|| > 0 (strict-complementarity LCPs, the ellipsoid's
  constant field) steps collapse quadratically with the residual and error
  decays O(1/sqrt(k)) — observed 3.8e-3 after 200k iters, on-curve. When
  F(x*) = 0 the denominator vanishes too and SS is fast (1-D hand LCP).
  VERDICT: SS = globalizer, not finisher; E3's smoothing-Newton tail is the
  finisher. Tests retuned to globalization-grade targets for SS (solTol 5e-2
  LCP, relTol 1e-2 ellipsoid; per-method bars in the three-way; outer cap 5
  bounds the grind) with the analysis in the test comments. printSolveStats
  added so the crawl is visible. Goes to Part III (hybrid motivation).
- 2026-07-05: E2b closed GREEN. Measured contrast on the identical ellipsoid
  instance: dHan06 35 inner iters (1e-15), bsHe94b 101 (1e-15), SS 500k cap
  (1.3e-3 squared). Gate-14 plan change (user proposal): E3 restructured —
  E3a = chained SS(loose) -> bsHe94b(warm-started, tight) at the InnerSolver
  seam; E3b smoothing-Newton DEFERRED, contingent on the E4 200-banded
  verdict (chain wins if the stall is transit-dominated; Newton tail needed
  if rate-dominated).
- 2026-07-05: E3a done. ROOT-library `chainedsolver.hpp`/`.cpp`:
  `chainedSolodovHe` — SS to roughMagTol 1e-4 / roughIterMax 20k (cap-and-
  hand-off is by design), then bsHe94b from the warm start to the caller's
  magTol; composite accounting (iter = phase-2, innerIters = both phases).
  `makeChainedSolver` adapter beside the others. Tests: fourth row
  "ssHeChain" in the ellipsoid three-way at the TIGHT 1e-6 bar; tight-bar
  PSD-LCP chain test with phase-accounting check.
- 2026-07-05: Gate 15 user requests: experiments memory written
  (`project_flowplan_experiments.md`, session-reset context); report gains
  Part IV testing appendix (new box D5, feeds D4; 20 gates total).
- 2026-07-05: E4 CODE done (box closes on the 200-banded verdict).
  `FlowPlanParams.engine` = "bshe94b" | "chain" (+ roughMagTol/roughIterMax);
  dispatch in the certificate loop; unknown engine throws. Config keys
  solver.engine/roughMagTol/roughIterMax. `benchmark-example.cfg` now runs
  the 200-banded chain experiment as-is. Tests: EngineSelectable (chain
  matches default engine's certified optimum on the 26-node instance;
  unknown engine refused), config key coverage. AWAITING: user's Release
  runs of benchmark-example.cfg with engine chain vs bshe94b -> P6 addendum
  -> E3b verdict.
- 2026-07-05: Doc consistency spot-check (formulation / reduction / literature
  / performance) passed -- mutually consistent and matching this ledger and the
  cross-session memory (variable counts, node profiles, benchmark shape, F1
  self-supply handling all cross-verified). Only nit: the group-C "C=0, D=0"
  typo the memory records is already fixed in `alternate-laydown.txt` on disk.
- 2026-07-05: `fpp_v02.txt` written -- a clean, literally-correct restatement of
  `flow-planning-problem.txt` that folds in the A1 findings (F1 delivery
  constraint `R_j <= sum_i f_ij`; R_i = 0 objective domain; L3 no-overshoot; G6
  greedy-bound direction; G1/G2 rationing clamp; R4 tie-break), with a
  CHANGES-FROM-v01 changelog at the top. v01 is left untouched as the historical
  original (it still carries its own "do not take literally" warning).
- 2026-07-05: SCOPE ADDED (gate 20, user request): Phase F (Testing and
  visualization) inserted between Phase E and Phase D; F1/F2 feed D5. F1 done
  (code; awaiting user's Qt build). `Instance` gains obligatory `xCoord`/`yCoord`
  (populated by `makeRandomInstance`; RNG draw order UNCHANGED, so all existing
  instances are byte-identical -- coords are simply retained rather than
  discarded). Design choice recorded: `validateInstance` checks coordinates only
  WHEN PRESENT, because it runs inside every solver entry point and on ~5
  hand-built abstract test instances that carry no geometry; the viewer only
  ever shows generated instances, which always have real coordinates. Qt6
  `network_viewer` in `network/gui/` (flowplanview + mainwindow + main_gui),
  matching the irrgo Qt6/MSVC2022/AUTOMOC pattern; optional behind
  `option(VINCPNET_BUILD_GUI ON)` and self-disabling via `find_package(Qt6
  QUIET)` so the core build never requires Qt. Controls: laydown, seed, four
  class counts, Regenerate, and the 0..N-1 nearest-links spinner (each node ->
  its k closest neighbours, k=0 = none). Build: reload CMake in CLion (target
  `network_viewer`) or `cmake -S . -B build -DCMAKE_PREFIX_PATH=<qt>/msvc2022_64`
  then `cmake --build build --target network_viewer`.
- 2026-07-05: F1 viewer refinements (user batch) + one generator tweak.
  (1) "inert" node class renamed to **transit** everywhere in code and GUI
  (NodeClass, colours, legend, combo label, comments, fpp_v02.txt); the profile
  field / config key `numNeither` was KEPT (accurate word "neither"; renaming it
  would break config files and ~10 test/benchmark sites) -- flagged, rename
  available on request. (2) Map gains mouse pan (left-drag) + wheel zoom (about
  cursor) + a Recenter button (restores the fitted, centred transform). (3+4)
  The closest-links spinner now ranks neighbours by the SYMMETRISED cost
  `(c_ij + c_ji)/2` (not Euclidean) and draws ORANGE links; range 0..N-1.
  (5) Type-1 `bandYHi` 200 -> 250 (y-band [100,250], 50% taller; upper limit
  only). Type1LaydownBandsAndJitter derives its bounds from `bandYHi`, so it
  still passes; the change deliberately diverges from alternate-laydown.txt
  (left as the historical original, like v01). (6) Seed 0 is a reroll sentinel:
  Regenerate draws the low 31 bits of microseconds-since-epoch (masked to fit a
  QSpinBox int; precision irrelevant) and writes it back so the user can copy
  the actual seed used. RNG draw order still UNCHANGED, so same-seed instances
  remain byte-identical.
- 2026-07-05: F2 started (greedy overlay). A mutually exclusive "Show" radio
  ("Random Placement" default / "Greedy Plan") drives the map: greedy mode runs
  `greedyPlan(current_)` and overlays its directed flows f_ij as teal arrows
  (width ~ flow, arrowheads back off node radii; diagonal self-supply not
  drawn); placement mode hides them. The radio persists so it is visible which
  mode produced the current view; the status line shows greedy ton-miles +
  shortfall. `FlowPlanView` gained `setPlan`/`clearPlan` + a `Plan` overlay;
  MainWindow keeps the current instance so a mode switch recomputes without
  regenerating. Only the greedy radio's toggle is connected (fires
  applyPlanMode once per switch). No CMake change -- `network_viewer` already
  links `vincpnet` (greedyPlan/tonMiles/shortfallObjective). Optimal-plan
  (`solveFlowPlan`) overlay still pending to close F2.
- 2026-07-05: F2 "Show" radio simplified (user request). Options are now
  **"Closest" / "Greedy Plan"** (the "Random Placement" option was dropped -- the
  Laydown menu already selects placement, and "Closest" with count 0 is a plain
  node map). The closest-links count spinner moved out of the Display group to
  sit indented directly under the "Closest" radio; it is enabled only in Closest
  mode. The two overlays are mutually exclusive (greedy mode forces links off).
- 2026-07-05: Viewer additions (user batch). "Show" box renamed **"Show Links"**
  and gains a **"None"** radio (now the default) that draws no overlay; the three
  options (None / Closest / Greedy Plan) sit in a QButtonGroup (single signal per
  selection). New **cost histogram** widget (`gui/costhistogram.{hpp,cpp}`): bins
  ALL N^2 costs c_ij into K equal-width buckets [0, w), [w, 2w), ... over
  [0, maxCost] (the max value clamped into the closed last bin), with its own
  "bins" spinner K in [1, 25] recomputing live. Added to the viewer CMake source
  list; the histogram refreshes on each regenerate.
- 2026-07-05: Viewer node identity + inspection (user batch). (a) `Instance`
  gains `labels` (one per node: class letter + zero-padded 3-digit per-class
  counter -- "S###" supply-only, "M###" both, "D###" demand-only, "T###"
  transit), generated in `makeRandomInstance`, validated when present, with a
  shared `nodeLabel(inst, i)` accessor (synthetic "#i" fallback). Test
  `NodeLabelsByClass`. (b) A "Labels" checkbox (default OFF) draws each label
  centred on its dot. (c) Two side-by-side node rankings (right panel), greyed
  out unless a plan is shown: "By tonnage" (throughput = sum of flows to/from a
  node, diagonal self-supply excluded) and "By count" (number of nonzero flows
  to/from), from one shared `computeNodeFlowStats` + `fillNodeList`. (d) Nodes
  are clickable: press-and-hold shows a popup (name, C, D) that hides on release;
  a `NodeListWidget` (both lists) mirrors this so a list press drives the SAME
  map popup via `FlowPlanView::showNodeInfo`/`hideNodeInfo`. Shared helpers:
  `nodeRadius`/`nodeAt` (paint + hit-test), one `drawPopup`. New files
  `gui/nodelistwidget.{hpp,cpp}` (in the viewer CMake list). Window widened to
  1280x720 for the third panel.
- 2026-07-05: `shortfallVsTarget(inst, target, resupply)` added to plan.{hpp,cpp}
  (shortfallOfResupply refactored onto it); the greedy status split into
  supply/demand, delivered/unmet (TONS), and objective-vs-rationed /
  objective-vs-original (the objective theta is dimensionless, ~single digits --
  NOT the ~D-C ton gap). Test `ShortfallVsTargetSeparatesRationedFromOriginal`.
- 2026-07-05: B3 un-descoped and built (user request) as the transportation
  2-exchange in THREE depths. Library `network/swap.{hpp,cpp}` (in vincpnet):
  `SwapMove`/`SwapSummary`; `bestSwapAtNode` (v1), `bestSwap` (v2),
  `swapToLocalOptimum` (v3), `applySwap`; over positive OFF-diagonal arc pairs,
  ranked by TOTAL ton-mile saving `x*((c_ij+c_mn)-(c_in+c_mj))`, `x =
  min(f_ij,f_mn)`; supplied/resupply invariant (only routing/cost change). Test
  `swap_test` (crossed 2x2: one swap saves 90 t-mi, resupply invariant,
  incidence filter, loop-to-optimum). GUI: RIGHT-click a node = its best local
  swap (left-click still = info popup; context menu suppressed); buttons "Best
  swap" / "Swap to optimum" / "Reset plan" (enabled only in Greedy mode); result
  popup (saved t-mi, tons, -/+ edges by label) + magenta arc highlight. Working
  plan is a mutable copy persisting across mode switches until Regenerate/Reset;
  status gains "(+N swaps, saved ... t-mi)" and the lists re-rank per swap.
- 2026-07-05: Added the INTERMEDIATE swap depth `swapNodeToLocalOptimum(inst,
  plan, node)` (iterate bestSwapAtNode at one node until it has no improving
  move) between v1 (single) and v3 (global iterate). GUI: shift+right-click a
  node drives it to its swap optimum (plain right-click still = one swap). Test
  `NodeToLocalOptimumIterates` (a node with two independent crossed pairs takes
  exactly 2 swaps, saving 180, resupply invariant).
- 2026-07-05: Gravity (proportional all-to-all) planner added as a crude
  baseline (user request). `gravity.{hpp,cpp}` (in vincpnet): `gravityPlan` =
  `f_ij = C_i * D_j / max(totalC, totalD)` (outer product / larger total).
  Feasible + cost-blind + DENSE; the short side is proportionally rationed and
  total moved = min(totalC,totalD). NOTE: user wrote "K = sum of rationed
  demands" but that (= min) over-ships supply when C<D; the intended divisor is
  max(C,D), and the sum-of-rationed is the resulting TOTAL DELIVERED (flagged,
  built as max). Tests `gravity_test` (both C<D and C>D cases). GUI: a 4th
  "Gravity Plan" radio in "Show Links" directly below "Greedy Plan"; shows the
  dense overlay + lists + status ("gravity plan", no objective-vs-rationed line);
  swaps stay GREEDY-ONLY (gravity is dense -> O(m^4) swaps, and it's a view-only
  baseline). MainWindow gained a plan-kind tag (`workingKind_` 0/1/2) so
  switching greedy<->gravity recomputes; `refreshGreedyStatus` -> generalized
  `refreshPlanStatus`.
- 2026-07-05: Swaps briefly enabled on gravity, then REVERTED to greedy-only:
  a swap pass over the dense gravity plan (O(arcs^2) per bestSwap, arcs ~
  sources x sinks) is infeasibly slow and freezes the synchronous UI. Gravity is
  view-only again; swap controls/right-click stay gated on greedy mode
  (`workingKind_ == 1`). (If wanted later: run "Swap to optimum" on a worker
  thread with progress/cancel.)
- 2026-07-05: Mehrotra predictor-corrector interior-point engine added to the
  ROOT library (engine-roadmap Option 2, stages IP1-IP3; literature research
  and the three-option roadmap recorded in the cross-session memory).
  `mehrotraipm.{hpp,cpp}` beside the other inner solvers: monotone MIXED LCP
  over R^n x R_+^m (`numFree` parameter; no warm start by nature; one dense LU
  of M + blockdiag(0, diag(s./y)) per iteration shared by predictor+corrector;
  terminates and reports on the shared SQUARED natural-residual convention;
  sticky `regEpsilon` free-block regularization when the Newton solve is
  singular). Gate-verified counts: 9/19/10 iterations on clean / degenerate /
  rank-deficient PSD LCPs and 4-5 on mixed QPs, vs bsHe94b's 60 on the
  identical mixed instance — the degeneracy-insensitivity it was built for.
  IP3 wiring: seam adapter `makeMehrotraIpmSolver(numFree, ...)` (ignores x0
  and Pr — orthant/mixed K only, documented); third row in han_vs_he_test;
  network `solver.engine = "ipm"` (numFree = 0 pure NCP; under this engine
  iterMax counts LU factorizations — set it in the low hundreds) + docs +
  EngineSelectable coverage. Tests: mehrotra_ipm_test (7 cases). AWAITING:
  Ben's IP3 test run, then IP4 = 200-banded benchmark ipm vs chain vs bshe94b
  -> performance.md addendum -> feeds the E3b verdict and the engine-dispatch
  preprocessing design.
- 2026-07-06: IP3 verified green (han_vs_he 214 ms; ipm rows pass). IP4a
  bounded probe run (200-banded, engine ipm, instances=1,
  maxCertificateRounds=1, iterMax 200, Release, seed 20260704): kept
  10737/14500, rounds 1, final solve 35 iterations at Newton dim 10,838,
  wall ~29.6 min, th_star 53.117 vs th_ration 39.842, dlv 0.957, lambda
  2.2e-6, cert NO (honest one-round answer). VERDICT: the IPM's
  degeneracy-insensitive iteration count holds at scale (35 iters where
  bsHe94b had capped at 150k on a dim ~1.9k round-0 system); the binding
  cost is now the R3 certificate balloon (74% of keep-all after ONE round)
  times dense dim^3 LU (~50 s/iteration). IP4b (chain + bshe94b bounded
  controls) DEFERRED at Ben's direction to the coming weekend; IP4c
  (performance addendum) follows. Next build phase: the NewtonSolverFactory
  seam + structured flow factory (per-sink rank-1 Sherman-Morrison + dual
  Schur, SPD/LLT) enabling a no-screen keep-all solve; the algebra is
  machine-verified by `doc/ns2-newton-check.mac` (Maxima, ALL 10 CHECKS
  PASS, 2026-07-06). **Plan of record / status board:
  `../2026-07-06-engine-plan.md`** (repo root), created so the gate table is
  visible beside the working dialog.
- 2026-07-06: MCP track (Ben: the mixed NONLINEAR complementarity problem is
  the most important case). MC1 done: confirmation literature pass verified
  DFK/SEMI semismooth Newton + PENALIZED Fischer-Burmeister (Chen-Chen-Kanzow,
  lambda 0.8 default) as the choice; design in the status board; 19 cited
  papers archived under `../doc/` (left untracked like the existing PDFs).
  MC2 code done: ROOT-library `semismoothnewton.{hpp,cpp}` — direct solver
  for H = 0, 0 <= G _|_ y >= 0 over R^n x R_+^m (VIModel in, VIResult out;
  K compiled into Phi = [H; phi(y, G)]; swappable NcpFunctionPair + JacobianFn
  seams; Newton -> LM -> gradient ladder; directional Armijo w/ optional
  nonmonotone memory; natural-residual stop). `armijoLineSearchDirectional`
  added to armijo (gradient-form test; Ben-approved API addition). Tests:
  semismooth_newton_test (5 cases) + a fourth solver row in han_vs_he_test.
  AWAITING Ben's build+run (CMake reload needed).
- 2026-07-06: MC2 gate-verified by Ben (cubic 14 iters; degenerate affine 11
  vs ipm's 18; mixed QP 2 iters with residual exactly 0). One bug found by
  the mixed-QP test and fixed: the CCK kink-indicator vector lives in the
  FULL z-space (ones at n+i), not y-space — pure-LCP tests (n = 0) cannot
  catch that mistake. MC3 added at Ben's request (the semismooth engine had
  NO big-problem exercise): `solver.engine = "ssn"` dispatch in solveFlowPlan
  (flow LCP wrapped as a pure-NCP VIModel with its exact constant Jacobian;
  iterMax counts LU factorizations — low hundreds) + EngineSelectable ssn
  block; an ssn row joins the deferred IP4b weekend benchmark; the incoming
  reformulated GAMS model is designated the big-NONLINEAR acceptance test.
  MC3 code awaiting Ben's run (plain rebuild).

<!-- Copyright Ben Paul Wise. All Rights Reserved. -->

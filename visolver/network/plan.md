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
| F2 | **Plan / flow overlay.** Draw greedy and optimal flows on the same map (arc width ~ flow), toggle instance / greedy / optimal, and show theta and ton-miles; a visual check of solver output against the geometry. Greedy overlay + mode radio done (2026-07-05); optimal (`solveFlowPlan`) overlay done 2026-07-06 (worker-thread solve, ipm + flow Newton keep-all, cached per instance, theta*/certified/lambda status) | M | F1, B2, C4 | done, verified 2026-07-06 |
| F4 | **FUTURE WORK -- sparse optimal plans.** The verified optimal overlay reduces ton-miles (greedy 3.1e7 -> swapped 2.8e7 -> optimal 2.5e7 on the observed instance) but the flow pattern is "full of tiny flows going all over": look for a way to limit the number of nonzero links. Note the LIKELY CAUSE: the IPM converges to the ANALYTIC CENTER of the optimal face -- the maximally spread-out optimal routing -- so tiny flows are the engine's signature, not noise. Candidate levers, cheapest first: (a) a crossover/purification post-step toward a vertex of the optimal face (basic solutions of network problems are forest-sparse); (b) a consolidation heuristic on the returned plan (reroute sub-threshold flows onto their cheapest kept alternative, re-verify feasibility + objective delta); (c) a larger tie-break epsilon (R4 bounds the objective cost); (d) a concave/fixed-charge sparsity term (changes the problem class -- last resort). PREFERRED FIRST TRY (Ben, 2026-07-06): the consolidation heuristic (b). CALIBRATING OBSERVATION (same day): greedy 3.1e7 -> greedy+swap 2.8e7 -> optimal 2.5e7 ton-miles -- greedy+swapping captures half the optimality gap with a sparse plan, so it "might be hard to beat"; the consolidated-optimal must justify itself against greedy+swap, not just against the spread optimal. RESOLVED 2026-07-07 by (a)-via-swaps ("swap-as-pivot", Ben's pick): `purifyPlan` in swap.{hpp,cpp} -- opening improving pass, then best-saving ARC-COUNT-REDUCING pivots (negative saving allowed within the ton-mile budget), with later improving pivots restricted to non-spreading ones (unrestricted ones can re-spread what a spending consolidation removed -- the A<->B cycle found during design). theta invariant by construction (2-exchange never touches S/R). GUI: "Purify (sparsify)" button on greedy AND optimal overlays (worker thread -- the spread optimal plan has thousands of arcs at O(arcs^2) per pivot), arcs count in the status line, optimal working copy now persists across mode toggles with Reset restoring the pristine cached solve. Remaining open: length-4 pivots only reach a pairwise-pivot local optimum, not a certified vertex; longer-cycle pivots or a true transportation-simplex crossover stay future work if observed sparsity disappoints. | M | F2 | done (2026-07-07; Ben's visual check pending) |
| F3 | **Interactive screen / tolerance explorer (optional).** Vary the k-cheapest / gap screen, tie-break epsilon, and tolerances at runtime and watch kept pairs, certificate rounds, and iterations — a visual companion to the E1 config controls and the C5/E4 performance study. | S | F1, E1 | todo |

### Phase G — Fleet extension: multi-vehicle, multi-asset (added 2026-07-07, user request)

Capability-scoping generalization (`doc/fleet-formulation.md`): K vehicle
types (weight cap T_k, area cap A_k, FRACTIONAL count N_k, speed v_k) over a
horizon H, moving A asset types (unit weight w_a, unit area s_a) with
per-(node, asset) supply/demand/priority. Per-type vehicle-mile budgets
B_k = N_k v_k H are DATA (not calibrated); vehicles circulate at every node
with charged deadheading; link weight AND area constraints couple cargo to
allocated vehicles. Everything continuous — still a linearly constrained
convex QP. Scope: data model + validation + generator + feasibility checker
+ greedy planner ONLY; the optimizer pipeline (reduction/flowlcp/flowplan/
flownewton/oracle) and the GUI stay single-commodity and untouched.

| ID | Task | Size | Depends on | Status |
|----|------|------|-----------|--------|
| G1 | **Formalize the fleet model.** `doc/fleet-formulation.md`: data, variables, constraints (asset balance/delivery, link weight, link area, circulation, per-type budgets), objective; findings G-F1..G-F6 (degenerate types rejected; constrained diagonal; charged deadheading; fleetScaleHint replaces suggestedLimit; conservative per-type loading; priority-weighted service order); Lemma FL1 (per-asset rationing separability, exact) and FL2 (out-and-back circulation, exactly zero under the per-arc-difference checker). Resolves the 2026-07-01 handoff's "vehicle modeling fork": mixed cargo under joint weight+area capacity, continuous throughout. | M | A1 | done (2026-07-07) |
| G2 | **waterFillTargets extraction.** Behavior-preserving refactor of `rationTargets`'s water-fill core into a shared `waterFillTargets(demand, priority, meetable)` (greedy.hpp), reused per asset column by G4. Existing suite re-run green (58/58) with unchanged values. | S | G1 | done (2026-07-07) |
| G3 | **Fleet data model + generator + checker.** `fleetinstance.{hpp,cpp}`: `AssetType`/`VehicleType`/`FleetInstance` (per-(node,asset) MatrixXd columns; shared distance matrix; horizon), validation, per-asset helpers, `vehicleBudget`; `FleetProfile` + `makeRandomFleetInstance` (geometry reused from `makeRandomInstance` — its cost matrix IS the distance matrix; independent xor-tagged rng stream for fleet draws; default 2 assets x 2 vehicle types engineered so weight binds on one asset and area on the other). `fleetplan.{hpp,cpp}`: `FleetPlan` (S, R, per-asset flow, per-type vehicle matrices), evaluation, and the NINE-family `checkFleetPlan` (assetBalance, delivery, capacity, negativity, idleResupply, linkWeight, linkArea, vehicleBalance, budget). `fleetinstance_test` + `fleetplan_test` (exact per-family perturbations). | M | G1 | done (2026-07-07) |
| G4 | **Greedy fleet planner.** `fleetgreedy.{hpp,cpp}`: `rationFleetTargets` (FL1 column-wise water-fill) + `greedyFleetPlan`: serve the largest priority-weighted fractional shortfall cell from its cheapest ROUND-TRIP source, spill across vehicle types best-unit-capacity-first, out-and-back deadhead legs (FL2), exact-zero assignment of every binding resource (targets, caps, budgets — the sliver-loop guard), unservable marking on budget exhaustion, iteration cap 3mA+K+2; reports per-type miles/utilization, unserved cells, shortfall, and fleetScaleHint via an unlimited-budget advisory pass. `fleetgreedy_test`: per-asset rationing vs single-commodity water-fill, exact small cases (weight-bound, area-vs-weight, exact type mixing with utilization EXACTLY 1 on the drained type), graceful starvation, random-profile feasibility + sandwich ordering. | M | G2, G3 | done (2026-07-07) |
| G5a | **Fleet optimizer: formulation + debt records.** fleet-formulation.md sections 9+ (reduced conservative QP; Lemma FL3 per-asset shortest-route reduction; Lemma FL4 path out-and-back circulation; no-overshoot argument; findings G-F7 conservative-vs-link-coupled, G-F8 K budget rows, G-F9 purify entry-usage cap); report Ch. 4 paragraph (conservative formulation chosen because the full link-coupled QP needs sparse solver machinery); on-disk memory + pending task for the sparse-machinery debt. User decisions 2026-07-07: conservative model; screen now / structured Newton later; purify saving >= 0 only. | M | G4 | done (2026-07-07) |
| G5b | **MCP derivation checked with Maxima.** doc/fleet-mcp-check.mac in the ns2-newton-check.mac style: jacobian(G) of the reduced-QP KKT map reproduces the designed M block layout entry-for-entry on a mixed fixture (2 assets x 2 sources x 2 sinks x 2 types, asset 2 capable on type 1 only -- the incapable combos get no variable); symmetric part = per-cell rank-one blocks (borders cancel); the quadratic form is EXACTLY sum over cells of Q (cell sum)^2 (PSD as a sum of squares); KKT identities at hand-solved unconstrained, budget-bound (lambda* > 0), and capacity-bound (mu* > 0) optima on exact rationals; tie-break type selection (dear-type slack = eps (rho2 - rho1) exactly); unpack identities. RUN LIVE (Maxima 5.49 at C:\maxima-5.49.0): ALL 9 CHECKS PASS. C++ (G5d) cites checks by number. | M | G5a | done (2026-07-07, Maxima-verified) |
| G5c | **fleetreduction.{hpp,cpp} + test.** Shared shortest routes on the distance matrix (computeShortestRoutes on a distance-as-cost view); per-asset REUSE of makeReducedProblem VERBATIM, fed a routes copy whose .distance is the round-trip matrix d-hat + d-hat^T (only .distance/.selfDistance are read, so the one-way successors in the copy are harmless) -- shipCost comes out as rho-hat with the screen logic untouched; kappa matrix; throws on assets with demand but no supply or no capable type. 4 tests. | M | G5a | done (2026-07-07) |
| G5d | **fleetlcp.{hpp,cpp} + test.** buildFleetLcp: z = [y (cell-major, capable types only) | mu (per supply cell) | lambda (K rows)]; defaultFleetTieBreakEpsilon = 1e-8 (sum P) / (sum B); unpackFleetLcp: outbound path walk for cargo + loaded vehicles, reverse path walk for deadheads (self pairs close their own loop; circulation exact per arc only on single-arc routes, to rounding on multi-hop loops). 5 tests: layout/skew/Q-block/eigen-monotonicity (SMALL profile -- the 70-node keep-all fixture at ~6,500 vars took tens of Debug minutes in the eigensolve and the numVars^2 assertion sweep, found as a hung first run), hand KKT at the Maxima check-4/5 optima, multi-hop round-trip unpack, reject-bad. | M | G5b, G5c | done (2026-07-07) |
| G5e | **fleetsolve.{hpp,cpp} + test.** solveFleetPlan mirroring flowplan.cpp: nondimensionalize (units, miles, budgets), per-asset screen (default maxSourcesPerSink = 6), certificate loop (gain vs min-over-types price), engine dispatch reused (ipm dense default). FleetSolveResult with per-type lambda_k. LOAD-BEARING TEST: A=1/K=1 equivalence with solveFlowPlan on a symmetric-distance fixture (cost = (d + d^T)/kappa, L = B, binding budget so the lambda comparison is nontrivial) -- PASSES, inheriting the oracle-validated single-commodity chain; plus certificate recovery of a screened-out source the optimum needs, sandwich + feasibility + shadow prices on a small random profile, reject-bad. "chain" engine and "flow" Newton factory deliberately not offered (G5h). | L | G5d | done (2026-07-07) |
| G5f | **purifyFleetPlan + test.** Per asset: purifyPlan (exists) on the round-trip-cost slice with tonMileCap = that asset's entry usage (saving >= 0 initially, never exceeding entry level, G-F9); the vehicle reallocator extracted from swapFleetToLocalOptimum into a shared helper; one rebuild at the end. Tied-routing fleet test: 4 -> 2 arcs at zero saving, deliveries bitwise unchanged, miles equal, exactly feasible. | S | G5e | done (2026-07-07) |
| G5g | **Fleet viewer: Optimal Fleet Plan + Purify.** Optimal radio, worker-thread solve (token guard, per-instance cache, NO calibration pre-pass -- budgets are data); the fleet window gains its own busy bar; working-plan/kind architecture (0 none / 1 greedy / 2 optimal) so swaps and purification persist across mode/asset toggles and Reset restores the cached plan; Purify (worker thread) enabled for greedy AND optimal; Swap to Optimum stays greedy-only; status shows live theta/utilization plus certified flags and per-type lambda_k for the optimal plan. Builds clean; all 34 fleet tests green. CAVEAT for Ben's run: the Optimal solve at the 4x3 GUI default is a ~2,500-dim dense-LU IPM -- expect noticeable Debug wall time (the busy bar covers it); Release is quick. A Debug-scale timing run was stopped before completing; the full ^Network suite gate is handed to Ben's run (fleet suites verified green 34/34 just before close). | M | G5e, G5f | done (2026-07-07; Ben's build+run pending) |
| G5h | **FUTURE WORK -- fleet structured Newton factory.** Per-cell Sherman-Morrison + dual Schur of size numSupplyCells + K; needs its own Maxima check; makes keep-all fast. Not this week. | L | G5e | todo (follow-on) |
| G5i | **FUTURE WORK -- sparse solver machinery for the full link-coupled fleet QP.** Sparse assembly + sparse factorization or matrix-free engine behind the InnerSolver interface (or an external QP solver), enabling the aggregate per-link weight/area coupling with mixed loading and explicit vehicle circulation (~(A+K)m^2 variables). Recorded debt from the G5a decision; memory `sparse-solver-machinery-needed`. | L | G5e | todo (follow-on) |
| G6 | **Fleet viewer (fleet_viewer).** Second Qt executable twinned from the network viewer: Vehicle types / Asset types spinners (1-10, defaults 3/4) drawing catalog prefixes (`assetCatalog`/`vehicleCatalog`, 10 fixed types each, first two = the FleetProfile defaults); same generation path as the fleet tests (makeRandomFleetInstance -> greedyFleetPlan, synchronous); modes None/Closest/Greedy Fleet Plan; top-right "Asset Displayed" spinner (1..A) slices the map/lists/histogram per asset via single-commodity Instance/Plan slices (distance matrix doubles as slice cost); node popups show full per-asset C/D vectors via a new FlowPlanView info-provider hook; FlowPlanView now keeps pan/zoom when the placement is bit-identical (asset scrolling does not refit). | M | G4, F1 | done (2026-07-07; Ben's visual check pending) |
| G8 | **Fleet swap improvement + viewer button.** `fleetswap.{hpp,cpp}`: `swapFleetToLocalOptimum(inst, plan)` drives each asset class in turn to a 2-exchange local optimum by calling the EXISTING single-commodity `swapToLocalOptimum` verbatim on a per-asset slice whose cost matrix is the ROUND-TRIP distance d_ij + d_ji (deadhead charged, G-F3; d_ii on the diagonal); deliveries/theta bit-invariant. Vehicles cannot swap along (u aggregates assets per link), so the vehicle matrices are REBUILT from the swapped flows by the greedy transport rule (best `unitCapacity` first within budgets, out-and-back, exact-zero drains) -- `unitCapacity` promoted from fleetgreedy's anonymous namespace to a shared fleetinstance function (G-F5). Order-pathological reallocation overflow throws (defensive; an allocation always exists since per-asset unit-round-trip-miles only shrink). `fleetswap_test` (4): exact uncrossing with vehicle rebuild, per-asset turn-taking, fixed point, random greedy plan improved with budgets/feasibility held and resupply bitwise unchanged. Fleet viewer gains a "Swap to Optimum" button (greedy plan mode only; popup shows swaps per asset + vehicle-miles before/after; utilization/miles bookkeeping refreshed). | M | G4, G6 | done (2026-07-07; Ben's visual check pending) |
| G7 | **Fleet distance matrix: bare Euclidean + jitter.** Replace the inherited base cost model (100-mile floor + 1.35x scale) with bare Euclidean separation times an independent per-direction U[1, 1 + distanceJitterMax] multiplier (default 5%, so d_ij/d_ji within [1/1.05, 1.05]); d_ii ~ U[selfDistanceLo, Hi] (default 1-5 mi); 1-mile min-separation guard for coincident placements; drawn AFTER the C/D/P draws so a seed's supply/demand pattern is unchanged. FleetProfile gains distanceJitterMax + selfDistance band (validated). Effect on the 20260704 1x1 case: delivered 5,753 -> 14,578 u, avg round trip/unit 450 -> 178 mi, fleetScaleHint 4.32 -> 2.34, budget still 100% used. | S | G3 | done (2026-07-07) |

### Phase D — Technical report (LaTeX, 20-50 pp)

**Phase D is discharged by the LIBRARY-WIDE report at `doc/report/`**
(2026-07-06 decisions-log entry): the scope grew from the network project
alone to the whole VINCP library (engines, GAMS games, compositions), so
the report is four parts at ~43 pp with the network material woven through.
Mapping of the old boxes onto the report: D1 -> Part I (Overview);
D2 -> Part II (Developer Manual, network layer in its own section);
D3 -> Part IV (Mathematical Foundations; flow QP transcribed from
formulation.md/reduction.md at the cited-proof standard);
D5 -> Part III (Testing and Lessons Learned; banded QP and deploy as
equal-weight case studies); D4 -> the assembly pass (terminology,
cross-refs, number consistency). Remaining under Phase D: Ben's review of
Parts I (and any re-reads), plus items the report defers to future data
(IP4b comparative rows; F2 optimal-overlay visual evidence for the testing
chapter).

| ID | Task | Size | Depends on | Status |
|----|------|------|-----------|--------|
| D1 | **Skeleton + Part I (management overview).** Three-part LaTeX scaffold; Part I: problem, solution approach, challenges (scale -> reduction; degeneracy; budget calibration). | M | A1 | drafted (report Part I) |
| D2 | **Part II (developers' manual).** Data structures, APIs, block layouts, expected usage walkthrough (generate -> greedy -> reduce -> solve -> unpack), build instructions. | M | C4 | done (report Part II; Ben-approved) |
| D3 | **Part III (mathematical appendix).** Full formalization; algorithm specification; proofs: convexity + existence/uniqueness of `R*`, reduction lemma, KKT <-> mixed LCP equivalence, monotonicity, convergence of the projection-contraction method (cited + conditions verified). Now also: chained-solver rationale (SS global convergence + the O(1/sqrt(k)) tail analysis motivating the chain). | L | A3, C4, E4 | drafted (report Part IV; Ben-revised) |
| D5 | **Part IV (testing appendix; added gate 15 at user request).** How thoroughly the result was tested: known-solution unit tests; brute-force cross-checks (Floyd-Warshall vs path enumeration); the INDEPENDENT oracle (different formulation AND solver, validating Lemma R1 end to end); hand-derived KKT points pushed through `M z + q`; R3 certificates as per-run optimality proofs; the sandwich bounds; feasibility checkers (incl. the F1 shortcut test); the SS calibration story (honest failure -> globalization-grade bars); benchmark methodology; the visual evidence from the Phase-F viewer/overlay. | M | C3, C5, E4, F1 | drafted (report Part III; Ben-approved; viewer visual evidence pending F2) |
| D4 | **Assembly and final pass.** Merge, cross-reference, numbers from C5, page-count check (20-50), consistency read. | M | D1, D2, D3, D5, C5 | done 2026-07-06 (terminology/cross-ref/number sweep) |
| D6 | **Testing-chapter section on the network GUI.** Explain the viewer's main layout (instance panel, Show Links modes, swaps/purify group, busy bar, rankings lists, status line) with a large figure `doc/images/initial-nw-gui.png` (image present as of 2026-07-07). Prose per `../../doc/style-instructions.md` (standing instruction for all written products, Ben 2026-07-07). SCOPE GREW at Ben's request to a full "Visual validation" section (`sec:t-viewers`, part3.tex after case study 1): network viewer walkthrough against the figure; re-implementation-grade pseudo-code (target: Java 17) for waterFill + greedyPlan, bestSwap + swapToOptimum, and the budget-limited fleetGreedy; the optimizer behind the Optimal overlay named; the tiny-flows explanation (interior point -> analytic center vs edge-following -> forest-sparse vertex) and the purify pass; the fleet viewer with asset/vehicle class definitions. First figure in the report (43 -> 51 pp). | S | D5, F2 | done (2026-07-07; Ben's read pending) |

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
- 2026-07-06: NS1 code done. `mehrotraIpm` gains the per-iteration
  linear-algebra seam (OOQP layering): `NewtonSolve` / `NewtonSolverFactory`
  types in mehrotraipm.hpp; trailing `newtonFactory` parameter (empty =
  built-in dense-LU factory, historical behavior bit for bit — the retry
  path assembles the identical matrix). The factory contract carries the
  singularity-rescue protocol via `freeRegularization` (0.0, then sticky
  regEpsilon after a non-finite solve). `MehrotraIpmParams.newtonCheckTol`
  (default 0 = off) is the dev-mode drift guard: each Newton solve verified
  against the engine's own M by one O(dim^2) matvec, throwing on violation
  (catches a factory whose K disagrees with M). New tests in
  mehrotra_ipm_test: counting dense wrapper matches the default EXACTLY
  (equal z/iter/residual; one factorization per iteration; two solves each;
  reg 0 throughout); rescue protocol traced on the singular-free-block QP
  (retry call with regEpsilon in iter 0, sticky after, 2*iter+1 solves);
  honest solves pass the drift guard (incl. regularized iterations); an
  echo factory is caught by name; empty NewtonSolve refused; negative
  newtonCheckTol rejected. No new files (no CMake reload). AWAITING Ben's
  build+run; then NS2 (makeFlowNewtonFactory, algebra machine-verified by
  ns2-newton-check.mac).
- 2026-07-06: NS1 first run: 90/93 green (all pre-existing tests unchanged --
  the seam's bit-for-bit acceptance held, incl. the exact-match seam test);
  the 3 failures were all NEW tests, all test-construction bugs, fixed.
  FINDING (verified by a standalone Eigen probe): on the singular-free-block
  QP the rescue NEVER fires -- Eigen's partial-pivot LU leaves an exact zero
  pivot but the flat coordinate's rhs entry is STRUCTURALLY zero (row+column
  of M zero; nonzero q would make the free rows infeasible), and a zero
  pivot with an exactly-zero numerator yields 0, not NaN, so the solve stays
  finite. The old RegularizationRescuesSingularFreeBlock test had therefore
  never exercised the rescue (renamed SingularConsistentFreeBlockConverges,
  honest comment; header docs tightened: rescue triggers on NON-FINITE
  solves, not on singularity per se). Fixes: rescue-protocol seam test now
  drives the rescue deterministically via a fail-first factory (first
  factorization returns NaN, then delegates to the honest dense solve) and
  checks the reg-value sequence 0, regEpsilon, regEpsilon, ...; the
  wrong-factory and empty-solver tests used M = I, q = -1 whose solution IS
  the interior start (converged in 0 iterations, factory never called) --
  q flipped to +1. AWAITING Ben's re-run.
- 2026-07-06: NS1 gate-verified by Ben (93/93 green). NS2 code done.
  `network/flownewton.{hpp,cpp}` (in vincpnet): `makeFlowNewtonFactory(lcp)`
  returns a NewtonSolverFactory for mehrotraIpm that solves
  K = M + diag(sOverY) by structure — per-sink Sherman-Morrison inverse of
  the rank-1-plus-diagonal t-block (O(k_n) per slice) and an SPD dual Schur
  complement of size numSources+1 (two-part assembly, LLT) — transcribed
  verbatim from the Maxima-verified algebra (ns2-newton-check.mac, cited in
  the header). O(pairs) + tiny LLT per iteration vs dense dim^3. Rejects
  nonzero freeRegularization (pure NCP), wrong-size/non-positive sOverY,
  inconsistent lcp; LLT failure throws. Wiring: `FlowPlanParams.ipmNewton`
  = "dense" (default) | "flow" + `newtonCheckTol` passthrough; config keys
  `solver.ipmNewton` / `solver.newtonCheckTol`; factory rebuilt per
  certificate round. `LLT` added to vincp.hpp's Eigen imports. Tests
  (`flownewton_test`, + config_test key coverage): dense-LU parity keep-all
  (direct 1e-8 + backward-error 1e-12 on the SCALED system), extreme
  1e-6..1e6 diagonal (backward-error metric), k_n = 1 slices, extreme Q_n
  spread, end-to-end solveFlowPlan ipm dense-vs-flow parity with the
  engine's newtonCheckTol drift guard ON, guard cases. `ns3-keepall.cfg`
  staged for NS3 (200-banded keep-all, ipm + flow Newton, iterMax 200).
  AWAITING Ben's build+run (CMake reload: new files).
- 2026-07-06: NS2 gate-verified by Ben (99/99 green, incl. the 6 new
  flownewton tests). NS3 run and CLOSED the same day: 200-banded keep-all
  (ns3-keepall.cfg, seed 20260704, Release) — kept 14500/14500, 0 rounds,
  36 iterations, **4,513.9 ms**, certified YES, th_star 52.90758, dlv
  0.959, tm/tmG 0.800, lambda 2.22e-06. Versus IP4a (screened, dense LU,
  same seed): ~394x faster AND exact — the one-round screened answer had
  overshot the optimum by ~0.4% (53.117). Iteration count 36 at Newton dim
  14,601 (vs 35 at 10,838) confirms dimension-insensitivity. VERDICT
  (performance.md addendum P6-P9): for banded/large instances the
  production configuration is now engine ipm + ipmNewton flow + NO screen
  (exact by construction, no certificate machinery); small laydown-0
  instances keep the screened bsHe94b path. E4's success criterion
  ("200-banded solves in minutes with certified results") is exceeded by
  ~two orders of magnitude via the NS track. IP4b weekend controls are now
  purely comparative (they gate nothing); E3b (smoothing-Newton hybrid)
  is moot for this problem class.
- 2026-07-06: Phase D discharged by the library-wide report `doc/report/`
  (main.tex + prolog.tex style module + part1-4.tex + refs.bib; ~43 pp,
  pdflatex+bibtex). Scope grew beyond this network project to the whole
  VINCP library, so the old three-part network report became four parts:
  D1 -> report Part I, D2 -> Part II (Ben-approved), D3 -> Part IV
  (Ben-revised: regular chapter, Maxima citations, peer application
  subsections), D5 -> Part III (Ben-approved; banded QP and the deploy
  game as equal-weight case studies), D4 -> assembly pass done (first-use
  terminology definitions incl. natural residual, cross-ref audit, number
  consistency, 0 undefined refs / 0 overfull lines). Ledger rows updated
  above. Still pending under D: Ben's Part-I review; IP4b comparative rows
  and F2 viewer visuals when they exist.
- 2026-07-06: F2 optimal overlay CODED (awaiting Ben's build+run; CMake
  reload -- gui/CMakeLists gained Qt6::Concurrent). New "Optimal Plan" radio
  in network_viewer: solveFlowPlan(engine=ipm, ipmNewton=flow, keep-all,
  iterMax 200) on a QtConcurrent worker thread (the gravity-swap freeze
  lesson), budget calibrated by a greedy pre-pass (suggestedLimit) exactly
  as the benchmark pipeline; result cached per instance, stale results
  discarded by a solve token when Regenerate races a solve; status line
  shows certified/converged flags, theta*, budget, lambda; swaps disabled
  (nothing to improve); rankings/status via the shared working-plan
  pattern (workingKind_ 3).
- 2026-07-07: Phase G (fleet extension) added and G1-G4 done in one pass
  (user-approved plan, Claude-run build+tests this time; Ben's own run still
  welcome). Model per `doc/fleet-formulation.md`: K vehicle types (T_k, A_k,
  fractional N_k, v_k), A asset types (w_a, s_a), per-(node,asset) C/D/P,
  ONE shared distance matrix, budgets B_k = N_k v_k H as DATA, circulation
  with CHARGED deadheading, link weight+area coupling. User decisions
  recorded: greedy+data-model only (optimizer untouched, G5 future);
  per-(node,asset) priorities; single distance matrix; deadhead miles count.
  Design decisions: suggestedLimit inverts into fleetScaleHint (G-F4);
  conservative per-type loading kappa_ak = min(T_k/w_a, A_k/s_a) (G-F5);
  service order = priority-weighted fractional shortfall since budgets can
  bind (G-F6); diagonal constrained (G-F2). FINDING (FL2 numerics): the
  greedy's out-and-back u is exactly symmetric, but Eigen's contiguous
  col().sum() and strided row().sum() associate differently, leaving ~2e-15
  circulation residue on identical values — checkFleetPlan therefore
  accumulates PER-ARC differences u_ij - u_ji (each exactly 0 for symmetric
  u), and vehicleBalance asserts EXACT zero in tests. New files:
  include/fleet{instance,plan,greedy}.hpp + lib mirrors + three test suites
  (15 tests); CMake: 3 lib sources + 3 vincpnet_add_gtest rows. greedy.cpp
  refactor is the ONLY touch to existing code. Full network suite 73/73
  green (58 pre-existing unchanged + 15 new) in a fresh build-fleet/ tree
  (Ninja + MSVC, Debug).
- 2026-07-07: F4 done -- "swap-as-pivot" purification (Ben chose crossover-
  via-the-swap-engine over the consolidation heuristic after a techniques
  review: the optimal face of the flow polytope has forest-sparse vertices,
  the IPM parks at its analytic center, and the 2-exchange IS a
  transportation-simplex cycle pivot). `positiveArcCount` + `PurifySummary`
  + `purifyPlan(inst, plan, tonMileCap)` in swap.{hpp,cpp}: opening
  unrestricted improving pass; then repeatedly the best-saving pivot that
  strictly reduces the positive-arc count, accepting NEGATIVE savings within
  tonMileCap (theta is invariant under any 2-exchange, so budget slack may
  be spent on sparsity); improving pivots after consolidation begins are
  restricted to non-spreading ones. DESIGN FINDING: the naive alternation
  cycles -- a spending consolidation re-enables the improving pivot that
  re-spreads it (A<->B forever); the non-spreading restriction makes the
  arc count monotone after the opening pass, which is the termination
  proof. Tests (swap_test +4): tied-routing 4->2 arcs at zero saving;
  improving-first ordering; spend-only-within-cap accept/refuse pair;
  random greedy plan purified with theta bit-identical. GUI (network
  viewer): "Purify (sparsify)" button, enabled for greedy (uncapped) and
  optimal (cap = calibrated budget) overlays, running on a QtConcurrent
  worker with token+kind staleness guard (the spread optimal plan is
  O(thousands) of arcs, O(arcs^2) per pivot -- gravity-freeze lesson);
  "arcs: N" line in the plan status; the optimal working copy now PERSISTS
  across mode toggles (was: re-copied from cache each switch) so purification
  sticks, Reset restores the pristine solve, and the "(+N pivots, saved X)"
  head now covers the optimal overlay too. Suite 77/77 green (Claude-run,
  Debug, build-fleet/); network_viewer builds; Ben's visual check of the
  purified overlay pending (expected: arcs drop toward #sources+#sinks-1,
  ton-miles <= budget, theta* unchanged in the status line).
- 2026-07-07: viewer polish (Ben): closest-links spinner now STARTS at 5 --
  root cause was the construction-time setRange(0, 0) clamping the initial
  value to 0 before regenerate() widened the range (mainwindow.cpp:155);
  plus a busy bar between the Swaps and Display frames that refills on a
  50 ms timer while any unknowable-duration background operation (optimal
  solve, purify) runs -- start/stop counted (startBusy/stopBusy) so
  overlapping operations keep it alive until the last one lands.
- 2026-07-07: G6 fleet viewer done (Ben's clarified spec: catalog types /
  second executable / None+Closest+Greedy / filter-everything-by-asset).
  New: `assetCatalog`/`vehicleCatalog` in fleetinstance (10 fixed types
  each, prefix-ordered so small counts mix weight- and area-bound; first
  two entries are the FleetProfile defaults; +1 fleetinstance test);
  gui/fleetmainwindow.{hpp,cpp} + fleet_gui.cpp + CMake fleet_viewer
  target. Reuse strategy: FlowPlanView/CostHistogram/NodeListWidget are
  single-commodity widgets, so the fleet window feeds them per-asset
  SLICES (Instance slice: C/D/P columns + distance-as-cost + geometry;
  Plan slice: flow[a] + S/R columns) and re-slices when "Asset Displayed"
  moves. Two surgical FlowPlanView changes shared with the network viewer:
  a node-info PROVIDER hook (fleet popups show label + full C/D vectors,
  one entry per asset) and setInstance now PRESERVES pan/zoom when the
  placement is bit-identical (asset scrolling no longer refits; also means
  same-seed regenerates keep the view). Status line shows the displayed
  asset's arcs/supply/demand/delivered plus whole-plan objective, fleet
  scale hint, and per-type utilization. Suite 78/78 green; both viewers
  build. Pending new tasks logged: G7 (bare-Euclidean fleet distances,
  U[0,5]% per-direction jitter) and D6 (report section on the network GUI
  layout with doc/images/initial-nw-gui.png).
- 2026-07-07: fleet-greedy "incompleteness" report ANALYZED -- no bug. On
  Ben's case (seed 20260704, 20/20/30/0, 1 asset x 1 vehicle) the planner
  delivered 5,753 of 21,463 rationed units with utilization EXACTLY 1.0000
  and fleetScaleHint 4.317: the 40-truck catalog fleet's 129,600
  vehicle-miles x kappa 20 u/vehicle / 450-mi avg round trip IS 5,753 u --
  delivery equals physical fleet capacity to four digits. The basic greedy
  ignores its budget by design (it calibrates L); the fleet greedy obeys
  budgets as DATA, so parity holds only when the fleet does not bind.
  Verified by Ben's proposed test: an "Unlimited fleet" checkbox in the
  fleet viewer (x1000 vehicle counts -> budgets moot); with it the plan
  delivers 21,460.5 u = the scaled rationed targets (basic-greedy parity),
  unmet 1,699 u = pure supply scarcity, and the unlimited run's 559,466
  vehicle-miles / 129,600 = 4.317 = the hint, exactly.
- 2026-07-07: G7 done -- fleet distances are now bare Euclidean x
  independent per-direction U[1, 1.05] (FleetProfile.distanceJitterMax),
  d_ii ~ U[1, 5] (selfDistance band), 1-mile min-separation guard, drawn
  from the fleet stream AFTER the C/D/P draws (seed's supply/demand pattern
  bit-identical before/after -- confirmed on the 20260704 case). The base
  generator's cost matrix is no longer used by the fleet path. Tests:
  determinism test now checks separation/jitter bounds + near-symmetry
  ratio instead of distance == base.cost; fleet-formulation.md section 1
  records the generated-distance model. Same-case effect: delivered 5,753
  -> 14,578 u, hint 4.32 -> 2.34, mean off-diagonal distance 821 -> 532 mi.
- 2026-07-07: G5 (fleet optimizer) planned and G5a-G5f executed in one
  evening session (user-approved plan; three decisions: CONSERVATIVE model
  matching the greedy's kappa/out-and-back conventions, with the full
  link-coupled QP recorded as sparse-machinery debt (G5i + memory); screen
  now / structured Newton later (G5h); fleet purify saving >= 0 only,
  per-asset entry-usage caps). Pipeline mirror: fleetreduction (reuses
  makeReducedProblem verbatim on a round-trip routes copy) -> fleetlcp
  (z = [y | mu | lambda_K], cell-major, Maxima-verified: fleet-mcp-check
  .mac ALL 9 CHECKS PASS, run live on Maxima 5.49) -> fleetsolve
  (nondimensionalize incl. budgets-through-counts, per-asset screen,
  K-budget certificate pricing gain vs min-over-types price, engines
  ipm/bshe94b/ssn) -> purifyFleetPlan. LOAD-BEARING: the A=1/K=1
  equivalence test agrees with solveFlowPlan (shortfall, deliveries,
  mileage, shadow price at a BINDING budget), inheriting the
  oracle-validated single-commodity chain. LESSON: the first
  LayoutAndMonotonicity fixture used the 70-node keep-all LCP (~6,500
  vars); the Debug eigensolve + numVars^2 EXPECT sweep effectively hung
  ctest and had to be killed -- structural tests belong on small fixtures.
  Report Ch. 4 gains the conservative-formulation paragraph (52 pp,
  clean); fleet-formulation.md gains sections 9+ (FL3/FL4, G-F7/8/9).
- 2026-07-07: standing instruction (Ben): all written PROSE products
  (report sections, docs, formal writeups) follow doc/style-instructions.md
  at the visolver root. doc/images/initial-nw-gui.png for D6 is present.
- 2026-07-07: G8 done -- fleet swap improvement. Reuse per Ben's directive
  ("the already-written swap function should work: do not re-write it"):
  swapToLocalOptimum is called VERBATIM per asset on a round-trip-cost
  slice; the only extraction needed was unitCapacity (fleetgreedy anonymous
  namespace -> shared fleetinstance function) for the vehicle reallocation,
  which rebuilds u from the swapped flows with the greedy transport rule.
  On the crossed hand case: 1 swap, vehicle-miles 20 -> 2, all families
  exactly 0. On the random default profile: swaps fire, budgets held,
  resupply/theta bitwise unchanged, total vehicle-miles non-increasing.
  Viewer: "Swap to Optimum" button (swaps each asset class in turn, per
  Ben's spec), enabled only when the greedy fleet plan is shown. Suite
  82/82 green; both viewers build.
- 2026-07-07: D6 done -- report section 3.5 "Visual validation: the
  flow-plan and fleet viewers" (part3.tex, after case study 1), written to
  doc/style-instructions.md register. Contents per Ben's spec: network GUI
  walkthrough against doc/images/initial-nw-gui.png (the report's FIRST
  figure); pseudo-code at Java-17 re-implementation depth for the greedy
  planner (waterFill + phase-2 loop), swap-to-optimum (2-exchange), and
  the budget-limited fleet greedy (kappa loading, round-trip sources,
  unservable marking, exact-zero discipline flagged as REQUIRED for
  termination, not style); optimizer behind the Optimal overlay stated
  (generation-3 pipeline: reduction -> KKT mixed LCP -> Mehrotra IPM with
  flow Newton, keep-all, budget = 0.8 x greedy); tiny flows explained as
  the analytic center of the optimal flow face (interior-point property)
  vs the forest-sparse vertices an edge-following method would return;
  purify described as in-face pivoting to a corner (theta bit-invariant);
  fleet viewer explained with asset-class (w_a, s_a; per-(node,asset)
  C/D/P) and vehicle-class (T_k, A_k, fractional N_k, v_k; B_k = N_k v_k
  H as DATA) definitions. pdflatex x2 clean: 51 pp, all references
  resolved, 0 overfull boxes. AWAITING Ben's read.
- 2026-07-07 (session close): G5g done -- fleet viewer gains Optimal Fleet
  Plan (worker thread, cache, token, own busy bar), Purify for greedy and
  optimal working plans, Reset, and live status (theta, utilization,
  certified + lambda_k for optimal). All 34 fleet tests green at close.
  HANDED TO BEN: (1) CMake reload (new files fleetreduction/fleetlcp/
  fleetsolve + tests), build network_tests + fleet_viewer; (2) full
  `ctest -R "^Network"` gate (only the fleet subset was re-verified after
  two long background runs were stopped); (3) visual check: Optimal Fleet
  Plan on the 4x3 default (expect a spread of hairline flows; the Debug
  solve takes a while -- watch the busy bar; Release is quick), then
  Purify (arcs collapse, theta and budgets unchanged), Reset restores;
  (4) optional Release timing. G5h (structured fleet Newton) and G5i
  (sparse machinery for the full link-coupled QP) remain the recorded
  follow-ons for after the week's pause.

<!-- Copyright Ben Paul Wise. All Rights Reserved. -->

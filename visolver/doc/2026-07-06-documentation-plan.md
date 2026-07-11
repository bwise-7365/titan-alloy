<!-- Copyright Ben Paul Wise. All Rights Reserved. -->
# Documentation plan: the VIMCP report (draft outline for review)

Drafted 2026-07-06 for Ben's review. Four parts per his framing: overview,
developer manual / API, testing and lessons learned, technical appendix for
OR professionals. This plan subsumes the network D-phase boxes (D1-D5 in
`network/plan.md`): Part I extends D1, Part II extends D2, Part III extends
D5, Part IV extends D3; D4 (assembly) applies to the whole.

Sources for every number and claim already exist: `network/doc/*.md`
(formulation, reduction, performance P1-P9, ns2-newton-check.mac),
`2026-07-06-engine-plan.md`, the test files themselves (header comments were
written to be citable), and the cross-session memories (the deploy eight-run
narrative and altchain rationale).

---

## Part I -- Overview (management / non-specialist; ~6-8 pages)

- **1.1 What this library is.** Solves mixed variational-inequality /
  complementarity problems `H(x,y) = 0, 0 <= G(x,y) _|_ y >= 0` over
  `K = R^n x R_+^m`; one problem type in (`VIModel`), one result type out
  (`VIResult`), interchangeable solver engines between them.
- **1.2 Why it exists.** The motivating trajectory: Octave prototype port ->
  logistics flow-planning QP -> the engine-robustness program the hard
  instances forced. One paragraph per driving problem.
- **1.3 The problem portfolio.** A flock of minimal constructed-solution
  problems (random PSD LCPs, cubic VIs, degenerate LCPs, mixed QPs) plus
  three almost-real-world problems: (a) large banded network cost
  minimization, (b) strategic allocation of effort (Nash influence game),
  (c) pre-position-and-deploy (PPD) with interdiction. A larger, tougher PPD
  is expected.
- **1.4 Solution approach.** (a) Layered architecture: core types / engines /
  drivers / compositions, dependency arrows one way. (b) An engine MENU
  matched to problem class rather than one universal algorithm: projection-
  contraction for well-conditioned monotone problems, interior point for
  degenerate ones, structured linear algebra for scale, semismooth Newton
  for mixed nonlinear structure, engine composition for nonmonotone games.
- **1.5 Headline results.** The numbers, one table + a paragraph each:
  200-node banded keep-all (14,500 pairs) solved certified-exact in 4.5 s
  (~394x over the screened dense path, and 0.4% better objective); alloceff
  PATH equilibrium reproduced independently by two engine classes (182 ms /
  83 ms); deploy solved only by the alternating chain, which found TWO Nash
  equilibria; suite 111/111.
- **1.6 Challenges and answers (narrative).** Scale -> reduction/screening ->
  structure exploitation made screening unnecessary; degeneracy -> engine
  class change (central path); nonmonotonicity -> composition + perturbation.
- **1.7 Status and roadmap.** Comparative IP4b probes; the larger PPD;
  chooseEngine dispatcher; acceleration wrappers (roadmap Option 3);
  matrix-free M seam; JNI boundary.

## Part II -- Developer manual / API (engineers; ~14-18 pages)

- **2.1 Getting started.** Build on Windows/Debian (CMake, header-only Eigen,
  `EIGEN3_INCLUDE_DIR`), running tests (ctest, aggregate targets, CLion
  notes), directory map.
- **2.2 Core types (`vimcp.hpp`).** `VIResult` -- including the two
  contract subtleties: residuals are SQUARED norms, and `converged=false`
  returns are honest stalls at real iterates (best-visited for ssn).
  `VIModel` / `makeVIModel` / `evaluateF`; `Projector` and ready-made
  projectors; `IterationLogger`.
- **2.3 Library-wide contracts.** Squared-norm convention (and why it is not
  to be "fixed"); the throw-vs-stall doctrine (bad values throw, stalls
  return); `invalid_argument` for caller errors vs `runtime_error` for
  numerical events; style/format pointers (.clang-format, STYLE.md),
  copyright banners.
- **2.4 Engine catalog.** One subsection per engine on a uniform template:
  problem class, signature, parameters, failure modes, cost profile, when to
  choose it.
  - `dHan06` -- self-adaptive projection; refactors per iteration (cost note).
  - `bsHe94b` -- fixed-metric projection-contraction; factor-once.
  - `solodovSvaiter` -- matrix-free hyperplane projection; GLOBALIZER only
    (O(1/sqrt(k)) tail).
  - `chainedSolodovHe` -- SS loose -> He warm-started.
  - `mehrotraIpm` -- mixed monotone LCP; degeneracy-insensitive; no warm
    start; `numFree` not `Projector`; stall-return semantics; the
    `NewtonSolverFactory` seam and `newtonCheckTol` drift guard.
  - `semismoothNewtonSolve` -- direct mixed NCP; `NcpFunctionPair` /
    `JacobianFn` seams; direction ladder; nonmonotone memory; best-visited
    return; line-search-only +inf on domain violations.
  - `solveVI` (Josephy-Newton driver) -- `InnerSolver` seam + adapters;
    Armijo damping; no-progress cutoff; FD Jacobian.
  - `alternatingChainSolve` -- `StageSolver` seams; rounds, best-point
    memory, throw-as-stall, perturb-restart; when to reach for it.
  - Building blocks: `armijo` (value-form + directional), `levenbergmarquardt`
    (primitives + solver), `fdjacobian`, `dampednewton`, `smoothingnewton`
    (and its relationship to the semismooth solver).
- **2.5 Choosing an engine.** A one-page decision table (monotone? affine?
  orthant K? warm start available? degenerate face? nonmonotone?) with the
  planned `chooseEngine` dispatcher as future work.
- **2.6 The network layer (`vimcpnet`).** Instance model and generators
  (laydown 0/1, node classes); greedy/gravity/swap planners; reduction and
  the sink-major LCP packing; screens and the certificate loop; `solveFlowPlan`
  and the config-file keys (`solver.engine`, `solver.ipmNewton`,
  `solver.newtonCheckTol`, screen keys); `makeFlowNewtonFactory`;
  `network_viewer` GUI; the benchmark harness and .cfg format.
- **2.7 Bringing a new problem.** The VIModel-builder pattern; the GAMS
  translation recipe as a worked method (variable packing in pairing order,
  H rows from =e= equations, G rows from =g=, initial point from .L levels,
  reference-solution checks); the `mcpengines.hpp` multi-engine test harness;
  the network generate->reduce->solve->unpack walkthrough.
- **2.8 Extending the library.** Adding an engine (which seams to implement,
  what tests are expected); adding an NCP function; adding a Newton factory;
  the JNI/row-major boundary caveat.

## Part III -- Testing and lessons learned (~10-14 pages)

- **3.1 Test philosophy and infrastructure.** GoogleTest via FetchContent;
  the `SolveFn`/`CheckFn`/`runCase` harness and its GoogleTest bridge;
  aggregate targets; taxonomy of the 111 tests (unit / protocol / acceptance).
- **3.2 The minimal problems.** What each constructed-solution family gates:
  PSD vs deliberately indefinite LCPs (the divergence guard AS a feature),
  cubic VIs through the JN loop, ellipsoid-K cases (Projector generality),
  degenerate/rank-deficient LCPs (the IPM's raison d'etre), mixed hand QPs
  (exact-solution Newton signature), kink-exercising cases.
- **3.3 Protocol testing (lesson).** Test protocols by injecting deliberately
  failing/fake components, not by hunting hard problems: the failing Newton
  factory (NS1 rescue), the frozen inner solver (JN stall cutoff), identity/
  throwing stages (chain). Includes the Eigen zero-pivot finding: a singular-
  but-consistent solve returns finite values, so rescue protocols trigger on
  non-finite results only -- a natural problem cannot drive them reliably.
- **3.4 Acceptance problem 1: banded network QP.** EQUAL WEIGHT with 3.6
  (Ben, 2026-07-06): the two are the report's paired case studies -- one
  monotone-at-scale, one nonmonotone -- and each gets the full narrative
  treatment. The 200-banded arc:
  projection-contraction capped at 150k iterations; IPM converged in 35
  iterations but dense LU + certificate balloon cost ~30 min; the structured
  factory solved keep-all in 4.5 s certified. Lessons: iteration count vs
  per-iteration cost are separate axes; the screened answer was 0.4%
  suboptimal (screens trade optimality silently); nondimensionalization is
  mandatory and exact.
- **3.5 Acceptance problem 2: alloceff (SAOE).** Literal GAMS MCP vs the
  reduced effort-space VI -- the MCP formulation is harsher on projection
  engines; ssn and jn+ipm both reach the PATH equilibrium; gate pinned to
  BOTH known-good rows so neither masks a regression.
- **3.6 Acceptance problem 3: deploy (PPD).** The eight-run narrative as a
  case study (equal weight with 3.4): all standalone engines fail ->
  failure modes are complementary -> single-shot chain (19.6 -> 0.68) ->
  throw-as-stall hardening -> percentage improvement rule proved brittle ->
  digit-identical reruns exposed the deterministic round-map fixed point ->
  scaled perturb-restart -> convergence, twice, to two distinct equilibria.
  The residual-breakdown diagnostic (worst rows by GAMS name) as the tool
  that localized each obstruction.
- **3.7 Cross-cutting lessons.** Each with its evidence pointer:
  (a) engine CLASS beats engine tuning on degenerate faces;
  (b) structure exploitation beats screening at scale;
  (c) composition beats any single engine off-monotone;
  (d) stalls are not errors -- honest converged=false, and throws cost
      composability;
  (e) return the best-visited point under nonmonotone searches;
  (f) determinism makes verbatim retries worthless -- perturb scaled to the
      error;
  (g) observability: flushed heartbeats (buffered-stdout incident), named
      residual breakdowns, honest flags;
  (h) pin test gates to verified answers; any-of gates + a growing roster
      for multi-equilibrium problems;
  (i) process: gate-by-gate review with owner-run builds; machine-checked
      algebra (Maxima) before code; literature-grounded engine choices.
- **3.8 Known limitations and open items.** IPM cannot warm-start; dense M
  memory (matrix-free seam is the noted generalization); dHan06 refactor
  cost; SAOE equilibrium-selection stays open (chain is the candidate);
  IP4b comparative rows pending; the larger PPD.

## Part IV -- Technical appendix for OR professionals (~18-25 pages)

- **4.1 Problem statement and geometry.** Mixed VI over `K = R^n x R_+^m`;
  natural map and residual; solution concepts; monotone / pseudomonotone /
  nonmonotone classes; degeneracy and non-strict complementarity.
- **4.2 Projection-contraction methods.** He 1994b fixed-metric (eq. 16) and
  Han 2006 self-adaptive; convergence conditions (PSD); why contraction
  rates approach 1 on near-degenerate faces (the observed stall, explained);
  the beta0 = 1 metric note.
- **4.3 Solodov-Svaiter hyperplane projection.** Global convergence under
  pseudomonotonicity; the O(1/sqrt(k)) tail when ||F(x*)|| > 0 -- globalizer,
  not finisher; the E3a chain rationale.
- **4.4 The Josephy-Newton driver.** Linearized affine VI over the SAME K
  (no Schur elimination -- the projector carries the structure); Armijo on
  the natural merit (undamped period-2 chatter); the inner-tolerance floor
  on the outer residual; 4th-order FD Jacobian design; the no-progress
  cutoff.
- **4.5 Mehrotra predictor-corrector for the mixed monotone LCP.** The
  algorithm as implemented (infeasible start, centering exponent, corrector,
  fraction-to-boundary, sticky free-block regularization, stall return);
  mu -> 0 conditioning (benign, per Wright); why the central path is
  insensitive to degenerate optimal faces. Citations: Potra-Wright, Gondzio,
  Gertz-Wright OOQP, Potra-Liu.
- **4.6 Structured Newton solve for the flow LCP.** The NS2 mathematics:
  sink-major block structure, per-sink Sherman-Morrison on the rank-1
  t-blocks, thin borders, skew-cancellation => SPD dual Schur (LLT),
  complexity accounting (O(pairs k) vs dim^3); the Maxima machine-check
  methodology (and its hygiene pitfalls); backward-error parity bars vs
  forward error and cond(K).
- **4.7 Semismooth Newton for the mixed NCP.** Penalized Fischer-Burmeister
  (CCK) and why plain FB fails; C-subdifferential diagonals and the kink
  rule -- including the full-z-space indicator subtlety; overflow/
  cancellation-stable evaluation (SEMI 4.1); direction ladder with LM under
  a local error bound (no nonsingularity assumption); directional Armijo,
  nonmonotone memory; the natural-residual stop (small-lambda pitfall);
  best-visited-iterate return.
- **4.8 The flow-planning QP.** Formulation with the delivery constraint
  (F1) and tie-break bound (R4); the reduction lemma; KKT <-> mixed LCP;
  monotonicity; screens and the R3 certificate (bounded suboptimality);
  budget regimes read from lambda; nondimensionalization as an exact unit
  change.
- **4.9 The two game formulations.** Alloceff: the literal MCP with free
  intermediates (exactness argument for treating GAMS Positive intermediates
  as free; the dropped eff upper bound justification). Deploy: survivor
  ratio-combat h(x) = x^2/(x + c) and its exact chain-rule gradients;
  mass-or-abstain curvature; the myEps poles; the h'(0) = 0 cold-start dead
  zone; multiplicity of equilibria.
- **4.10 The alternating chain, formally.** Round map definition; the
  complementary failure sets (indefinite linearizations vs merit geometry
  outside K); projection as handoff repair; best-point memory; stagnation as
  a fixed point of a deterministic round map; scaled perturbation and its
  PATH lineage; honesty guarantees (what is bounded and reported) vs what
  is NOT guaranteed (no global theory off-monotone); the equilibrium roster
  as an empirical sampling protocol; the mirror-twin cap observation.
- **4.11 Measurement conventions.** Squared residuals and cross-engine
  comparability; what `iter` counts per engine; wall-time attribution
  (factorizations vs iterations).
- **4.12 Annotated bibliography.** The verified open-access set (local
  copies in `doc/`), one line each on what the library took from it.

**Proof standard (Ben, 2026-07-06):** Part IV proofs are CITED with their
conditions verified against our setting -- not reproved in full. PROVISION:
if Part IV proves too hard to follow, a further appendix of Detailed Proofs
will be added, reconstructed from the Maxima results
(`network/doc/ns2-newton-check.mac`, the author's `v07_check.mac`) and the
derivation notes saved in the cross-session memory
`project_visolver_proof_notes.md` (kept current for exactly this purpose).

---

## Production decisions (recommendations, for Ben's approval)

1. **Format: LaTeX** (per the original D1 plan), one document, four parts,
   with Part-level `\include`s so parts can be drafted and reviewed as
   gates. Target ~50-65 pages total (the original 20-50 was for the network
   report alone; the scope has roughly doubled).
2. **Location:** `doc/report/` (new), keeping `network/doc/*.md` as the
   working sources it cites.
3. **Drafting order = review gates:** Part II first (closest to the code,
   fixes terminology), then Part IV (the hard math while fresh), then Part
   III (narratives), then Part I (written last, summarizing the others),
   then D4 assembly/cross-referencing pass.
4. **Numbers policy:** every performance figure cites its source
   (performance.md section, test log date, or commit) -- no unsourced
   numbers.
5. **The network `plan.md` ledger** gets an entry mapping D1-D5 onto this
   plan when approved.

<!-- Copyright Ben Paul Wise. All Rights Reserved. -->

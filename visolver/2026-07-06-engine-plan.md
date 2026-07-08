<!-- Copyright Ben Paul Wise. All Rights Reserved. -->
# Solver-engine plan: interior point + structured Newton seam (status board)

Living status board for the engine-roadmap work, created 2026-07-06 so the
plan stays visible beside the working dialog. Update statuses here as gates
close. Authoritative history stays in `network/plan.md`'s decisions log; the
deep detail lives in the cross-session memories (listed at the bottom).

## Why (one paragraph)

The 200-node banded flow-planning KKT-LCP stalls projection-contraction
(bsHe94b capped at 150k iterations with no answer). The 2026-07-05 literature
research produced three general-purpose engine options; Option 2, a Mehrotra
predictor-corrector interior-point method, is now built into the library.
IP4 showed the engine converges in tens of iterations regardless of dimension
or degeneracy; the remaining cost is the R3 certificate balloon times dense
dim^3 LU. The Newton-solver seam (NS gates) removes that with
structure-exploiting linear algebra, enabling a no-screen, no-certificate
solve of the full pair set — expected seconds, certified exact by
construction.

## Gate board

| Gate | What | Status |
|------|------|--------|
| IP1  | Pure-LCP Mehrotra engine `mehrotraipm.{hpp,cpp}` + degenerate / rank-deficient tests | DONE, verified 2026-07-05 (9 / 19 / 10 iters) |
| IP2  | Mixed free block + sticky `regEpsilon` rescue + QP tests | DONE, verified (4-6 iters vs bsHe94b's 57-60 on identical QPs) |
| IP3  | `makeMehrotraIpmSolver` seam adapter; third row in `han_vs_he_test`; network `solver.engine = "ipm"`; CLAUDE.md + ledger updates | DONE, verified 2026-07-05 |
| IP4a | 200-banded ipm bounded probe (1 certificate round) | DONE 2026-07-06 — results below |
| IP4b | `chain` + `bshe94b` bounded control probes | **DEFERRED to next weekend (with Opus)** |
| IP4c | performance.md addendum + close the gate in `plan.md` | DONE 2026-07-06 in substance — addendum P6-P9 (IP4a + NS3) written; IP4b rows will extend it but no longer gate anything |
| NS1  | `NewtonSolverFactory` seam on `mehrotraIpm` (default dense LU; `newtonCheckTol` drift guard) | DONE, gate-verified 2026-07-06 (93/93) |
| NS2a | Maxima algebra checker | DONE 2026-07-06 — `network/doc/ns2-newton-check.mac`, ALL 10 CHECKS PASS |
| NS2  | `makeFlowNewtonFactory`: per-sink Sherman-Morrison + dual Schur (SPD, LLT) + parity tests vs dense | DONE, gate-verified 2026-07-06 (99/99 green) |
| NS3  | Keep-all / no-screen 200-banded run; compare vs screened path and greedy+swap; addendum | DONE 2026-07-06 — 14500/14500 kept, 0 rounds, 36 iters, **4.51 s**, cert YES, th_star 52.908 (~394x vs IP4a's 1778 s uncertified 53.117); addendum P6-P9 in performance.md |
| MC1  | Select + design the specialized mixed-NCP algorithm (semismooth FB Newton, DFK/Munson line) | DONE 2026-07-06 — design below |
| MC2  | Implement it: `semismoothnewton.{hpp,cpp}` + directional Armijo + tests | DONE, verified 2026-07-06 (cubic 14 iters FD-path; degenerate 11 vs ipm 18; mixed QP 2 iters, residual exactly 0 after the kink-indicator fix) |
| MC3  | Big-affine exercise: `solver.engine = "ssn"` in solveFlowPlan + EngineSelectable row; ssn row joins the IP4b weekend benchmark; GAMS model (incoming) designated the big-NONLINEAR acceptance test | DONE, verified 2026-07-06 (four-engine EngineSelectable green) |
| SC1  | Alternating chain on the SAOE equilibrium-selection problem (`test/saoe_chain_test.cpp`; new `saoeModel`/`saoeDefaultStart` API in saoe.hpp): reference 6x10 gated on reaching E; expanded seeded 20x12 (U[-50,100] rewards, exactly 80 of 240 zeros, col >=1 pos/neg, row >=2 pos/neg, S ~ U[10,100], seed 20260706) gated on converge + feasible | BOTH GREEN 2026-07-06 (Ben's run). THE CHAIN REACHES E from the default start that always trapped the JN engines -- the library's oldest open item (SAOE equilibrium selection) is CLOSED. Expanded 20x12 converged feasible |
| DE1  | `chooseEngine` dispatcher (the last piece of the 2026-07-05 "all three options + a dispatcher" decision): `include/chooseengine.hpp` + `lib/chooseengine.cpp` — `probeMonotone` (Cholesky-attempt PSD probe), pure `chooseEngine(ProblemTraits)` decision table, `solveAffineAuto` (probe -> ipm/bshe94b/chain) and `solveModelAuto` (ssn first -> alternating-chain fallback on honest stall or runtime guard), `ChoiceLogger` observability hook; `choose_engine_test` (8 cases: probe classes incl. rank-deficient + skew, full decision table, both executor paths, forced fallback via ssnIterMax=1, guards) | DONE, verified 2026-07-06 (Ben's Debug run green; design + the two departures -- no degeneracy proxy, warm start as a declared flag -- approved). Roadmap Option 3 (AA/Halpern wrappers) is now the only unbuilt roadmap item |
| JN2  | Inexact-Newton FORCING SEQUENCE on solveVI (the Octave scripts' min(5e-4, n0/100) rule, generalized): `InnerSolverFactory` seam + factory overload, forcingCap/Ratio/Floor in JosephyNewtonParams (defaults match the Octave), shared solveVICore keeps the fixed-tolerance path bit-identical; `solveVIVanilla(model, z0)` one-call plain JN (bsHe94b inner under forcing, no basin control -- documented); 4 new josephy_newton_test cases (recorded-tolerance schedule check, vanilla solve, guards incl. empty-factory); report Part II (forcing + one-call paragraphs) + Part IV 4.4 (Dembo-Eisenstat-Steihaug / Eisenstat-Walker discussion, refs.bib entries) + CLAUDE.md; ALSO Part IV SAOE section gained the coalition-persistence paragraph (feature of the parliament formation problem, claim deliberately bounded per Ben) | CODE DONE 2026-07-06, awaiting Ben's build+run (plain rebuild) |
| FB1  | Forward-backward splitting engine `fbsHyz04` (He-Yuan-Zhang 2004, COAP 27:247-267; reimplementation of Ben's 5-year-old pmedemo iterateFBS with its adaptive secant-Lipschitz step): GENERAL monotone VI (VectorField F + any Projector; matrix-free, Jacobian-free), library conventions (VIResult, squared natural residual, guards); `makeFbsHyz04Solver` affine adapter; `fbs_hyz04_test` (monotone LCP, nonlinear cubic VI direct, ellipsoid K, guards, + the HISTORICAL SAOE PROBE: PME paper 4.4.6's pmedemo run found an INTERIOR equilibrium -- supports {4,6,9}, actor 1 SPLIT 12.8/53.2 across options 4 and 6 -- so all-or-nothing is NOT universal; probe gated converged+feasible and may honestly fail, that is the experiment). Report: Part II catalog subsection, Part IV section (sec:a-fbs), 4.8.2 provenance paragraph (small example = Leontief IO economy + GDP-neutral taxes; eps/precision differences noted), refs.bib hyz2004; "parliament formation" renamed POLICY ASSEMBLAGE per Ben (details to come). 45 pp, 0 undefined | CODE DONE 2026-07-06, awaiting Ben's build+run (CMake reload -- new files) |
| RA1  | Risk-averse SAOE (PME paper eq. 4.53 / tested Octave saoeJNra.m + esJ.m): `saoeModel(R, S, riskAversion)` steers by S_ij = R_ij(1 - alpha_i(R_ij - mu_i)), alpha_i = a/sigma_i under current probabilities (sigma=0 guard: alpha=0; a=0 branch skipped => exact identity); `saoePayoffVariance` helper; shared `test/saoesupport.hpp` extracted (chain runner + BOTH instance setups + the single-source-of-truth case list `saoeSharedCases()`; saoe_chain_test refactored onto it, per Ben: future setup changes reach every SAOE test automatically); `saoe_risk_test` iterates BOTH shared cases (reference 6x10 + expanded 20x12) -- criterion 1: a=0 G maps + chain results EXACTLY equal to risk-neutral; criterion 2: at a = 0.25 (the Octave fracRA) and 0.5, every actor's payoff variance (original R, equilibrium probabilities) <= the a=0 equilibrium's | REDESIGNED after run 1 (2026-07-06): cold-starting every a let the a=0.25 solve land in a DIFFERENT basin (5 of 6 variances rose, one fell -- basin hop, not a formula error; a=0.5 also stalled at 1e-8 grinding perturb rounds). Fix: criterion B is now a CONTINUATION -- each a > 0 warm-starts from the previous solution on the branch -- and every solve is labeled + timed (criterion A's "same answer, same cost" now visible). Awaiting re-run (plain rebuild) |
| MC4  | Translate the two GAMS models (`alloceff01cm.gms`, `deploy_v07.gms`) into multi-engine test cases (`gams_alloceff_test`, `gams_deploy_test` + shared `test/mcpengines.hpp` rows harness) | BOTH GREEN. alloceff 2026-07-06, gated on converge + match the PATH equilibrium (pinned to 2 rows). deploy SOLVED 2026-07-06 by the ALTERNATING CHAIN (round 4, residual^2 6.0e-15, 17.7 s; pr {RS3 .505, RS4 .495}, pb {BS2 .600 = rho cap ACTIVE, BS5 .400}, payoffs 58.07/60.72; quadratic tail visible; no standalone engine solves it). Ben confirmed the answer MATCHES GAMS; gate now PINNED on converge + match (checkGamsEquilibrium; standalone engines kept as documented failing comparison rows). MC track COMPLETE pending the larger PPD model (cooking in another session) |

Order of work when resuming: Ben rates the MCP track (MC1/MC2) **the most
important case**; it is independent of NS1-NS3 and of IP4b, so it can go
first or in parallel. Within tracks: NS1 -> NS2 -> NS3; MC1 -> MC2; IP4b
whenever benchmark hours are available.

## MCP track (MC1/MC2): direct solver for the mixed nonlinear complementarity problem

Target problem (the library's core case): H(x, y) = 0, 0 <= G(x, y) _|_
y >= 0 over K = R^n x R_+^m. The chosen family — roadmap Option 1 — exploits
exactly that K by compiling it into a nonsmooth equation instead of
projecting onto it: Phi(z) = [ H(x, y) ; phi_FB(y_i, G_i(x, y)) ] = 0, free
rows plain, orthant rows Fischer-Burmeister. This attacks the NONLINEAR
problem directly (no Josephy-Newton outer / affine inner nesting): per
iteration one generalized-Jacobian assembly H_k = [ J_H ; D_a + D_b J_G ]
(diagonal D_a, D_b from the FB kink rule) + one factorization + a DFK line
search on Psi = 1/2 ||Phi||^2, with gradient fallback and Levenberg-Marquardt
damping for degenerate solutions (error-bound theory, no nonsingularity
needed). Monotone bonus: every stationary point of Psi is a solution.

- MC1 (select + design): confirm the choice against the already-verified
  open-access set (De Luca-Facchinei-Kanzow 2000 "CombNCP"; Munson et al.
  "SEMI" — both Wuerzburg PDFs, URLs in the engine-roadmap memory — plus the
  penalized-FB and singularity-recovery variants they describe and
  arXiv:1703.07461 for LM-without-nonsingularity), with one targeted search
  for anything newer that is SPECIFICALLY about the mixed free/orthant
  structure. Deliverable: a short staged design (module name, params struct,
  Jacobian assembly from the existing FD Jacobian or analytic J, switching
  logic, test problems) appended here + to memory. Reuse inventory already
  known: VIModel/evaluateF (input shape), smoothingnewton's FB machinery,
  dampedNewton, levenbergMarquardt primitives, fdjacobian.
- MC2 (implement, gates within): the solver takes a VIModel directly (same
  input as solveVI) and returns VIResult. Tests: the han_vs_he cubic mixed
  VI (known solution) as a fourth solver row; a degenerate affine case
  cross-checked against mehrotraIpm/bsHe94b; the SAOE instance as the
  non-monotone stress case (feasibility-gated, like saoe_test).

### MC1 design (2026-07-06; confirmation search done, all sources verified open-access)

**Verdict.** Keep the DFK/SEMI semismooth Newton line; adopt the PENALIZED
Fischer-Burmeister NCP function (Chen-Chen-Kanzow 2000) as the default:
phi_lam(a,b) = lam*(a + b - sqrt(a^2+b^2)) + (1-lam)*max(0,a)*max(0,b),
lam = 0.8 (SEMI production default; never below 0.5). It fixes plain FB's
documented weaknesses (unbounded merit level sets for merely monotone F; too
flat in the positive orthant) and swept MCPLIB where plain FB failed.
Nothing 2005-2025 materially supersedes this for finite-dimensional mixed
NCPs at our scale (recent work is PDE-specialized; Kanzow's 2004 inexact
Krylov variant is a future large-scale option with a documented robustness
caveat). Full formulas, parameter guidance, and pitfalls: memory
`project_visolver_mcp_design.md`; sources: CCK chenchen.pdf, SEMI.pdf,
InSemiP.pdf (Wuerzburg), Ferris-Munson PATH TR 98-12 (UW-Madison).

**Module.** `include/semismoothnewton.hpp` + `lib/semismoothnewton.cpp`:
Phi(z) = [ H(x,y) ; phi(y_i, G_i(x,y)) ] = 0 solved by generalized-Jacobian
Newton with a three-tier direction ladder and directional Armijo on
Psi = 1/2||Phi||^2. Distinct from and complementary to `smoothingnewton`
(which needs a mu-continuation precisely because plain Newton stalls at the
exact-FB kink; the semismooth method works AT the kink).

**Seams (future-proofing; no bool flags where a third option is plausible):**
- `NcpFunctionPair { value; jacobianDiagonals }` — swappable NCP function
  with its C-subdifferential diagonals (da, db); ready-made:
  `penalizedFischerBurmeisterPair(lam = 0.8)` (default) and
  `fischerBurmeisterPair()` (the plain-FB restart option). The kink rule
  receives gz = J_G * (kink indicator) per CCK Algorithm 4.
- `JacobianFn` — analytic F' hook; empty default = 4th-order FD Jacobian.
- Direction ladder (fixed policy, each tier a reused primitive): LU on
  H_k d = -Phi; on non-finite solve OR failed descent test
  (grad_Psi . d <= -rho ||d||^p), Levenberg-Marquardt direction via the
  existing `levenbergMarquardtDamp`; last resort gradient step -grad_Psi
  (keeps DFK global convergence). Per SEMI, singular H_k is COMMON (12% of
  factorizations on MCPLIB), and the gradient tier alone is unreliable —
  hence LM in the middle.
- Nonmonotone line-search memory (SEMI m = 4) as an int param, default 1
  (= monotone, matching the citable DFK theory).

**Termination.** Iterate on Psi, but STOP on the library-standard SQUARED
natural residual ||z - P_K(z - F(z))||^2 < magTol (P_K = mixed projector).
This keeps VIResult::residual comparable across every engine AND avoids
SEMI's documented pitfall where a Psi-only stop under-enforces the sign
conditions for small lam.

**Proposed API additions (for Ben's review, per the future-proofing rule):**
1. `armijoLineSearchDirectional(meritAt, merit0, slope0, params)` added to
   `armijo.hpp` beside the existing value-only search: the gradient-form
   test phi(alpha) <= phi0 + c*alpha*slope0 the DFK theory is stated for.
   Existing callers untouched.
2. During the LINE SEARCH ONLY, a trial point where evaluateF throws
   (non-finite model value) is treated as merit +infinity, i.e. the step is
   backtracked away from domain violations (SEMI's handling). At ACCEPTED
   points non-finite values still throw. This is a documented, deliberate
   softening of throw-never-substitute for trial steps only.

### MC4: the GAMS acceptance tests (2026-07-06)

Both models arrived (not one): `sheridan/alloceff/alloceff01cm.gms` (influence
game, mixed NCP dim 87: free nfv/sigma/gamma intermediates + beta/eff
complementarity) and `sheridan/deployment/deploy_v07.gms` (two-sided
interdiction game with smooth ratio combat, dim 450: 4 free =e= multipliers +
446 nonneg). Translated 1:1 into `test/gams_alloceff_test.cpp` and
`test/gams_deploy_test.cpp`; each test file documents its GAMS -> VINCP
variable/equation mapping in the header. `test/mcpengines.hpp` is the shared
engine-rows harness (ssn / jn+bshe94b / jn+dhan06 / jn+ipm), so a problem
drops in as data + a VIModel builder. Both problems are NONMONOTONE with
multiple equilibria, so the gate is "at least one engine row converges"
(saoe_test's any-of pattern); each row prints stats plus the GAMS output
parameters (option probabilities / strategy probabilities and payoffs) for
eyeball comparison with the verified GAMS listings.

First run (Ben, 2026-07-06), alloceff: ssn 14 iters / 182 ms and jn+ipm
2 outer / 16 inner / 83 ms both converged, and BOTH landed on the reference
equilibrium (= saoe_test's E; Ben confirmed it is the one PATH reaches);
jn+bshe94b and jn+dhan06 threw their divergence guards immediately (expected
-- nonmonotone; notably this MCP formulation is harsher on the projection
engines than the reduced saoe formulation, where bsHe94b reaches E). The
alloceff gate was then UPGRADED per Ben: a row passes iff it converges AND
matches the PATH equilibrium's prob/expVal (kRefProb / kRefExpVal, tol 2e-3 /
5e-2), pinned at TWO passing rows (ssn and jn+ipm are the known-good pair).
deploy_v07 keeps the convergence-only gate until a run identifies which
equilibrium the engines reach.

deploy_v07 first Release run (Ben, 2026-07-06, heartbeat on): NO engine
converged -- the test is honestly RED and stands as the open acceptance
challenge for the nonmonotone engine work. ssn cut residual^2 1.42e5 ->
2.07e4 in 20 iterations and then stopped on a failed direction/line search
(not a residual plateau; iterates had left the orthant). jn+ipm's first
Josephy-Newton step reached residual^2 22.9, then froze at 19.62 for ~22
outer iterations until the inner IPM threw "step length collapsed"
(indefinite linearization). Proposed next-step menu (cheapest first): ssn
nonmonotone memory 4 + FB restart ladder; multi-start; myEps continuation;
PATH-style proximal wrapper (deferred). See the MCP design memory for detail.

Menu item #1 IMPLEMENTED (2026-07-06, awaiting Ben's build+run -- CMake
reload, one new test file): three SEMI-style ssn variant rows added to
gams_deploy_test (ssn-m4 = nonmonotone memory 4; ssn-m4-lam95 = penalized FB
at the 0.95 restart lambda; ssn-m4-fb = plain FB), built via the new
makeSsnRow hook in mcpengines.hpp. ALSO: the cheap no-progress cutoff Ben
asked for is now in the LIBRARY -- JosephyNewtonParams::stallIterMax /
stallRelDecrease (default off; honest converged=false before the next
Jacobian + inner solve), unit-tested in the new josephy_newton_test (frozen
inner solver drives the stall deterministically, per the NS1 lesson);
gams_deploy_test sets stallIterMax 5, so the jn+ipm row now stops in ~6 outer
iterations instead of 27 s of frozen ones.

Run 2 (Ben, 2026-07-06): josephy_newton_test GREEN; deploy still all-fail,
but with sharp structure -- jn+ipm stalls at a FEASIBLE mixed-strategy point
at residual^2 19.6 (natural norm ~4.4, three orders closer than any cold ssn
row), ssn-m4 escaped the old iter-20 death (85 iters, plateau 1.22e4),
ssn-m4-lam95 VISITED 8.2e3 then wandered up and died at 1.74e4 (solver
returns last iterate, not best-visited -- improvement proposed, pending Ben),
plain FB weakest (10 iters). Follow-up: (1) chain row "chain ipm->ssn-m4" --
jn+ipm to its stall, ssn-m4 warm-started from that iterate; (2) report-only
residual-breakdown check printing the worst natural-residual rows BY GAMS
NAME (r_i = min(y_i, G_i) on orthant rows, H on free rows) with y/G values
for every returned point.

Run 3 (Ben, 2026-07-06): CHAIN CONFIRMED -- phase 2 descended 19.6 -> 0.68
(52 iters), best of any engine by far; breakdown shows the remaining ~0.3
violations clustered on the active pairs, led by NEGATIVE escort allocations
(pole adjacency: the cause of the finisher's line-search death) plus
stationarity violations and pr(RS5) wanting to enter. End point support
pr {RS3, RS4}, pb {BS2, BS5}, payoffs 60.10 / 58.85 -- compare against the
GAMS/PATH listing when a run converges. NEXT (built, awaiting run): the
ALTERNATING chain "altchain ipm->ssn" -- rounds of {project onto K ->
jn+ipm (stall) -> ssn-m4}, continue while a round halves the best residual,
cap 5, return best-visited. Ben REQUIRES the chain logic documented in the
final report; the complete write-up rationale (evidence, design decisions,
precedents) is recorded in memory `project_visolver_altchain_writeup.md`,
condensed in makeAlternatingChainRow's header comment.

Run 4 (Ben, 2026-07-06): altchain round 2 validated the alternation --
after projection the re-linearized jn+ipm made fresh progress (1.19 -> 1.07;
its round-1 stall was at 19.6) -- but its inner IPM threw "step length
collapsed" before the stall cutoff tripped, and the escaping exception
failed the row, discarding the 0.68 best. HARDENED same day (awaiting run,
plain rebuild): each phase under try/catch -- a throw is a STALLED PHASE
(logged, best kept, finisher runs from the projected start; both phases
throwing ends the loop); improvement factor loosened 0.5 -> 0.9 so endgame
rounds may grind small gains under the round cap. Open question flagged for
Ben: mehrotraIpm throwing on step-length collapse (a stall, not a bad value)
vs returning converged=false -- the throw stance costs composability.

## Post-success work package (2026-07-06, Ben approved "2+3+4, 5, commit"): CODE DONE, awaiting Ben's build+run (CMake RELOAD -- new files)

1. `semismoothNewtonSolve` now returns its BEST-VISITED iterate (natural-
   residual sense), not the last (the lam95 evidence: ended 1.7e4 after
   visiting 8.2e3). Header documents it; iter still counts all iterations.
2. `mehrotraIpm` step-length collapse now returns honest converged=false at
   the current iterate instead of throwing (a stall is not a bad value; the
   throw cost the chain a row in run 4). Header contract updated; no test
   depended on the throw.
3. Alternating chain PROMOTED to the library: `include/alternatingchain.hpp`
   + `lib/alternatingchain.cpp` -- `alternatingChainSolve(model, z0,
   globalizer, finisher, params, logger, projector)` with StageSolver seams,
   best-point memory (chain recomputes the natural residual itself),
   throw-as-stall, improvement cutoff, ChainStageLogger hook. Full rationale
   in the header (the report can cite it). New `alternating_chain_test`
   (5 cases: real-engine integration on a monotone LCP; best-point memory;
   throw absorption; improvement cutoff; input guards -- fake fixed/throwing
   stages drive the protocol deterministically). gams_deploy_test's row now
   BINDS the library chain (deploy-specific stages only); its old inline
   loop is gone. CMake: lib source + test target + aggregates.
4. CLAUDE.md updated: direct mixed-NCP solvers section (ssn best-iterate,
   alternating chain), JN stall cutoff, and the throw-never-substitute
   invariant now distinguishes STALLS (honest converged=false) from bad
   values (throw).

Verification for Ben: CMake reload, then run_all_tests (expect 105 + 5 new
chain cases + the deploy row exercising the library chain = green), with
special eyes on gams_deploy_test still converging to the GAMS equilibrium
through the library chain. Commit after green.

Run 6 result: 109/110 -- only gams_deploy red. The library chain reached
best 0.1449 in the RIGHT basin (support RS3/RS4 x BS2/BS5; the semantics
changes legitimately shifted the trajectory) but the 10%-per-round
improvement demand ended the loop after round 3, before the finisher's
quadratic tail engaged. Fix: the cutoff became strict-improvement (default
improveFactor 1.0; roundsMax alone bounds cost).

Run 7: digit-for-digit identical to run 6 -- which is itself the diagnosis:
round 3 made NO improvement (the 0.1449 was round 2's), and the chain is
deterministic, so a stagnant round retried verbatim repeats identically; no
improvement rule can help. Fix (awaiting re-run, plain rebuild):
PERTURB-RESTART in the library chain (the pre-approved PATH-style fallback).
New param perturbScale (default 0 = off): a stagnant round makes the NEXT
round start from bestZ jiggled by U[-1,1] * perturbScale * sqrt(bestMag)
(shrinks as the chain closes in), projected onto K; fixed-seed mt19937 keeps
runs reproducible; stagnation with perturbation off still ends the chain.
Deploy test: perturbScale 0.1, roundsMax 8 (~7 s/round worst case). New unit
case PerturbRestartKeepsStagnantChainTrying (identity stages = a guaranteed
fixed point) + a negative-perturbScale guard.

Run 8: the perturbed chain CONVERGED (round 4, residual^2 1.2e-9, 28.8 s) --
to a SECOND Nash equilibrium: same supports, mirror-image cap pattern
(pr(RS3) = 0.600 = rho binds for RED where the GAMS point has Blue's
pb(BS2) = 0.600). Ben accepted it as genuine. Gate is now an EQUILIBRIUM
ROSTER (kKnownEquilibria): converge to magTol + match ANY verified entry;
non-roster converged points print themselves as paste-ready entries for
verification; the roster grows as Ben finds equilibria from random starts.
Test renamed AtLeastOneEngineReachesKnownEquilibrium. Awaiting the
confirming re-run (plain rebuild), then commit the whole package.

## IP4a probe result (Release, seed 20260704, engine ipm, iterMax 200, maxCertificateRounds 1)

    kept 10737/14500   rounds 1   final-iter 35   wall 1,778,400 ms (~30 min)
    th_ration 39.84167   th_star 53.11743   dlv 0.957   tm/tmG 0.800
    lambda 2.22e-06   cert NO (honest: one round is not enough on banded)

Readings: the IPM converged at Newton dimension 10,838 in THIRTY-FIVE
iterations, so the degeneracy-insensitivity claim holds at scale (bsHe94b
had capped at 150k iterations on a system a fifth that size). Wall time is
almost entirely dense LU (~50 s per iteration at dim 10.8k). One certificate
round ballooned the kept set to 74% of keep-all. Verdict: the ENGINE problem
is solved; the screen/certificate formulation times dim^3 linear algebra is
the binding constraint — hence NS.

## Deferred: IP4b control runs (UNFINISHED TASK)

Planned for next weekend, when hours (possibly days) of benchmark time are
available; can be driven by an Opus session. Steps:

1. Bounded probes (`benchmark.instances = 1`, `screen.maxCertificateRounds
   = 1`) for `solver.engine = chain` and `= bshe94b` (`solver.iterMax =
   150000`), plus `= ssn` (`solver.iterMax = 200`; MC3's big-affine datum
   beside ipm's 35-iteration result). Recipe: copy
   `network/benchmark-example.cfg` and edit those keys. Expect up to hours
   each (the round-1 dim ~10.8k grind is the datum).
2. Decide whether full 3-instance sweeps add anything beyond the bounded
   probes.
3. IP4c: write the performance.md addendum (P6/P7) comparing ipm / chain /
   bshe94b on identical instances, and close the gate in `network/plan.md`.

## Problem input format: DECIDED 2026-07-08 — GAMS-subset front end, staged

("GAMS" is a registered trademark of GAMS Development Corporation; this
work is not endorsed or certified by them, and the parsed subset is
incompatible with most of the GAMS modeling language. All GAMS mentions
in this section mean that limited subset.)

Ben chose to proceed with the bespoke GAMS-subset parser (gates GP1-GP5;
plan of record `doc/2026-07-08-gams-frontend-plan.md`, which adds Ben's
GP5 result-reproduction gate and its per-file evidence inventory). GP1
(grammar + AST + canonical echo, new `gams/` module, lib `vincpgms`,
`gms_parse_test` with the census as assertions) is CODE DONE, awaiting
Ben's build+run (CMake RELOAD — new directory and targets; expected new
ctest total 191 = 182 + 9 GmsParse cases). The `.nl`/AMPL-MP research
below stays as the interoperability option to layer on later.

## Deferred: remove unneeded GAMS references (PENDING TASK, Ben 2026-07-08)

Sweep the tree for "GAMS"/"gams" occurrences that are not required and
rename or reword them, keeping only (a) the mandatory trademark/subset
disclaimers and (b) mentions that genuinely must name the language. Known
candidates: file/target names `test/gams_alloceff_test.cpp`,
`test/gams_deploy_test.cpp` (suites already renamed GmsAlloceff/GmsDeploy;
the executables and files still say gams_), the `gams/` directory and
comments/docs that could say "GMS subset" instead. Involves git renames +
CMake + report references — do as one dedicated pass, not piecemeal.

## Original research record (2026-07-08, pre-decision)

Ben's requirement (2026-07-08): a text-based way for people to specify
problems -- fleet-scale, constantly changing -- including the solver to
apply. Research done, decision NOT made (Ben is thinking it over).
The GAMS-subset census and estimate for option B (six example files,
construct table, four-gate plan, ~a week of gated sessions, open
decisions) is in `doc/2026-07-08-gams-subset-census.md`.
Weighting note (Ben, 2026-07-08): the clean subset-to-VINCP mapping the
census found is DESIGNED, not lucky -- owning an implementation of the
GAMS/GLPK/AMPL/OCTAVE subset he actually uses is one of the project's two
founding motivations (the other: assessing what modern AI can do).
Automatic differentiation is out of scope BY DESIGN: derivatives are
Maxima's job in his workflow (stationarity conditions arrive
hand-derived and Maxima-verified).

Findings (sources in memory `project_visolver_input_format.md`): no open
C++ front-end exists for the AMPL language (the translator is AMPL's
commercial core). The open, solver-standard channel is the .nl INSTANCE
format; the AMPL/MP library (github.com/ampl/mp, BSD-style, CMake,
Windows + Linux, active) reads it and its NLHandler has a first-class
`OnComplementarity` callback -- the exact mixed-NCP form VIModel solves,
and byte-identical input parity with PATH. Pyomo (BSD, Python,
pyomo.mpec) is the open front-end that writes .nl with complementarity.

Options on the table:
- A: Pyomo/AMPL front-end -> .nl -> visolver reads via MP (days of work;
  Python dependency on the MODELER's machine only).
- B: bespoke restricted algebraic parser in visolver (params, sets,
  indexed sums, linear/quadratic, complements; weeks + maintenance;
  fully self-contained).
- C (recommended): A now, B only if the Pyomo dependency proves
  unacceptable -- the MP reader targets the same internal problem
  representation either way, so nothing is wasted.
Deciding question: who the model authors are (Python-capable vs analysts
editing a text template). Solver choice rides outside .nl (command-line /
options string, as PATH does; the existing .cfg precedent).

## Recipes (verified working on this machine)

- Release build from the CLI (the CLion tree; plain `cmake --build` fails
  for lack of the MSVC environment). PowerShell:

      cmd /c '"C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvars64.bat" >nul 2>&1 && cmake --build C:\repos\ghub-per\titan-alloy\visolver\cmake-build-release --target network_benchmark'

- Benchmark: `cmake-build-release\network\network_benchmark.exe <full path
  to cfg>` (any working directory).
- Maxima algebra check (FORWARD slashes in the path — backslashes are eaten):

      C:\maxima-5.49.0\bin\maxima.bat -q -b "C:/repos/ghub-per/titan-alloy/visolver/network/doc/ns2-newton-check.mac"

## Where the detail lives

- NS seam design, contract, and the machine-verified NS2 algebra:
  cross-session memory `project_visolver_newton_seam_plan.md` (in the Claude
  memory directory) plus `network/doc/ns2-newton-check.mac` itself.
- IPM engine history, IP4 state, build recipes: memory
  `project_visolver_ipm_plan.md`.
- The three-option engine roadmap with open-access citations: memory
  `project_visolver_engine_roadmap.md`.
- Dated decisions: `network/plan.md` (2026-07-05 IPM entry; 2026-07-06
  entry for IP4a/deferral).
<!-- Copyright Ben Paul Wise. All Rights Reserved. -->

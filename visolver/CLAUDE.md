# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## What this is

A C++20 / Eigen solver for variational-inequality / nonlinear-complementarity
problems (VINCP), ported from GNU Octave. Everything lives in namespace `VINCP`.
The mathematical target is a mixed VI over `K = R^n x R_+^m` with variable
`z = (x, y)` (free block `x`, non-negative block `y`):

    H(x, y) = 0,   0 <= G(x, y) _|_ y >= 0,   with F = (H, G).

`README.md` describes the layer structure and build. (`handoff.md` /
`handoff.pdf` are an early, now-stale design handoff; do not treat them as
current.)

## Build, test, run

Eigen is **header-only**; there is nothing to compile or install for it. The
build finds it via the `EIGEN3_INCLUDE_DIR` cache variable, which must point at
the directory that *contains* the `Eigen/` folder (the checkout root, not the
`Eigen` subfolder). It defaults to `C:/repos/ghub-ext/eigen`; override for other
machines (e.g. `-DEIGEN3_INCLUDE_DIR=/usr/include/eigen3` on Debian).

    cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
    cmake --build build
    ctest --test-dir build --output-on-failure

Run a single test (by ctest name) or the executable directly:

    ctest --test-dir build -R lcp_psd_test --output-on-failure
    ./build/lcp_psd_test            # Windows: build\Debug\lcp_psd_test.exe

CLion configures `CMakeLists.txt` directly; its build tree is `cmake-build-debug/`.
The aggregate targets `run_all_tests` / `run_engine_tests` / `run_network_tests`
are custom targets whose ctest command executes during the BUILD step: in CLion
use the hammer (Build) on them, never Run — a custom target has no executable,
so Run reports "Executable is not specified". To run the whole suite from the
Run button, use CLion's native "All CTest" configuration instead.

The user builds and runs themselves — hand off the commands rather than invoking
CMake/compilers/binaries to "verify". Warnings are errors-in-spirit: the library
target `vincp` is built with `/W4` (MSVC) or `-Wall -Wextra` (else); Eigen
headers are included as `SYSTEM` so they are exempt.

## Architecture

Two solver layers over one shared core; the dependency arrows point one way
(demos/tests -> solvers -> core), with no cycles.

- **Core (`include/vincp.hpp`, `lib/vincp.cpp`)** — solver-independent domain
  types, included by everything, including nothing itself:
  - `VIResult { VectorXd z; double residual; int iter; bool converged; }` — the
    **single** result type returned by *every* solver.
  - `VIModel { Index n, m; std::function H, G; }` plus free `evaluateF(model, z)`
    which splits `z` into `x = head(n)`, `y = tail(m)` and stacks `(H, G)`.
  - The feasible set `K` enters solvers only through a `Projector`
    (`std::function<VectorXd(const VectorXd&)>`). Ready-made:
    `projectNonnegative` (onto `R_+^n`) and `makeMixedProjector(numFree)` (onto
    `R^numFree x R_+^(n-numFree)`).

- **Inner solvers (interchangeable via the `InnerSolver` seam)** — same
  interface (identical argument order and the shared `VectorXd`/`MatrixXd`/
  `Projector`/`IterationLogger`/`VIResult` types; only the params struct differs):
  - `dHan06` (`include/dhan06.hpp`, `lib/dhan06.cpp`) — Deren Han's 2006
    self-adaptive projection method; tunables in `DHan06Params`.
  - `bsHe94b` (`include/bshe94b.hpp`, `lib/bshe94b.cpp`) — Bingsheng He's 1994
    fixed-metric projection-contraction (eq. 16): factors `(M + I)` once and
    reuses it; tunables in `BsHe94bParams`.
  - `solodovSvaiter` (`include/solodovsvaiter.hpp`) — matrix-free hyperplane
    double-projection; globally convergent for pseudomonotone `F` but only a
    GLOBALIZER (`O(1/sqrt(k))` tail whenever `||F(x*)|| > 0`), not a finisher.
  - `chainedSolodovHe` (`include/chainedsolver.hpp`) — SS to a loose target,
    then bsHe94b warm-started from its iterate.
  - `mehrotraIpm` (`include/mehrotraipm.hpp`) — Mehrotra predictor-corrector
    interior point for the monotone MIXED LCP over `R^n x R_+^m`. Iteration
    counts (~10-40, one dense LU each) are insensitive to degenerate optimal
    faces — the cure for near-tied instances that stall the projection
    engines. Structural exceptions: it takes `numFree` instead of honoring an
    arbitrary `Projector` (its seam adapter ignores `x0` and `Pr` — never pair
    it with a non-orthant `K` like an ellipsoid), and it cannot warm-start.
  The projection engines solve a **linear** VI `(M x + q)` over any `K` given
  as the `Projector`; the IPM is mixed/orthant-structural only.

- **Direct mixed-NCP solvers and compositions (beside the inner-solver seam):**
  - `semismoothNewtonSolve` (`include/semismoothnewton.hpp`) — penalized
    Fischer-Burmeister semismooth Newton on `Phi(z) = [H; phi(y_i, G_i)] = 0`,
    attacking the NONLINEAR mixed NCP directly (no Josephy-Newton nesting).
    Seams: `NcpFunctionPair` (penalized / plain FB ready-made) and `JacobianFn`
    (empty = FD). Returns the BEST-VISITED iterate in the natural-residual
    sense, not the last (the nonmonotone search can end above its best).
  - `alternatingChainSolve` (`include/alternatingchain.hpp`) — rounds of
    project-onto-K -> globalizer -> finisher with best-point memory, for
    NONMONOTONE problems where no single engine converges (built and proved on
    the deploy_v07 GAMS game — see `test/gams_deploy_test.cpp`). Stage solvers
    are seams (`StageSolver`); a stage that throws is a stalled stage, not a
    chain failure. The header carries the full rationale.
  - `chooseEngine` (`include/chooseengine.hpp`) — the dispatcher:
    `probeMonotone` (one Cholesky attempt on sym(M) + shift, not an
    eigensolve), the PURE decision function `chooseEngine(ProblemTraits)`
    encoding the evidence-backed selection table, and two executors —
    `solveAffineAuto` (orthant mixed LCP: probe, pick ipm/bshe94b/chain, run)
    and `solveModelAuto` (mixed NCP: semismooth first; on an honest stall or
    a runtime guard, fall back to the alternating chain). Choices surface
    through the `ChoiceLogger` hook, never silently.

- **Outer driver (`include/josephynewton.hpp`, `lib/josephynewton.cpp`)** —
  `solveVI`, a Josephy-Newton loop for the nonlinear VI. Each step linearizes `F`
  with a finite-difference Jacobian `J`, then solves the affine VI `M = J(z_k)`,
  `q = F(z_k) - J(z_k) z_k` over the **same** `K` (via `makeMixedProjector(n)`),
  then **damps** the step with an Armijo line search on the natural-map merit.
  The projector carries the mixed free/non-negative structure, so there is
  deliberately **no Schur complement** and no elimination of the free block.
  - The inner solver is a parameter, not hard-wired: `solveVI` takes an
    **`InnerSolver`** functor `(x0, M, q, Pr) -> VIResult`. Adapt a concrete
    solver with `makeDHan06Solver(...)` / `makeBsHe94bSolver(...)`, which bind its
    tolerances/caps/params/logger. This lets the same outer loop drive different
    inner solvers on identical problems (see `test/han_vs_he_test.cpp`). Inner
    controls live in the functor, not in `JosephyNewtonParams` (outer-only).
    `JosephyNewtonParams` also carries a no-progress cutoff (`stallIterMax`,
    default off): after that many consecutive outer iterations without relative
    residual improvement it stops honestly (converged = false) BEFORE spending
    another Jacobian + inner solve.

- **Jacobian (`include/fdjacobian.hpp`, `lib/fdjacobian.cpp`)** —
  `centralDifferenceJacobian`. **4th-order** central differences (step
  `eps^(1/5)`), snapped to an exactly representable width. Real-arithmetic
  analogue of the Octave complex-step Jacobian; complex-step is intentionally
  not used.

- **Globalization building blocks (separate, reusable):**
  - `include/armijo.hpp`, `lib/armijo.cpp` — `armijoLineSearch`, a solver-
    agnostic backtracking search on any scalar merit. The driver uses it
    (`JosephyNewtonParams::armijo`) to damp the Newton step; without it the
    undamped step chatters (period-2) at the non-smooth complementarity solution.
  - `include/levenbergmarquardt.hpp`, `lib/levenbergmarquardt.cpp` — two layers:
    the primitives `levenbergMarquardtDamp`/`Update` (a `J + lambda I` regularizer
    + lambda policy, reusable by any algorithm), and `levenbergMarquardtSolve`, a
    self-contained LM nonlinear-least-squares solver for `min 1/2 ||F(x)||^2`,
    `F: R^n -> R^m` (m >= n), built on those primitives + the FD Jacobian and
    returning the shared `VIResult`. Exercised by `test/lm_test.cpp`. It is
    independent of the Josephy-Newton driver (which globalizes with Armijo).

## Invariants that will bite you if ignored

- **Residuals are SQUARED norms.** `magTol` (inner), `residual`/`outerTol`
  (outer), and `VIResult::residual` are all squared Euclidean norms
  (`dot(e, e)`), not norms. This matches the Octave source and the user's
  verified tolerances — do not "fix" it to a plain norm.
- **The outer residual floor is set by `innerMagTol`, not the FD Jacobian.**
  With Armijo damping the outer natural residual descends monotonically to a
  floor that scales linearly with the inner dHan06 tolerance (`innerMagTol`).
  The FD Jacobian order does not bind it here (2nd- and 4th-order give identical
  iterates). To reach a tighter `outerTol`, tighten `innerMagTol` — do not reach
  for the Jacobian step or order.
- **Throw, never silently substitute.** NaN residual, divergence (residual
  exceeding `divergenceFactor * initialMag`), and a non-finite linear solve all
  throw `std::runtime_error`; dimension/parameter problems throw
  `std::invalid_argument`. Preserve this stance in new code; surface bad values
  early rather than papering over them. But a STALL is not a bad value: a
  collapsed step length in `mehrotraIpm`, a failed line search or direction
  ladder in `semismoothNewtonSolve`, and the Josephy-Newton no-progress cutoff
  all return honestly with `converged = false` at a real iterate — throwing on
  stalls costs composability (engine chains had to catch and ignore).
- **Han's method needs a monotone problem** (M positive semidefinite) to
  converge. This is why the two LCP tests differ: `lcp_random_test` uses a
  random indefinite `M` (a stress test that may legitimately hit the divergence
  guard) and `lcp_psd_test` uses `M = A^T A` (guaranteed convergent). Keep this
  in mind when constructing new test problems.
- **Eigen is column-major by default.** Relevant for the not-yet-written JNI
  boundary: a row-major buffer from Java must be mapped with an explicit
  `RowMajor` map, or it transposes silently.

## Copyright headers (required)

Every `.cpp`, `.hpp`, `.h`, `.md`, and `.txt` file carries
`Copyright Ben Paul Wise. All Rights Reserved.` at both its top and bottom.

- `.cpp` / `.hpp` / `.h`: a **ruler-banner block** opens and closes the file. The
  top banner frames the copyright in ruler lines, then a one-line description of
  the file's purpose, then a final ruler:

      // ----------------------------------------------
      // Copyright Ben Paul Wise. All Rights Reserved.
      // ----------------------------------------------
      // <one-line description of what this file is>
      // ----------------------------------------------

  The file ends with the bare copyright banner (no description):

      // ----------------------------------------------
      // Copyright Ben Paul Wise. All Rights Reserved.
      // ----------------------------------------------

  The ruler is dashes; a few older files use `// =-=-=...` — dashes are the norm.
- `.md`: `<!-- Copyright Ben Paul Wise. All Rights Reserved. -->` as the very
  first and very last line.
- `.txt`: the bare line `Copyright Ben Paul Wise. All Rights Reserved.` first and last.

Exceptions: omit it only where it would interfere with the file's purpose (e.g.
a file whose first line is significant, or data/input `.txt` files that are
parsed). **`CLAUDE.md` itself is exempt.** When creating any new source, header,
markdown, or text file, add the banner.

## Conventions

- C++20; must build on **both** Windows 11 and Debian Linux — never use anything
  platform-restricted. Keep linear-algebra calls localized so the Eigen choice
  stays reversible.
- Headers `.hpp`, sources `.cpp`; include guards are `VINCP_<FILE>_HPP` (never
  `#pragma once`). American spelling. Split files/functions well before a few
  hundred lines.

### Personal C++ style (all of `include/` and `lib/` follow it)

A `.clang-format` at the repo root captures the deterministic slice (indent 2,
Allman braces for function bodies, return-type break for definitions, namespace
indent, `else` on its own line); the rest is applied by hand. Run CLion's bundled
clang-format. The hallmarks:

- **Indent 2 spaces, no tabs; namespace contents are indented** one level.
- **`using std::…` at file scope** for the common types (`string`, `vector`,
  `function`, `ostream`, …) so they read bare; keep `std::` on calls / utilities /
  exceptions (`std::sqrt`, `std::invalid_argument`, …). `Eigen::` is named ONLY in
  `vincp.hpp`, which pulls the Eigen types into `namespace VINCP`.
- **Out-of-line function DEFINITIONS**: the return type is on its own line and the
  opening brace on its own line (Allman). Control flow (`if`/`for`/`while`) and
  `class`/`struct`/`namespace` keep the brace on the same line (K&R); `else` goes on
  its own line. Declarations keep the return type inline.
- **Yoda conditions**: when one operand is a numeric literal (or `nullptr`), put it
  on the LEFT — `0.0 < x`, `0 == n % 2`, `nullptr == p` — to catch `=`-for-`==`
  typos on near-obsolete toolchains that do not warn. Two-sided ranges read
  ascending (`0.0 < x && x < 1.0`); variable-vs-variable comparisons stay natural.
- **Braces around every `if`/`else` body**, including single statements.
- **Explicit `return;`** ends every `void` function.
- **Predicate booleans take a trailing `P`** (`doneP`, `lessP`).
- **Classes list all three access labels** — `public:`, `protected:`, `private:`,
  in that order, even when a section is empty; single-argument constructors are
  `explicit`; deleted copy operations carry a `// no copy` comment.
- Types PascalCase; functions / methods and variables / members camelCase, with **no
  `m_` prefix and no trailing underscore** (a couple of legacy `label_` / `start_`
  members in `utils` are the lone survivors).
- The correctness stance is UNCHANGED by the restyle, and overrides surface style
  in any conflict: still `throw` (never `assert`) for preconditions, value-semantic
  Eigen/RAII (no raw `new`/`delete`), and `static_cast` / modern `<c…>` headers.
- Tests use **GoogleTest** (fetched via CMake `FetchContent`, pinned tag; MSVC needs
  `set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)`). To add one: drop a `.cpp`
  in `test/` using `#include <gtest/gtest.h>` and `TEST(...)`/`EXPECT_NEAR` etc., then
  add an `add_executable` + `target_link_libraries(... vincp GTest::gtest_main)` +
  `gtest_discover_tests(...)` triplet in `CMakeLists.txt`. Name all
  dimensions/tolerances as named constants (no magic numbers). Legacy plain-exe
  tests (return 0/1, registered with `add_test`) may still exist during the
  migration; new tests should be GoogleTest. The shared harness for solver cases is
  `utils`'s `runCase`/`CheckFn` — a `CheckFn`'s pass/report maps naturally onto
  `EXPECT_*` inside a `TEST()`.

## Planned future direction (do not start unprompted)

A second design document, `2026-06-29-LVI_solver_handoff.md`, sketches a more
sophisticated solver. **It was written by a Claude instance that did not know
about this project**, so its proposed architecture, module layout, and class
names do NOT correspond to the current code — ignore those specifics. The single
idea to carry forward from it is the solver engine:

- **Global engine:** the Solodov–Svaiter projection (hyperplane / double-
  projection) method as a matrix-free, globally convergent safeguard — converges
  under pseudomonotonicity, weaker than the PSD/monotone condition Han's method
  needs.
- **Local engine:** a Rui–Xu-style inexact smoothing Newton step for the
  superlinear/quadratic tail (smoothing parameter `mu > 0` keeps the Jacobian
  nonsingular; inexact truncated-Krylov linear solve).
- **Switching logic:** track the natural-map merit `theta(x) = 1/2 ||F_nat(x)||^2`;
  accept the smoothing-Newton step on sufficient (Armijo) decrease of `theta`,
  otherwise fall back to a Solodov–Svaiter projection step. Drive `mu -> 0` as
  the residual shrinks.

This is deferred work: when asked, we will plan how to fit this hybrid engine
into the existing `VINCP` framework (reusing `VIResult`, `VIModel`, the
`Projector` abstraction, and the natural-residual merit already computed in
`solveVI`). Do not begin implementing it without an explicit request.

## Next planned work: logistics network QP (start here next session)

The user is bringing a NEW problem to this codebase: a **system-optimal convex QP**
for distribution planning over a sparse ~100-node / ~500-arc network (minimize
weighted squared demand-shortfall subject to flow balance, supply caps, and a
ton-mile budget), generalizing soon to multiple goods, vehicle types, and resource
budgets. It is a single-planner optimization, NOT a game/equilibrium (do not
conflate with SAOE). The full spec, agreed recommendations (Lagrangian-on-budget +
convex min-cost flow; OSQP; or reuse VINCP via a monotone KKT-LCP), data-structure
notes, the coming multi-good/multi-vehicle generalization, a staged plan skeleton,
and open questions are in **`2026-07-01-logistics-qp-handoff.md`** — read it first.

## Outstanding tasks (revisit later; do not start unprompted)

- **SAOE equilibrium selection.** On the fixed `test/saoe_test.cpp` instance the
  solver reliably converges to a negative-utility KKT point, not the reference
  equilibrium a now-lost earlier C++ solver found (`saoe_test` therefore gates on
  *feasibility*, not a specific equilibrium). This game is non-monotone / non-
  concave with multiple KKT/stationary points; random starts (the `repeatable`
  switch in `saoe_test`) all fall into the same basin. Likely levers to reach the
  intended equilibrium: a stronger globalization (the Solodov–Svaiter safeguard
  above) or a homotopy/continuation start.
- **dHan06 inner-solver cost.** In the SAOE runs dHan06 is ~12x slower in wall
  time than bsHe94b at nearly equal iteration counts, because it re-factorizes
  `(I + beta_k M)` every inner iteration (beta_k is self-adaptive) while bsHe94b
  factors `(M + I)` once and reuses it. If dHan06 speed ever matters, cache/reuse
  its factorization across beta updates (or a rank-update), or cap inner iters.

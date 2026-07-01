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

- **Inner solver (`include/dhan06.hpp`, `lib/dhan06.cpp`)** — `dHan06`, a C++
  port of Deren Han's 2006 self-adaptive projection method for a **linear** VI
  `(M x + q)`. `K` is supplied as the `Projector` argument, so the same routine
  handles any set you can project onto. Tunables in `DHan06Params` (defaults
  reproduce the Octave constants exactly).

- **Outer driver (`include/josephynewton.hpp`, `lib/josephynewton.cpp`)** —
  `solveVI`, a Josephy-Newton loop for the nonlinear VI. Each step linearizes
  `F` at the current iterate with a finite-difference Jacobian `J`, solves the
  affine VI `M = J(z_k)`, `q = F(z_k) - J(z_k) z_k` over the **same** `K` via
  `dHan06` with `makeMixedProjector(n)`, then **damps** that step with an Armijo
  line search on the natural-map merit (see below). The projector carries the
  mixed free/non-negative structure, so there is deliberately **no Schur
  complement** and no elimination of the free block.

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
  - `include/levenbergmarquardt.hpp`, `lib/levenbergmarquardt.cpp` —
    `levenbergMarquardtDamp`/`Update`, a `J + lambda I` regularizer + lambda
    policy. **Not wired into any solver yet**; provided for future algorithms.

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
  early rather than papering over them.
- **Han's method needs a monotone problem** (M positive semidefinite) to
  converge. This is why the two LCP tests differ: `lcp_random_test` uses a
  random indefinite `M` (a stress test that may legitimately hit the divergence
  guard) and `lcp_psd_test` uses `M = A^T A` (guaranteed convergent). Keep this
  in mind when constructing new test problems.
- **Eigen is column-major by default.** Relevant for the not-yet-written JNI
  boundary: a row-major buffer from Java must be mapped with an explicit
  `RowMajor` map, or it transposes silently.

## Copyright headers (required)

Every `.cpp`, `.hpp`, `.h`, `.md`, and `.txt` file must carry the exact line

    Copyright Ben Paul Wise. All Rights Reserved.

as both its **very first** and **very last** line, wrapped in that file type's
comment syntax:
- `.cpp` / `.hpp` / `.h`: `// Copyright Ben Paul Wise. All Rights Reserved.`
- `.md`: `<!-- Copyright Ben Paul Wise. All Rights Reserved. -->`
- `.txt`: the bare line `Copyright Ben Paul Wise. All Rights Reserved.`

Exceptions: omit it only where it would interfere with the file's purpose (e.g.
a file whose first line is significant, or data/input `.txt` files that are
parsed). **`CLAUDE.md` itself is exempt.** When creating any new source, header,
markdown, or text file, add both lines.

## Conventions

- C++20; must build on **both** Windows 11 and Debian Linux — never use anything
  platform-restricted. Keep linear-algebra calls localized so the Eigen choice
  stays reversible.
- Headers `.hpp`, sources `.cpp`; include guards are `VINCP_<FILE>_HPP`.
- Braces around every `if`/`else` body, including single statements. American
  spelling. Split files/functions well before a few hundred lines.
- Tests are plain executables returning 0 (pass) / non-zero (fail), registered
  with `add_test` — there is no test framework. To add one: drop a `.cpp` in
  `test/`, then add an `add_executable` + `target_link_libraries(... vincp)` +
  `add_test` triplet in `CMakeLists.txt`. Name all dimensions/tolerances as
  named constants (no magic numbers).

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

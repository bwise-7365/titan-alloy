# Design Guidance — Variational-Inequality / NCP Solver Port (Octave → C++20)

*A handoff to another instance of Claude. Read this in full before writing or
changing any code. It records what the person wants, what has been decided, what
already exists and is verified, and what you must still confirm with the person
rather than assume.*

---

## 0. The single most important instruction

Do not substitute textbook constructions for the person's own design. This
project deliberately uses Deren Han's self-adaptive projection method for the
linear variational inequality (LVI), **not** Lemke, **not** a Fischer–Burmeister
reformulation, **not** an interior-point LCP solver. The person evaluated
several LVI solvers and chose Han's on purpose (Lemke has pathological cases they
must avoid). When in doubt about a numerical or modeling choice, ask the person;
do not fill the gap with the statistically common option.

The person is a former Ivy League professor of probability and statistics, fluent
in optimization, equilibrium analysis (NCP/VI), and game theory. Write at a
professional level. Do not explain elementary material. Do not pad, do not
flatter, and do not agree reflexively. When they ask for a recommendation, give
three contrasting options with pros and cons. Prefer asking a precise question
over making a "plausible assumption."

---

## 1. The mathematical problem

The person solves nonlinear complementarity / equilibrium problems phrased as a
variational inequality (VI).

Let `z = (x, y)` with the free block `x ∈ R^n` and the non-negative block
`y ∈ R^m`. Let `F = (H, G)` with `H: R^{n+m} → R^n` and `G: R^{n+m} → R^m`. Let
the feasible set be `K = R^n × R_+^m`. The VI is

    find z ∈ K such that  F(z) · (w − z) ≥ 0  for all w ∈ K.

This is equivalent to the mixed system

    H(x, y) = 0,
    0 ≤ G(x, y) ⊥ y ≥ 0.

`H` and `G` are smooth: the person has pushed all nonsmoothness into `K` and the
complementarity relation, not into `F`. (This is why a differentiable Jacobian is
meaningful at every iterate.)

### 1.1 Outer iteration — Josephy–Newton

The person's method: evaluate the Jacobian `J(z_k)` of `F` at the current point,
solve the resulting **linear** VI over the same `K`, take the solution as the
next iterate, and repeat to convergence. This is a full Josephy–Newton step (no
damping was described — see open questions).

### 1.2 Assembling the inner LVI (this is the key implementation insight)

The linearized map is

    F̂_k(z) = F(z_k) + J(z_k)(z − z_k) = J(z_k) z + [F(z_k) − J(z_k) z_k].

So the affine VI over `K` is exactly the LVI that `dHan06` already solves, with

    M = J(z_k),
    q = F(z_k) − J(z_k) z_k,
    Pr = makeMixedProjector(n)        // free first n components, clamp the rest at 0
    z_{k+1} = dHan06(z_k, M, q, Pr, ...).x

Because `dHan06` takes the projector `Pr`, the mixed free/non-negative structure
of `K` is handled directly. **You do not need a Schur complement and you do not
need to eliminate the free block.** (An earlier discussion mentioned reducing the
mixed LCP to a pure LCP via the Schur complement `M = D − C A⁻¹ B`; that is only
relevant if the inner solver were a pure-orthant LCP solver. It is not. Ignore
it for this project unless the person asks.)

The Jacobian block layout, for reference when validating dimensions:

    J = [ A  B ]   A = ∂H/∂x (n×n),  B = ∂H/∂y (n×m)
        [ C  D ]   C = ∂G/∂x (m×n),  D = ∂G/∂y (m×m)

---

## 2. Confirmed decisions (do not relitigate)

- **Inner LVI solver:** Han 2006 self-adaptive projection method. Already ported
  (Section 3). Reference: Deren Han, *Solving linear variational inequality
  problems by a self-adaptive projection method*, 2006.
- **No complex arithmetic.** The original Octave computes Jacobians with the
  `optim`-package `jacobs`, which is the **complex-step** method
  (`Im(F(z + i·h·e_j))/h`, default `h = 1e-20`). The C++ port is real-valued, so
  the outer driver's Jacobian must use **finite differences** instead. This does
  not change the converged solution (it is fixed by `F` and `K`), only the Newton
  path. See open questions for the scheme.
- **Linear-algebra backend:** Eigen (header-only). Chosen over Armadillo because
  the eventual JNI library on Windows then carries no LAPACK/BLAS runtime
  dependency to co-locate beside the JVM. Keep linear-algebra calls localized so
  the choice stays reversible.
- **Model functions `H` and `G`:** the person writes these by hand in C++. Your
  job is to define a clean interface they plug into, not to invent the models.
- **Java/JNI boundary:** arrays of `double` passed as pre-allocated 1-D buffers;
  matrices are row-major. (Exact signature still open — Section 5.)
- **Acceptance test:** the person has verified problems and solutions from the
  Octave code. Reproducing them to a user-specified tolerance is a required test.

---

## 3. What already exists and is verified

A standalone CMake project for the inner solver. Layout:

    include/dhan06.hpp   public interface
    src/dhan06.cpp       implementation
    src/main.cpp         demo / regression test (known-solution LCP)
    CMakeLists.txt       static library + demo + ctest
    README.md            build notes and fidelity notes

### 3.1 Public interface (already implemented)

- `lvi::dHan06(x0, M, q, Pr, magTol, iterMax, iterFreq, params, logger)` returning
  `DHan06Result { Eigen::VectorXd x; double mag; int iter; }`.
- `lvi::DHan06Params { gamma=1.6, mu=1.05, beta0=0.5, tau0=0.5, tauN=10,
  divergenceFactor=100.0 }` — the Octave constants, now tunable.
- `lvi::tau(t0, n, k)` — the τ schedule sub-function.
- `lvi::projectNonnegative(v)` — projection onto `R_+^n`.
- `lvi::makeMixedProjector(numFree)` — projection onto `R^numFree × R_+^(n−numFree)`;
  this is the projector the outer driver should use, with `numFree = n`.

### 3.2 Build and test (verified in a Linux container; the person builds on Windows 11 / CLion)

    # Eigen: Debian  ->  sudo apt install libeigen3-dev   (lands in /usr/include/eigen3)
    #        Windows ->  vcpkg install eigen3 + the vcpkg toolchain file
    cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
    cmake --build build
    ctest --test-dir build --output-on-failure

In the container, CMake was installed with `pip install cmake --break-system-packages`
and the compiler was g++ 13 (full C++20). The code compiles clean under
`-Wall -Wextra` (and `/W4` is set for MSVC in the CMake file). You can and should
build and run before handing anything back.

Baseline regression result (so you can detect a behavioral change): the demo
solves the monotone LCP `M = [[4,1],[1,3]]`, `q = (−4, 1)` on `R_+^2`, known
solution `x* = (1, 0)`, and converges in **12 iterations** to a solution error of
about **1.8e-9** with `magTol = 1e-14`.

### 3.3 Fidelity notes — places the port could silently drift

- `magTol` is compared against the **squared** residual norm `dot(e, e)`, exactly
  as in the Octave. It is not a norm. Preserve this; the person's verified
  tolerances assume it.
- The NaN check and the divergence guard (`mag` must stay below
  `divergenceFactor * initialMag`) **throw** `std::runtime_error`. A non-finite
  linear solve also throws. This is deliberate: the person dislikes silent
  default substitution because it hides the origin of bugs. Keep this stance
  everywhere — never quietly return a fallback value.
- The inner solve `(I + β_k M) y = e` uses Eigen `partialPivLu`, matching
  Octave's general-matrix `linsolve` (LU with partial pivoting).
- The dead local `myEps` from the Octave was dropped. (Confirm with the person it
  was not a placeholder for an intended feature.)
- The original `printf` at `iterFreq` is now an optional `IterationLogger`
  callback; logging is I/O, so the side effect is acceptable, but it is opt-in.

---

## 4. Next: the outer Josephy–Newton driver

This is probably your first task, but **confirm scope first** (the person may be
writing the driver themselves and may only have wanted the inner solver ported).

If asked to build it, the structure follows Section 1.2:

1. Define a model interface the person fills in, e.g.

       struct VIModel {
           Eigen::Index n;   // dimension of the free block x
           Eigen::Index m;   // dimension of the non-negative block y
           std::function<Eigen::VectorXd(const Eigen::VectorXd& x,
                                         const Eigen::VectorXd& y)> H;  // -> R^n
           std::function<Eigen::VectorXd(const Eigen::VectorXd& x,
                                         const Eigen::VectorXd& y)> G;  // -> R^m
       };

   Concatenate to `F(z)`; split `z` into `x = head(n)`, `y = tail(m)`. Propose
   this shape to the person and let them confirm; they may prefer free functions
   or templates.

2. Finite-difference Jacobian of `F` (no complex step — decision in Section 2).
   Recommend **central differences** with relative step `eps^(1/3)` (this is the
   accuracy-matched real-arithmetic analogue of the complex-step Jacobian the
   Octave used, and matches the Octave `optim` central default). Forward
   differences (`sqrt(eps)`) are the cheaper, lower-accuracy alternative. Confirm
   the choice and the step rule with the person.

3. Per outer iterate: build `M = J(z_k)`, `q = F(z_k) − J(z_k) z_k`,
   `Pr = makeMixedProjector(n)`, call `dHan06`, set `z_{k+1}` to its solution.

4. Outer convergence: recommend the natural-residual merit, reusing the inner
   solver's metric for consistency — `r(z) = z − Π_K(z − F(z))`, converged when
   `‖r(z)‖² < outerTol`. Confirm; the person may use `‖z_{k+1} − z_k‖` or a
   split test on `‖H‖` and the complementarity residual instead.

Keep functions short (a few hundred lines is the person's ceiling for a file or
function — split before that). Braces on every `if`/`else`, even one-liners.
Prefer referential transparency; confine side effects to genuine I/O. American
spelling.

---

## 5. Next: the JNI entry point

Confirm the exact signature before writing the shim. The person described the
boundary as: pass pre-allocated 1-D `double` arrays — the first two are
1-D vectors, the third is a matrix in **row-major** order — and the solver
returns a 1-D array representing a matrix of the same shape.

Current best reading (to be confirmed, not assumed): this exposes `dHan06`
itself, taking `x0[]`, `q[]`, and `M[]` (row-major), and returning the solution
`x[]` (an `n×1` matrix, i.e. the same length as `x0`). Two points need the
person's answer:

- **Projector across the boundary.** A `std::function` cannot cross JNI. Likely
  pass an integer count of free components feeding `makeMixedProjector`; confirm.
- **"Same shape."** Confirm this means the solution vector (the current reading)
  and not literally an `n×n` matrix.

Critical implementation gotcha: **Eigen is column-major by default.** A row-major
`double[]` coming from Java must be mapped with an explicit row-major map, e.g.

    Eigen::Map<const Eigen::Matrix<double, Eigen::Dynamic, Eigen::Dynamic,
                                   Eigen::RowMajor>> Mmap(ptr, rows, cols);

Do not pass a row-major buffer straight into a default Eigen map; that transposes
the matrix silently — exactly the kind of silent error the person wants avoided.

Provide a thin `extern "C"` layer over the C++ core; keep the numerics in the
core so the JNI layer is pure marshaling. Use `GetPrimitiveArrayCritical` or
`GetDoubleArrayElements` per the person's preference; ask if unsure.

---

## 6. Standing preferences that shape the code

- C++20 throughout. Must build on **both** Windows 11 and Debian Linux; never use
  anything restricted to one platform. CMake build, CLion IDE. Qt 6 only if a GUI
  is requested (none so far).
- Braces around every `if`/`else` body, including single statements.
- No silent default substitution. Surface errors early (throw with a clear
  message) rather than papering over a bad value that surfaces later.
- No huge functions or files. Split for clarity well before a few hundred lines.
- Avoid programming by side effect except for explicit I/O. Prefer referential
  transparency and a functional style.
- American spelling and phraseology.
- Deliverables are **editable source files the person downloads** (.hpp/.cpp,
  CMakeLists, .md). Do **not** produce PDF or DOCX unless explicitly asked.
  Present files through the file-presentation mechanism so the person can download
  them.
- Provide verified references (open-access papers, theses, reputable sources) when
  citing, and say what the source is.

---

## 7. References

- Deren Han (2006), *Solving linear variational inequality problems by a
  self-adaptive projection method*. (Source of the `dHan06` algorithm; the
  ported `.m` cites equations (3), (4), (10), (11) and the τ schedule.)
- Octave-Forge `optim` package, `jacobs` (complex-step Jacobian):
  https://octave.sourceforge.io/optim/function/jacobs.html — relevant only as
  background on what the original Octave did; the C++ port uses finite
  differences.
- Eigen: https://eigen.tuxfamily.org — note the default column-major storage
  order (Section 5).

---

*End of handoff. First actions for the receiving instance: (1) read the existing
four source files and build/test them; (2) confirm scope of the next deliverable
(outer driver, JNI shim, or both); (3) get answers to the open questions in
Sections 4 and 5 before committing to an implementation.*

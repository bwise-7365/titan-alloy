# Handoff: Solver for Linear Variational Inequalities `VI(M, q, K)`

**Purpose.** This document hands off a design conversation to a new Claude instance. It states the
problem, the constraints the user has fixed, the agreed architecture, the per-class mathematics, the
C++20 implementation plan, and a verified reference list. The user is a domain expert (optimization,
equilibrium analysis, formal game theory); write at a professional level and do **not** re-explain
basics. Ask for clarification rather than substituting "plausible" assumptions.

---

## 1. Problem statement

Find `x* ∈ K` such that

```
⟨ M x* + q , y − x* ⟩ ≥ 0    for all y ∈ K.
```

Write `F(x) = M x + q` (the affine/linear operator). `M` is **not** assumed symmetric. The user calls
this the Linear Variational Inequality (LVI). When `K` is a polyhedron this is the Affine VI; when
`K = ℝⁿ₊` it is the LCP; over a box it is the Mixed Complementarity Problem (MCP).

**The user explicitly excludes proprietary/closed-source solvers (e.g. PATH).** Everything here must be
a published algorithm reimplementable from open literature.

---

## 2. Fixed constraints (decisions already made by the user)

1. **Condition on `M`: pseudomonotone on `K`, not necessarily PSD.** This is strictly weaker than
   monotonicity. For affine maps it is a charted matrix class (see §7, Crouzeix–Schaible / Gowda /
   PSBD). Consequence: methods whose *global* convergence requires only `pseudomonotone + continuous`
   are safe; methods requiring `M ⪰ 0`, `P`-matrix, or `P₀` are **not** justified by the stated
   condition alone.
2. **Projection onto `K` is cheap.** This removes the usual penalty of double-projection methods.
3. **Two `K` classes** (the solver must handle both):
   - **Class 1 — box family:** product of intervals `K = ∏ᵢ [lᵢ, uᵢ]` mixing
     *free* (`lᵢ=−∞, uᵢ=+∞`), *orthant* (`lᵢ=0, uᵢ=+∞`), and *box* (both finite) coordinates.
     This is the MCP / box-constrained VI.
   - **Class 2 — axis-aligned ellipsoid:** `K = { x : (x−c)ᵀ D (x−c) ≤ ρ }`, `D = diag(dᵢ) ≻ 0`.
     Axis-aligned was confirmed; the secular equation is therefore **diagonal**.
4. **The free block in Class 1 cannot be eliminated.** Carry all `n` coordinates. `M_FF` (the
   free–free principal block) may be singular/ill-conditioned, so the Newton Jacobian can be singular
   without regularization (see §5).
5. **Sparsity is mixed and unpredictable** (some instances sparse, some dense). The solver must not
   commit to dense factorization, and must not assume an explicit sparse `M`.
6. **Implementation target:** C++20, CLion, CMake build system, must run on **both** Windows and
   Debian Linux. No platform-specific APIs.

### User coding conventions (apply throughout the C++)

- Curly braces around **every** `if`/`else` body, including one-liners.
- **No silent default substitution.** If a sub-procedure (e.g. the secular-equation root find, or the
  inner linear solve) fails to meet tolerance, signal it explicitly (status code or exception); never
  return a clamped/guessed value that hides the failure.
- No huge functions or files; split anything beyond a few hundred lines for clarity.
- Prefer referential transparency and a functional style; avoid programming by side effect except for
  genuine I/O. Projections, residuals, and merit values should be pure functions of their inputs.
- American spelling and phraseology.

---

## 3. Agreed architecture: matrix-free global safeguard + sparsity-adaptive local Newton

A single hybrid solver, reused across both `K` classes; only the **projector** and the **Jacobian
shift policy** differ between classes.

- **Global engine (safeguard): Solodov–Svaiter hyperplane/double-projection method (1999).**
  Matrix-free (touches `M` only through the product `x ↦ Mx`), globally convergent under exactly the
  user's condition (`pseudomonotone + continuous`), and indifferent to sparsity. With cheap projection
  it is inexpensive. This is what *certifies* global convergence; the smoothing-Newton theory alone
  does not, because it assumes `P₀`/sufficient data the user has not guaranteed.

- **Local engine (acceleration): Rui–Xu–style inexact smoothing Newton.**
  Gives the superlinear/quadratic tail. The **inexact** (truncated-Krylov) linear solve is the hedge
  against unpredictable sparsity: sparse `M` exploits sparsity in the mat-vecs; dense `M` pays only
  `O(n²)` per mat-vec with early termination. The **smoothing parameter `μ > 0`** keeps the Jacobian
  nonsingular even when `M` is not a `P`-matrix (essential for the pseudomonotone, non-PSD case).

- **Switching logic.** Maintain the natural-map merit `θ(x) = ½‖F_nat(x)‖²`. Each outer iteration,
  attempt the smoothing-Newton step; accept it if it yields sufficient merit decrease (Armijo on `θ`).
  Otherwise fall back to a Solodov–Svaiter projection step (guaranteed progress toward the solution
  set). Drive `μ_k → 0` as the residual shrinks. Stop on `‖F_nat(x)‖ ≤ tol`.

**Rationale for the hybrid.** Projection methods are robust and matrix-free but only linearly
convergent. Smoothing Newton is fast but its global guarantee needs matrix-class assumptions the user
cannot promise. Wrapping the Newton step in the projection safeguard yields global convergence under
pseudomonotonicity *and* a quadratic local rate.

---

## 4. Shared objects

### Natural map and merit (both classes)

```
F_nat(x) = x − Π_K( x − F(x) ),     F(x) = M x + q
x* solves VI  ⇔  F_nat(x*) = 0
θ(x) = ½ ‖F_nat(x)‖²      (merit; also the error-bound residual for stopping)
```

`Π_K` is cheap in both classes (§5). `F_nat` and `θ` are pure functions of `x`.

### Solodov–Svaiter global step (skeleton — verify exact constants against the source PDF)

Given `x_k`, parameters `σ ∈ (0,1)`, `γ ∈ (0,1)`:

1. `y_k = Π_K( x_k − F(x_k) )`; set residual direction `r_k = x_k − y_k`. If `‖r_k‖` small, stop.
2. **Armijo search on the segment** `[x_k, y_k]`: find the smallest integer `m ≥ 0` so that, with
   `z_k = x_k − γᵐ r_k`, the vector `F(z_k)` strictly separates `x_k` from the solution set
   (the paper's inner inequality, of the form `⟨F(z_k), r_k⟩ ≥ σ ‖r_k‖²` up to its exact constant).
3. **Project onto `K ∩ halfspace`** via the explicit double projection:
   ```
   x_{k+1} = Π_K(  x_k − [ ⟨F(z_k), x_k − z_k⟩ / ‖F(z_k)‖² ] · F(z_k)  )
   ```

> **To verify before coding:** lift the precise line-search inequality and the projection formula
> from the open-access PDF (Solodov & Svaiter 1999, §7 link below). Do not transcribe the constants
> from memory.

---

## 5. Per-class specialization

### Class 1 — box family (MCP)

**Projector (clamp).** Separable, `O(n)`, strongly semismooth:
```
Π_K(y)_i = mid(l_i, y_i, u_i)        (use ±∞ sentinels for free / orthant coordinates)
```

**Natural-map components** (piecewise affine, strongly semismooth):
- free `(−∞,+∞)`:  `H_i(x) = F_i(x)`            (pure equation row)
- orthant `[0,+∞)`: `H_i(x) = min(x_i, F_i(x))`   (the classic min-map)
- box `[l,u]`:       `H_i(x) = x_i − mid(l_i, x_i − F_i(x), u_i)`

**Generalized Jacobian** `H'(x) = D_a + D_i M`, where `D_a` is the diagonal selector for coordinates
clamped to a bound (identity rows `e_iᵀ`) and `D_i = I − D_a` selects the *inactive*-plus-*free* rows
(rows of `M`). The Newton solve reduces to a system in the principal submatrix `M_II` on the
inactive+free index set.

**Why smoothing is required here, not optional.** With `M_FF` singular (admissible for
pseudomonotone, non-PSD `M`), `M_II` can be singular at some iterates, so a bare semismooth-Newton
step is undefined. Replace the hard `min`/`mid` by a smoothing (e.g. Chen–Harker–Kanzow–Smale):
```
min(a,b) ≈ ½( a + b − sqrt( (a−b)² + 4μ² ) )        ( → min(a,b) as μ → 0 )
```
This yields a **nonsingular** Jacobian for every `μ > 0`. For belt-and-suspenders, add a
Levenberg–Marquardt shift `+ νI` with `ν` tied to the residual norm. **Class 1 needs the explicit `ν`
shift** (Class 2 supplies its own — see below).

Purpose-built reference: Qi–Sun–Zhou 2000 (smoothing Newton for NCP **and box-constrained VI**).

### Class 2 — axis-aligned ellipsoid

**Projector (diagonal secular equation).** Project `y` onto `K`:
```
x_i(λ) = ( y_i + 2 λ d_i c_i ) / ( 1 + 2 λ d_i )
solve  g(λ) = Σ_i d_i ( x_i(λ) − c_i )² − ρ = 0  for the unique λ ≥ 0   (monotone scalar root)
```
`g` is monotone decreasing in `λ`; solve by safeguarded Newton or bisection. Each evaluation is `O(n)`
and touches no matrix. If `y ∈ K` already, `λ = 0` and `Π_K(y) = y`.
(Ball / closed-form is the special case `D = I`; Dai 2006 gives faster schemes for the general case.)

**Local engine: single-multiplier KKT, not a box reformulation.** `x` solves the VI iff
```
M x + q + 2 λ D (x − c) = 0
0 ≤ λ  ⊥  ρ − (x − c)ᵀ D (x − c) ≥ 0         (one scalar complementarity pair)
```
Smooth the scalar complementarity (CHKS/FB `ψ_μ`) and apply Newton to the bordered `(n+1)` system
`Φ_μ(x, λ) = 0`. The top-left Jacobian block is
```
A = M + 2 λ D       (M plus a positive-definite diagonal)
```
i.e. the active constraint (`λ > 0` on the boundary, where the interesting solutions sit) **acts as a
built-in Tikhonov shift**, improving conditioning and helping keep `A` nonsingular. No separate `ν`
shift is needed in Class 2. Eliminate the scalar `λ` by a **Schur complement**: solve `A` against two
right-hand sides (the residual and the border `g = 2 D (x − c)`), then recover `Δλ` from a scalar.

**Generalization path.** If ellipsoidal/conic constraints later proliferate (intersections, multiple
cones), route Class 2 through Second-Order-Cone Complementarity (SOCCP) smoothing Newton so the box
and the cones share one semismooth-Newton framework. Rui & Xu have their own SOCCP extension (2014).

---

## 6. C++20 implementation plan

### Linear-algebra backend (decision needed from user; recommendation below)

Recommended: **Eigen 3.4** — header-only, BSD/MPL2, cross-platform (Windows + Debian), trivial CMake
integration, has dense and sparse types and an (unsupported) GMRES. Alternatives worth one line each:
Blaze (faster dense kernels, heavier build) or a hand-rolled minimal vector/operator layer (maximal
control, more work). This is the user's call; flag it rather than assume.

### Matrix-free core

```cpp
// Pure interface: the solver only ever needs y = M x.
class LinearOperator {
public:
    virtual ~LinearOperator() = default;
    virtual void apply(const Vector& x, Vector& y_out) const = 0; // y_out = M x
    virtual std::size_t dimension() const = 0;
};
// Implementations: SparseOperator (Eigen::SparseMatrix), DenseOperator (Eigen::MatrixXd),
// CallbackOperator (user-supplied mat-vec for the implicit/dense-unpredictable case).
```

```cpp
// Pure interface: Euclidean projection onto K. Must report failure, never silently clamp.
class Projector {
public:
    virtual ~Projector() = default;
    // Returns projection of y onto K. status communicates root-find / inner convergence.
    virtual Vector project(const Vector& y, ProjectionStatus& status) const = 0;
};
// BoxProjector            : clamp with ±inf sentinels (Class 1)
// AxisAlignedEllipsoid    : diagonal secular equation root-find (Class 2)
```

### Modules (each small, single-responsibility)

```
vi/Vector.hpp                 type aliases over the chosen backend
vi/LinearOperator.hpp         + Sparse / Dense / Callback implementations
vi/Projector.hpp              + BoxProjector + AxisAlignedEllipsoidProjector
vi/NaturalMap.hpp             F_nat(x) and merit θ(x) as free (pure) functions
vi/SolodovSvaiter.{hpp,cpp}   global safeguard step (matrix-free)
vi/SmoothingFunction.hpp      CHKS / smoothed-FB strategy (μ-parameterized), pure
vi/SmoothingNewton.{hpp,cpp}  local engine; assembles smoothed residual + Jacobian-apply
vi/Gmres.{hpp,cpp}            inexact linear solve against a LinearOperator (restarted, forcing term)
vi/EllipsoidKkt.{hpp,cpp}     Class-2 single-multiplier system + Schur complement
vi/HybridSolver.{hpp,cpp}     switching logic, μ/ν schedules, stopping test
tests/                        unit + regression tests
CMakeLists.txt                C++20, locate backend, warnings high
```

### Inner solver and schedules

- **GMRES** operates only through `LinearOperator::apply`, so the same code serves sparse and dense
  `M`. Apply the Jacobian *implicitly*: for Class 1 the Jacobian-vector product is
  `(D_a + D_i M) v` (plus the smoothing/`νI` terms); for Class 2 it is `A v = M v + 2λ D v` inside the
  Schur complement. Never form the Jacobian densely.
- **Forcing term** (inexactness): Eisenstat–Walker schedule so early Newton steps solve loosely and
  late steps solve tightly — this is where the performance comes from.
- **`μ` schedule:** decrease geometrically on accepted steps; never below a floor tied to `tol`.
- **`ν` (Class 1 only):** `ν_k = min(ν_max, c · ‖F_nat(x_k)‖)` so it vanishes near the solution and
  does not spoil the quadratic rate.

### Suggested test problems (correctness gate)

1. Monotone LCP with a known closed-form solution (sanity).
2. A **pseudomonotone, non-PSD** affine example (construct from Gowda 1990 / Crouzeix–Schaible 1996)
   to exercise the regime the user actually cares about and confirm the safeguard engages.
3. Affine VI over an axis-aligned ellipsoid with a boundary solution (`λ > 0`) and an interior
   solution (`λ = 0`), to test both KKT branches and the secular-equation projector.
4. A box instance with a deliberately singular `M_FF` to confirm the `μ`/`ν` regularization keeps the
   Newton step defined.

---

## 7. Open items to confirm with the user before/while coding

- **Linear-algebra backend** (Eigen vs alternative) — see §6.
- **How `M` is supplied:** explicit `Eigen::SparseMatrix` / `Eigen::MatrixXd`, or only as a mat-vec
  callback? Determines which `LinearOperator` implementations to prioritize.
- **Smoothing function choice:** CHKS smoothed-min vs smoothed Fischer–Burmeister. Pick one and cite;
  the user dislikes unexplained defaults.
- **Exact Solodov–Svaiter constants:** transcribe from the open PDF, do not reconstruct from memory.
- **Matrix-class check (non-blocking):** if the user can confirm `M` lies in the PSBD/sufficient
  class, the smoothing-Newton step inherits stronger standalone guarantees; if not, the projection
  safeguard remains the sole certificate of global convergence. Either way the architecture stands.
- **Asymptotic-rate risk:** with `M_FF` singular the limiting solution may be non-isolated, which can
  degrade the local rate from quadratic toward linear even with smoothing. Pseudomonotonicity + the
  safeguard still guarantee convergence; only the rate is at stake. An error-bound check on
  `‖F_nat‖` indicates whether the iterate is in the fast regime.

---

## 8. References

Open-access links are marked **[open]**; verify before relying on a paywalled item.

**Global projection method (the safeguard).**
- M. V. Solodov, B. F. Svaiter, "A new projection method for variational inequality problems,"
  *SIAM J. Control Optim.* 37(3):765–776, 1999.
  **[open]** https://pages.cs.wisc.edu/~solodov/solsva99proj.pdf
- M. V. Solodov, B. F. Svaiter, "A class of globally convergent algorithms for pseudomonotone
  variational inequalities," in *Complementarity: Applications, Algorithms and Extensions*, Applied
  Optimization vol. 50, Springer, 2001. (Subsumes a Josephy–Newton step — the hybrid in §3.)
- M. V. Solodov, B. F. Svaiter, "A truly globally convergent Newton-type method for the monotone
  nonlinear complementarity problem," *SIAM J. Optim.* 10:605–625, 2000.
  (Author copies on Solodov's page, same directory as the 1999 PDF above.)

**Contrast / alternative projection methods.**
- G. M. Korpelevich, extragradient method, *Ekonomika i Mat. Metody* 12:747–756, 1976.
- P. Tseng, "A modified forward-backward splitting method for maximal monotone mappings,"
  *SIAM J. Control Optim.* 38(2):431–446, 2000. DOI 10.1137/S0363012998338806.
- Y. Censor, A. Gibali, S. Reich, "The subgradient extragradient method...," *J. Optim. Theory Appl.*
  148(2):318–335, 2011. DOI 10.1007/s10957-010-9757-3.
- Yu. Malitsky, "Golden ratio algorithms for variational inequalities," *Math. Program.*
  184:383–410, 2020. **[open]** https://arxiv.org/abs/1803.08832 ·
  https://optimization-online.org/wp-content/uploads/2018/05/6598.pdf · code:
  https://gitlab.gwdg.de/malitskyi/graal.git
- A. Alacaoglu, A. Böhm, Yu. Malitsky, "Beyond the Golden Ratio for Variational Inequality
  Algorithms," *JMLR* 24(172):1–33, 2023. **[open]** https://www.jmlr.org/papers/v24/22-1488.html ·
  https://arxiv.org/abs/2212.13955

**Smoothing / semismooth Newton (the local engine).**
- S.-P. Rui, C.-X. Xu, "A smoothing inexact Newton method for nonlinear complementarity problems,"
  *J. Comput. Appl. Math.* 233(9):2332–2338, 2010. DOI 10.1016/j.cam.2009.10.018.
  Open companion presenting the inexact smoothing scheme: Wan et al., *Abstract and Applied Analysis*
  2015, **[open]** https://onlinelibrary.wiley.com/doi/10.1155/2015/731026
- L. Qi, D. Sun, G. Zhou, "A new look at smoothing Newton methods for nonlinear complementarity
  problems and box constrained variational inequalities," *Math. Program.* 87:1–35, 2000.
- T. De Luca, F. Facchinei, C. Kanzow, "A semismooth equation approach to the solution of nonlinear
  complementarity problems," *Math. Program.* 75:407–439, 1996.
- S.-P. Rui, C.-X. Xu, "An inexact smoothing method for SOCCPs based on a one-parametric class of
  smoothing function," *Appl. Math. Comput.* 241:167–182, 2014.
- M. Fukushima, Z.-Q. Luo, P. Tseng, "Smoothing functions for second-order-cone complementarity
  problems," *SIAM J. Optim.* 12:436–460, 2002.

**Projection onto an ellipsoid (Class 2).**
- Y.-H. Dai, "Fast algorithms for projection on an ellipsoid," *SIAM J. Optim.* 16:986–1006, 2006.
  https://epubs.siam.org/doi/pdf/10.1137/040613305

**Matrix-class background for pseudomonotone affine maps (the user's condition).**
- M. S. Gowda, "Affine pseudomonotone mappings and the linear complementarity problem,"
  *SIAM J. Matrix Anal. Appl.* 11:373–380, 1990.
- J.-P. Crouzeix, S. Schaible, "Generalized monotone affine maps," *SIAM J. Matrix Anal. Appl.*
  17:992–997, 1996.
- J.-P. Crouzeix, A. Hassouni, A. Lahlou, S. Schaible, "Positive subdefinite matrices, generalized
  monotonicity and linear complementarity problems," *SIAM J. Matrix Anal. Appl.*, 1999.
- J.-P. Crouzeix, "Pseudomonotone variational inequality problems: existence of solutions,"
  *Math. Program.* 78:305–314, 1997.

**Standard treatise (theory, existence, semismooth Newton on natural/normal maps).**
- F. Facchinei, J.-S. Pang, *Finite-Dimensional Variational Inequalities and Complementarity
  Problems*, Vols. I & II, Springer, 2003.

---

## 9. One-paragraph summary for the new instance

Build a C++20 (CLion/CMake, Windows + Debian) solver for `VI(M, q, K)` with `M` pseudomonotone (not
PSD) and cheap projection. Use a matrix-free Solodov–Svaiter projection method as a globally
convergent safeguard, accelerated by a Rui–Xu–style inexact (truncated-GMRES) smoothing Newton step
selected by sufficient decrease of the natural-map merit `θ = ½‖F_nat‖²`. Two `K` classes share one
Newton kernel and differ only in the projector and the Jacobian shift: Class 1 (free+orthant+box, MCP)
uses the clamp projector and the `D_a + D_i M` semismooth Jacobian, and **requires** `μ`-smoothing plus
an explicit `νI` shift because the retained, possibly singular free block can make the Jacobian
singular; Class 2 (axis-aligned ellipsoid) uses the diagonal secular-equation projector and a
single-multiplier KKT system whose block `M + 2λD` is **self-regularizing**, with the scalar
multiplier removed by a Schur complement. Keep `M` implicit everywhere (mat-vec only) so mixed/unknown
sparsity is absorbed by GMRES. Honor the user's coding conventions (braces always, no silent defaults,
small functions, referential transparency, American spelling). Confirm the open items in §7 before
finalizing.

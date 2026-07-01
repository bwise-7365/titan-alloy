# visolver — variational-inequality / complementarity solver (C++20 / Eigen)

A C++20 solver for variational-inequality / nonlinear-complementarity problems
(VINCP), ported from GNU Octave. All code is in namespace `VINCP`. Two layers
sit over one shared core:

- **Inner solver `dHan06`** — the self-adaptive projection method of Deren Han
  (2006), *Solving linear variational inequality problems by a self-adaptive
  projection method*. It solves the **linear** VI

      find x in K such that (M x + q) . (w - x) >= 0  for all w in K

  for any `K` you can project onto. The set enters only through a `Projector`,
  so the same routine handles the non-negative orthant, a mixed free/orthant
  set, or anything else. Two projectors are supplied: `projectNonnegative`
  (onto `R_+^n`) and `makeMixedProjector(numFree)` (onto
  `R^numFree x R_+^(n-numFree)`).

- **Outer driver `solveVI`** — a Josephy-Newton loop for the **nonlinear** mixed
  VI over `K = R^n x R_+^m`,

      find z = (x, y) in K such that F(z) . (w - z) >= 0 for all w in K,

  equivalently `H(x, y) = 0` and `0 <= G(x, y) _|_ y >= 0`, with `F = (H, G)`.
  Supply the model as a `VIModel` (`n`, `m`, and the `H`, `G` callables). Each
  outer step linearizes `F` with a central finite-difference Jacobian and solves
  the resulting affine VI over the same `K` with `dHan06`, using
  `makeMixedProjector(n)` — so the mixed free/non-negative structure is carried
  by the projector and no Schur complement is needed.

## Layout

    include/vincp.hpp         core: VIResult, VIModel, evaluateF, projectors
    lib/vincp.cpp             core implementation
    include/dhan06.hpp        inner LVI solver -- interface
    lib/dhan06.cpp            inner LVI solver -- implementation
    include/fdjacobian.hpp    central-difference Jacobian -- interface
    lib/fdjacobian.cpp        central-difference Jacobian -- implementation
    include/josephynewton.hpp outer Josephy-Newton driver -- interface
    lib/josephynewton.cpp     outer Josephy-Newton driver -- implementation
    include/utils.hpp         shared helpers (e.g. printVector) -- interface
    lib/utils.cpp             shared helpers -- implementation
    src/main.cpp              inner-solver demo (known LCP)
    src/driver_demo.cpp       outer-driver demo (known nonlinear VI)
    test/lcp_random_test.cpp  random indefinite LCP (stress test)
    test/lcp_psd_test.cpp     random monotone LCP, M = A^T A (guaranteed convergent)
    CMakeLists.txt            static library `vincp` + demos + tests + ctest

The library is the static target `vincp` (sources in `lib/`); the demos and
tests link against it.

## Outer-driver design choices

- **Full Newton step** (no damping).
- **Central-difference Jacobian**, relative step `eps^(1/3)` (the real-arithmetic
  analogue of the Octave complex-step Jacobian), snapped to an exactly
  representable width to limit cancellation. See `fdjacobian.hpp`.
- **Natural-residual convergence**: stop when `||z - Pi_K(z - F(z))||^2 <
  outerTol`. The squared metric matches the inner solver's `magTol` convention.

## Build

Eigen (header-only) is the only dependency: there is nothing to build or install
for it. The build finds it through the `EIGEN3_INCLUDE_DIR` cache variable, which
must point at the directory that *contains* the `Eigen/` folder (the checkout
root, not the `Eigen` subfolder). It defaults to `C:/repos/ghub-ext/eigen`;
override for other machines:

    # Debian/Ubuntu (sudo apt install libeigen3-dev):
    cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DEIGEN3_INCLUDE_DIR=/usr/include/eigen3
    # Windows default location:
    cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

Then:

    cmake --build build
    ctest --test-dir build --output-on-failure

Run a single test by name, or run any executable directly:

    ctest --test-dir build -R lcp_psd_test --output-on-failure

CLion: open the folder; it picks up `CMakeLists.txt` directly (build tree
`cmake-build-debug/`).

## Notes on fidelity to the Octave source

- `magTol` (and the outer `outerTol`, and `VIResult::residual`) are compared
  against the **squared** residual norm `dot(e, e)`, exactly as in the original;
  they are not norms.
- The algorithmic constants (`gamma = 1.6`, `mu = 1.05`, `beta0 = 0.5`,
  `tau0 = 0.5`, `n = 10`, divergence guard factor `100`) live in `DHan06Params`
  with those defaults.
- The NaN check and the divergence guard (`mag` must stay below
  `divergenceFactor * initialMag`) are preserved, and throw `std::runtime_error`
  rather than substituting a value. A non-finite linear solve also throws.
- The inner solve `(I + beta_k M) y = e` uses Eigen `partialPivLu` (LU with
  partial pivoting), matching Octave's general-matrix `linsolve`.
- Han's method is guaranteed to converge only for a **monotone** problem (`M`
  positive semidefinite). This is why `lcp_random_test` (indefinite `M`) may not
  converge while `lcp_psd_test` (`M = A^T A`) always does.

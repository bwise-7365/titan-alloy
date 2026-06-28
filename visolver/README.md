# dHan06 — linear variational inequality solver (C++20 / Eigen)

C++20 translation of the Octave routine `dHan06`, which solves the linear
variational inequality

    find x in K such that (M x + q) . (w - x) >= 0  for all w in K

by the self-adaptive projection method of Deren Han (2006), *Solving linear
variational inequality problems by a self-adaptive projection method*.

The set `K` enters only through the projector `Pr`, so the same routine solves
any LVI for which you can project. Two ready-made projectors are supplied:
`projectNonnegative` (onto R_+^n) and `makeMixedProjector(numFree)` (onto
`R^numFree x R_+^(n-numFree)`, matching the free/non-negative (x, y) partition).

## Layout

    include/dhan06.hpp   public interface
    src/dhan06.cpp       implementation
    src/main.cpp         demo / regression test (known-solution LCP)
    CMakeLists.txt       build (static library + demo + ctest)

## Build

Eigen (header-only) is the only dependency.

- Debian/Ubuntu: `sudo apt install libeigen3-dev`
- Windows: `vcpkg install eigen3`, then configure with the vcpkg toolchain file.

Then, from the project root:

    cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
    cmake --build build
    ctest --test-dir build --output-on-failure

CLion: open the folder; it picks up `CMakeLists.txt` directly.

## Notes on fidelity to the Octave source

- `magTol` is compared against the **squared** residual norm `dot(e, e)`,
  exactly as in the original; it is not a norm.
- The algorithmic constants (`gamma = 1.6`, `mu = 1.05`, `beta0 = 0.5`,
  `tau0 = 0.5`, `n = 10`, divergence guard factor `100`) live in
  `DHan06Params` with those defaults.
- The NaN check and the divergence guard (`mag` must stay below
  `divergenceFactor * initialMag`) are preserved, and throw `std::runtime_error`
  rather than substituting a value. A non-finite linear solve also throws.
- The inner solve `(I + beta_k M) y = e` uses Eigen `partialPivLu` (LU with
  partial pivoting), matching Octave's general-matrix `linsolve`.

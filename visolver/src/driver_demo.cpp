// Copyright Ben Paul Wise. All Rights Reserved.
#include "josephynewton.hpp"
#include "utils.hpp"

#include <Eigen/Dense>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <random>
#include <string>

using std::printf;
using Eigen::Index;
using Eigen::MatrixXd;
using Eigen::VectorXd;
using VINCP::printVector;

// Demonstration / regression test for the outer Josephy-Newton driver, on a
// larger nonlinear mixed VI with a known, randomly generated solution and
// genuine nonlinear cross-terms.
//
// The map is  F(z) = A^T ( u.^3 + u ) + k,  with the linear forms  u = A z
// (z = (x, y), elementwise cube). It splits as
//     H(x, y) = F(z).head(n),   G(x, y) = F(z).tail(m).
// Because each u_k = (A z)_k mixes all variables, both the cubic term A^T u.^3
// and the linear term A^T u = A^T A z couple x with y and the components with
// one another -- i.e. real nonlinear cross-terms, not a separable diagonal cube.
//
// This is grad(phi) of the convex
//     phi(z) = sum_k ( u_k^4 / 4 + u_k^2 / 2 ) + k^T z,   u = A z,
// so F is monotone and its Jacobian A^T diag(3 u_k^2 + 1) A is PSD at every
// iterate (positive definite a.s.), keeping Han's inner method convergent and
// making z* the unique solution.
//
// Construction (the "add constants to match" pattern):
//   - draw a random free block x*, and a complementary (y*, w*) pair
//     (0 <= w* _|_ y* >= 0) exactly as the LCP tests do;
//   - draw the random forms matrix A;
//   - choose the constant vector k so that at z* = (x*, y*),
//       H(x*, y*) = 0  and  G(x*, y*) = w*,  i.e.
//       k = [0; w*] - A^T ( (A z*).^3 + (A z*) ).

// Name of the z-component j: x0..x(n-1) are free, then y0..y(m-1).
static std::string varName(Index j, Index n) {
    if (j < n) {
        return "x" + std::to_string(static_cast<long long>(j));
    }
    return "y" + std::to_string(static_cast<long long>(j - n));
}

// Print a linear form  name = c0 x0 + c1 y0 + ... , skipping ~zero coefficients.
static void printLinearForm(const char* name, const VectorXd& coeffs, Index n) {
    printf("  %s =", name);
    bool any = false;
    for (Index j = 0; j < coeffs.size(); ++j) {
        const double a = coeffs(j);
        if (std::abs(a) > 1.0e-12) {
            printf(" %+.3f %s", a, varName(j, n).c_str());
            any = true;
        }
    }
    if (!any) {
        printf(" 0");
    }
    printf("\n");
}

// Print one field component  name = k_row + sum_k A(k,row) (u_k^3 + u_k).
static void printFieldComponent(const char* name, const MatrixXd& A,
                                const VectorXd& k, Index row) {
    printf("  %s = %+.3f", name, k(row));
    for (Index kk = 0; kk < A.rows(); ++kk) {
        const double a = A(kk, row);
        if (std::abs(a) > 1.0e-12) {
            printf(" %+.3f (u%td^3 + u%td)", a, kk, kk);
        }
    }
    printf("\n");
}

int main() {
    VINCP::ScopedUtcTimer timer("lvi_vi_demo");
    const Index n = 4;                       // free block dimension (edit here)
    const Index m = 4;                       // non-negative block dimension
    const Index d = n + m;
    const std::uint_fast32_t seed = 123456u; // PRNG seed

    const int    xLo = -8,   xHi = 8;        // integer range for the free block x*
    const int    yLo = 1,    yHi = 8;        // integer range for the (y*, w*) pair
    const double aLo = -1.0, aHi = 1.0;      // range for the forms matrix A

    printf("Build the expected solution with n=%td free variables and m=%td non-negative\n",
        n, m); // an Index is really long
    const double solTol = 1.0e-6;            // "very close" bound on ||z_solved - z*||

    // --- Solver parameters for THIS problem (set per problem here, not in the
    // headers). The Armijo-damped Newton step (params.armijo) converges
    // monotonically instead of chattering at the non-smooth solution.
    //
    // WARNING: the lever for a tighter outerTol is innerMagTol. The outer
    // natural-residual floor is set (linearly) by the INNER solver tolerance,
    // NOT by the finite-difference Jacobian's step or order. To drive outerTol
    // lower, lower innerMagTol -- do not touch the FD settings.
    //
    // REMEMBER "SQUARE": magTol / outerTol / residual are SQUARED residual norms,
    // so a tolerance t is an actual error (residual norm) of sqrt(t). Read it by
    // halving the exponent: innerMagTol = 1e-16 -> inner error ~1e-8;
    // outerTol = 1e-10 -> outer error ~1e-5. A tolerance can only go as low as
    // ~machine-eps (~2e-16) before it is below the squared rounding noise.
    const double outerTol      = 1.0e-10;    // stop when squared natural residual < this
    const int    outerIterMax  = 200;        // outer Josephy-Newton iteration cap
    const int    outerIterFreq = 1;          // outer logging frequency (<= 0 disables)
    const double innerMagTol   = 1.0e-16;    // dHan06 inner SQUARED-residual tol (error ~1e-8)
    const int    innerIterMax  = 100000;     // dHan06 inner iteration cap

    std::mt19937 rng(seed);
    std::uniform_int_distribution<int> xDist(xLo, xHi);

    // Free block x*, then the complementary (y*, w*) pair.
    VectorXd xStar(n);
    for (Index i = 0; i < n; ++i) {
        xStar(i) = static_cast<double>(xDist(rng));
    }
    VectorXd wStar, yStar;
    VINCP::makeComplementaryPair(m, rng, yLo, yHi, wStar, yStar);

    VectorXd zStar(d);
    zStar << xStar, yStar;

    // Build the cubic F(z) = A^T ( u.^3 + u ) + k (u = A z) via the shared
    // generator, forcing the PSD / gradient form so F is monotone, with k set so
    // that H(x*, y*) = 0 and G(x*, y*) = w* -- i.e. F(z*) = [0; w*].
    VectorXd target(d);
    target << VectorXd::Zero(n), wStar;
    const VINCP::CubicProblem prob =
        VINCP::makeCubicProblem(d, d, rng, zStar, target, /*forcePSD=*/true, aLo, aHi);

    const VINCP::VIModel model = VINCP::makeVIModel(n, m, prob.F);

    // Show the actual H and G functions (free vars x0.., non-negative y0..).
    printf("\nlinear forms  u = A z:\n");
    for (Index kk = 0; kk < d; ++kk) {
        char lbl[16];
        std::snprintf(lbl, sizeof lbl, "u%td", kk);
        printLinearForm(lbl, prob.A.row(kk).transpose(), n);
    }
    printf("\nF(z) = A^T ( u.^3 + u ) + k:\n");
    printf(" H (free block, solved to H = 0):\n");
    for (Index i = 0; i < n; ++i) {
        char lbl[16];
        std::snprintf(lbl, sizeof lbl, "H%td", i);
        printFieldComponent(lbl, prob.A, prob.k, i);
    }
    printf(" G (non-negative block, 0 <= G _|_ y >= 0):\n");
    for (Index i = 0; i < m; ++i) {
        char lbl[16];
        std::snprintf(lbl, sizeof lbl, "G%td", i);
        printFieldComponent(lbl, prob.A, prob.k, n + i);
    }

    const VectorXd z0 = VectorXd::Zero(d);

    VINCP::JosephyNewtonParams params;
    params.outerTol      = outerTol;
    params.outerIterMax  = outerIterMax;
    params.outerIterFreq = outerIterFreq;

    // Inner LVI solver: Han's self-adaptive method, with the inner controls bound in.
    const VINCP::InnerSolver innerSolver =
        VINCP::makeDHan06Solver(innerMagTol, innerIterMax, 0);

    const VINCP::OuterLogger logger =
        [](int iter, int itMax, double res, double tol) {
            printf("outer iter %3d/%3d, residual^2 %.3e/%.3e\n", iter, itMax, res, tol);
        };

    const VINCP::VIResult r = VINCP::solveVI(model, z0, innerSolver, params, logger);

    const double solErr = (r.z - zStar).norm();
    printf("\nn = %lld free, m = %lld non-negative (d = %lld)\n",
           static_cast<long long>(n), static_cast<long long>(m),
           static_cast<long long>(d));
    const VectorXd wSolved = model.G(r.z.head(n), r.z.tail(m));  // implied w at the solution
    printVector("z expected", zStar);
    printVector("z solved  ", r.z);
    printf("\nw = G(x,y)\n");
    printVector("w expected", wStar);
    printVector("w implied ", wSolved);
    VINCP::printSolveStats("solveVI", r);
    printf("solution error = %.3e\n", solErr);

    if (r.converged && solErr < solTol) {
        printf("PASS (within %.1e)\n", solTol);
        return 0;
    }
    printf("FAIL (exceeds %.1e)\n", solTol);
    return 1;
}
// Copyright Ben Paul Wise. All Rights Reserved.

// Copyright Ben Paul Wise. All Rights Reserved.
#include "smoothingnewton.hpp"
#include "utils.hpp"
#include "testsupport.hpp"

#include <Eigen/Dense>
#include <gtest/gtest.h>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <random>

using namespace VIMCP;

// Validation of the smoothing solvers on a convex QP with a known KKT point:
//     min 1/2 ||x||^2  s.t.  A x >= b,     x in R^N, A in R^{M x N}.
// Its KKT system is
//     x - A^T y = 0,     0 <= y _|_ (A x - b) >= 0,
// so H(x,y) = x - A^T y and G(x,y) = A x - b. Rather than hand-pick a solution, the
// instance is BUILT around a known KKT point (N = 10, M = 7): draw a random A, choose
// multipliers y* >= 0 with a strict active/inactive pattern, set x* = A^T y*
// (stationarity), and pick b so that A x* - b = s* with s*_i = 0 on the active set
// (y*_i > 0) and s*_i > 0 on the inactive set (y*_i = 0). The QP is strictly convex,
// so x* is its unique minimizer; the four active constraint gradients are a.s.
// independent (4 rows in R^10 => LICQ), so y* is the unique multiplier. The solver
// must recover (x*, y*), drive u -> 0, and satisfy feasibility and complementarity.
TEST(SmoothingNewton, ConvexQpKktPoint) {
    const bool   latex   = false;
    const double solTol  = 1.0e-6;   // ||x - x*||, ||y - y*||
    const double uTol    = 1.0e-6;   // final smoothing parameter |u|
    const double feasTol = 1.0e-6;   // min(A x - b) >= -feasTol
    const double compTol = 1.0e-6;   // max_i |y_i s_i|

    const Index N = 10;   // primal dimension
    const Index M = 7;    // number of A x >= b constraints

    // Reproducible random A (M x N), entries in [-1, 1].
    const std::uint_fast32_t seed = 20260703u;
    std::mt19937 rng(seed);
    std::uniform_real_distribution<double> aDist(-1.0, 1.0);
    MatrixXd A(M, N);
    for (Index r = 0; r < M; ++r) {
        for (Index c = 0; c < N; ++c) {
            A(r, c) = aDist(rng);
        }
    }

    // Multipliers y* >= 0: constraints 0, 2, 4, 5 active (positive), 1, 3, 6 inactive.
    VectorXd yStar = VectorXd::Zero(M);
    yStar(0) = 1.5;
    yStar(2) = 2.0;
    yStar(4) = 0.5;
    yStar(5) = 3.0;

    // Stationarity: x* = A^T y*.
    const VectorXd xStar = A.transpose() * yStar;

    // Slacks s* >= 0: zero on the active set, strictly positive on the inactive set,
    // so 0 <= y* _|_ s* >= 0 holds. Then b = A x* - s* makes A x* - b = s*.
    VectorXd sStar = VectorXd::Zero(M);
    sStar(1) = 4.0;
    sStar(3) = 2.5;
    sStar(6) = 1.0;
    const VectorXd b = A * xStar - sStar;

    // KKT maps: H(x,y) = x - A^T y (stationarity of 1/2||x||^2); G(x,y) = A x - b.
    const MixedField H = [A](const VectorXd& x, const VectorXd& y) -> VectorXd {
        return x - A.transpose() * y;
    };
    const MixedField G = [A, b](const VectorXd& x, const VectorXd& y) -> VectorXd {
        (void)y;
        return A * x - b;
    };

    printComment(latex, "smoothing-Newton test: QP  min 1/2||x||^2  s.t. A x >= b  (N=10, M=7)");
    printVector("x* (known)", xStar);
    printVector("y* (known)", yStar);

    const VectorXd x0 = VectorXd::Zero(N);   // infeasible start is fine (not interior-point)
    const VectorXd y0 = VectorXd::Ones(M);

    // KKT check: recovered (x, y) match the known optimum, u -> 0, and the point is
    // primal-feasible and complementary. Prints the decoded x, y, s.
    const CheckFn kktCheck = [&](const VIResult& r) -> CheckResult {
        const SmoothingSolution sol = smoothingDecode(r, N, M);
        printVector("x", sol.x);
        printVector("y", sol.y);
        printVector("s", sol.s);
        const double xErr    = (sol.x - xStar).norm();
        const double yErr    = (sol.y - yStar).norm();
        const double minFeas = (A * sol.x - b).minCoeff();
        double maxComp = 0.0;
        for (Index i = 0; i < sol.y.size(); ++i) {
            maxComp = std::max(maxComp, std::abs(sol.y(i) * sol.s(i)));
        }
        const bool pass = (std::abs(sol.u) < uTol) && (xErr < solTol) && (yErr < solTol)
                          && (minFeas >= -feasTol) && (maxComp < compTol);
        char buf[320];
        std::snprintf(buf, sizeof buf,
                      "u = %.3e (|u| < %.1e), ||x-x*|| = %.3e (< %.1e), ||y-y*|| = %.3e (< %.1e), "
                      "min(Ax-b) = %.3e (>= %.1e), max|y_i s_i| = %.3e (< %.1e)",
                      sol.u, uTol, xErr, solTol, yErr, solTol, minFeas, -feasTol, maxComp, compTol);
        return CheckResult{ pass, std::string(buf) };
    };

    // The two smoothing functions, reused by both solver paths below. (x0, y0) are
    // bound into each solve, so the SolveFn's z0 argument is unused.
    struct Smoother { const char* name; SmoothingFunction phi; };
    const Smoother smoothers[] = {
        { "smoothedFischerBurmeister (default)",  smoothedFischerBurmeister },
        { "smoothedFB_WZ (Wu-Zhao 2013, eq. 2)",  smoothedFB_WZ },
    };

    // Least-squares smoothingNewtonSolve -- PRINTED CONTRAST, not gated. On this
    // harder instance u collapses to ~0 in one step (it is a linear residual row),
    // so the smoothing evaporates and the solve stalls short of the KKT point. Shown
    // side by side with the continuation path below, but deliberately not asserted.
    printComment(latex, "--- least-squares smoothingNewtonSolve (contrast, NOT gated) ---");
    for (const Smoother& sm : smoothers) {
        printComment(latex, sm.name);
        SmoothingNewtonParams params;   // u0 = 1.0, dampedNewton defaults
        params.smoothing = sm.phi;
        const CheckResult cr = kktCheck(smoothingNewtonSolve(H, G, x0, y0, params));
        std::printf("  [contrast] %s -> %s\n", cr.pass ? "PASS" : "FAIL", cr.report.c_str());
    }

    // Continuation smoothingContinuationSolve -- the real assertion. u is an outer
    // parameter shrunk on a schedule (never a Newton unknown), so it cannot collapse;
    // both smoothing functions must recover (x*, y*).
    printComment(latex, "--- continuation smoothingContinuationSolve (gated) ---");
    for (const Smoother& sm : smoothers) {
        SCOPED_TRACE(sm.name);
        printComment(latex, sm.name);
        SmoothingContinuationParams params;   // u0=1, sigma=0.1, muMin=1e-12 defaults
        params.smoothing = sm.phi;
        const SolveFn solve = [&](const VectorXd&) { return smoothingContinuationSolve(H, G, x0, y0, params); };
        expectSolvePasses(solve, x0, { kktCheck });
    }
}

// Least-squares smoothingNewtonSolve smoke test on a small, well-conditioned QP where
// it DOES converge: min 1/2||x||^2 s.t. A x >= b with A = [[1,1],[1,0]], b = (2,-5);
// unique KKT point x* = (1,1), y* = (1,0). Guards that the least-squares path still
// works on easy problems (the harder N=10/M=7 instance above stalls it, which is why
// that test gates on the continuation solver instead).
TEST(SmoothingNewton, SmallQpLeastSquaresSmoke) {
    const double solTol = 1.0e-6, uTol = 1.0e-6, feasTol = 1.0e-6, compTol = 1.0e-6;

    MatrixXd A(2, 2);
    A << 1.0, 1.0,
         1.0, 0.0;
    VectorXd b(2);     b     << 2.0, -5.0;
    VectorXd xStar(2); xStar << 1.0, 1.0;
    VectorXd yStar(2); yStar << 1.0, 0.0;

    const MixedField H = [A](const VectorXd& x, const VectorXd& y) -> VectorXd {
        return x - A.transpose() * y;
    };
    const MixedField G = [A, b](const VectorXd& x, const VectorXd& y) -> VectorXd {
        (void)y;
        return A * x - b;
    };
    const VectorXd x0 = VectorXd::Zero(2);
    const VectorXd y0 = VectorXd::Ones(2);

    const CheckFn kktCheck = [&](const VIResult& r) -> CheckResult {
        const SmoothingSolution sol = smoothingDecode(r, 2, 2);
        const double xErr    = (sol.x - xStar).norm();
        const double yErr    = (sol.y - yStar).norm();
        const double minFeas = (A * sol.x - b).minCoeff();
        double maxComp = 0.0;
        for (Index i = 0; i < sol.y.size(); ++i) {
            maxComp = std::max(maxComp, std::abs(sol.y(i) * sol.s(i)));
        }
        const bool pass = (std::abs(sol.u) < uTol) && (xErr < solTol) && (yErr < solTol)
                          && (minFeas >= -feasTol) && (maxComp < compTol);
        char buf[256];
        std::snprintf(buf, sizeof buf,
                      "u = %.2e, ||x-x*|| = %.2e, ||y-y*|| = %.2e, min(Ax-b) = %.2e, max|y_i s_i| = %.2e",
                      sol.u, xErr, yErr, minFeas, maxComp);
        return CheckResult{ pass, std::string(buf) };
    };

    SmoothingNewtonParams params;   // default smoothed Fischer-Burmeister
    const SolveFn solve = [&](const VectorXd&) { return smoothingNewtonSolve(H, G, x0, y0, params); };
    expectSolvePasses(solve, x0, { kktCheck });
}
// Copyright Ben Paul Wise. All Rights Reserved.

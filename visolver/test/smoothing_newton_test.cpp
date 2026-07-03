// Copyright Ben Paul Wise. All Rights Reserved.
#include "smoothingnewton.hpp"
#include "utils.hpp"
#include "testsupport.hpp"

#include <Eigen/Dense>
#include <gtest/gtest.h>

#include <algorithm>
#include <cmath>
#include <cstdio>

using namespace VINCP;

// Validation of smoothingNewtonSolve on a hand-built convex QP with a known KKT
// point:  min 1/2 ||x||^2  s.t.  A x >= b.  Its KKT system is
//     x - A^T y = 0,     0 <= y _|_ (A x - b) >= 0,
// so H(x,y) = x - A^T y and G(x,y) = A x - b. With
//     A = [[1,1],[1,0]],  b = (2,-5),
// the unique KKT point is x* = (1,1), y* = (1,0), s* = A x* - b = (0,6):
// constraint 1 active (y1>0, s1=0), constraint 2 slack (y2=0, s2>0). The solver must
// recover (x*, y*), drive u -> 0, and satisfy feasibility and complementarity.
TEST(SmoothingNewton, ConvexQpKktPoint) {
    const bool   latex   = false;
    const double solTol  = 1.0e-6;   // ||x - x*||, ||y - y*||
    const double uTol    = 1.0e-6;   // final smoothing parameter |u|
    const double feasTol = 1.0e-6;   // min(A x - b) >= -feasTol
    const double compTol = 1.0e-6;   // max_i |y_i s_i|

    MatrixXd A(2, 2);
    A << 1.0, 1.0,
         1.0, 0.0;
    VectorXd b(2);
    b << 2.0, -5.0;

    VectorXd xStar(2); xStar << 1.0, 1.0;
    VectorXd yStar(2); yStar << 1.0, 0.0;

    // KKT maps: H(x,y) = x - A^T y (stationarity of 1/2||x||^2); G(x,y) = A x - b.
    const MixedField H = [A](const VectorXd& x, const VectorXd& y) -> VectorXd {
        return x - A.transpose() * y;
    };
    const MixedField G = [A, b](const VectorXd& x, const VectorXd& y) -> VectorXd {
        (void)y;
        return A * x - b;
    };

    printComment(latex, "smoothing-Newton test: QP  min 1/2||x||^2  s.t. A x >= b");
    printVector("x* (known)", xStar);
    printVector("y* (known)", yStar);

    VectorXd x0(2); x0 << 0.0, 0.0;   // infeasible start is fine (not interior-point)
    VectorXd y0(2); y0 << 1.0, 1.0;

    const SmoothingNewtonParams params;   // u0 = 1.0, FB, dampedNewton defaults

    // KKT check: recovered (x, y) match the known optimum, u -> 0, and the point is
    // primal-feasible and complementary. Prints the decoded x, y, s.
    const CheckFn kktCheck = [&](const VIResult& r) -> CheckResult {
        const SmoothingSolution sol = smoothingDecode(r, 2, 2);   // N = M = 2
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
        char buf[192];
        std::snprintf(buf, sizeof buf,
                      "u = %.3e, ||x-x*|| = %.3e, ||y-y*|| = %.3e, min(Ax-b) = %.3e, max|y_i s_i| = %.3e",
                      sol.u, xErr, yErr, minFeas, maxComp);
        return CheckResult{ pass, std::string(buf) };
    };

    // (x0, y0) are bound into the solver; the SolveFn's z0 argument is unused here.
    const SolveFn solve = [&](const VectorXd&) { return smoothingNewtonSolve(H, G, x0, y0, params); };
    expectSolvePasses(solve, x0, { kktCheck });
}
// Copyright Ben Paul Wise. All Rights Reserved.

// Copyright Ben Paul Wise. All Rights Reserved.
#include "smoothingnewton.hpp"
#include "utils.hpp"

#include <Eigen/Dense>
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <exception>

using Eigen::MatrixXd;
using Eigen::VectorXd;
using std::printf;

// Validation of smoothingNewtonSolve on a hand-built convex QP with a known KKT
// point:  min 1/2 ||x||^2  s.t.  A x >= b.  Its KKT system is
//     x - A^T y = 0,     0 <= y _|_ (A x - b) >= 0,
// so H(x,y) = x - A^T y and G(x,y) = A x - b. With
//     A = [[1,1],[1,0]],  b = (2,-5),
// the unique KKT point is x* = (1,1), y* = (1,0), s* = A x* - b = (0,6):
// constraint 1 active (y1>0, s1=0), constraint 2 slack (y2=0, s2>0). The solver must
// recover (x*, y*), drive u -> 0, and satisfy feasibility and complementarity.
int main() {
    VINCP::ScopedUtcTimer timer("smoothing_newton_test");
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
    const VINCP::MixedField H = [A](const VectorXd& x, const VectorXd& y) -> VectorXd {
        return x - A.transpose() * y;
    };
    const VINCP::MixedField G = [A, b](const VectorXd& x, const VectorXd& y) -> VectorXd {
        (void)y;
        return A * x - b;
    };

    VINCP::printComment(latex, "smoothing-Newton test: QP  min 1/2||x||^2  s.t. A x >= b");
    VINCP::printVector("x* (known)", xStar);
    VINCP::printVector("y* (known)", yStar);

    VectorXd x0(2); x0 << 0.0, 0.0;   // infeasible start is fine (not interior-point)
    VectorXd y0(2); y0 << 1.0, 1.0;

    char line[192];
    const auto tStart = VINCP::utcNow();
    try {
        const VINCP::SmoothingNewtonParams params;   // u0 = 1.0, FB, dampedNewton defaults
        const VINCP::SmoothingResult r = VINCP::smoothingNewtonSolve(H, G, x0, y0, params);
        VINCP::utcElapsed(tStart);

        VINCP::printSolveStats("smoothingNewton", r.solve);
        VINCP::printVector("x", r.x);
        VINCP::printVector("y", r.y);
        VINCP::printVector("s", r.s);

        const double   xErr    = (r.x - xStar).norm();
        const double   yErr    = (r.y - yStar).norm();
        const VectorXd resid   = A * r.x - b;              // = s at the solution
        const double   minFeas = resid.minCoeff();
        double maxComp = 0.0;
        for (Eigen::Index i = 0; i < r.y.size(); ++i) {
            maxComp = std::max(maxComp, std::abs(r.y(i) * r.s(i)));
        }

        std::snprintf(line, sizeof line,
                      "u = %.3e, ||x-x*|| = %.3e, ||y-y*|| = %.3e, min(Ax-b) = %.3e, max|y_i s_i| = %.3e",
                      r.u, xErr, yErr, minFeas, maxComp);
        VINCP::printComment(latex, line);

        const bool pass = (std::abs(r.u) < uTol) && (xErr < solTol) && (yErr < solTol)
                          && (minFeas >= -feasTol) && (maxComp < compTol);
        printf("\n%s\n", pass ? "PASS (recovered the known KKT point)"
                              : "FAIL (off the known KKT point)");
        return pass ? 0 : 1;
    } catch (const std::exception& ex) {
        VINCP::utcElapsed(tStart);
        std::snprintf(line, sizeof line, "smoothingNewtonSolve threw: %s", ex.what());
        VINCP::printComment(latex, line);
        printf("\nFAIL (threw)\n");
        return 1;
    }
}
// Copyright Ben Paul Wise. All Rights Reserved.

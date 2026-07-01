// Copyright Ben Paul Wise. All Rights Reserved.
#include "dhan06.hpp"
#include "utils.hpp"

#include <Eigen/Dense>
#include <cstdio>

using std::printf;

// Demonstration / regression test.
//
// Monotone LCP   0 <= x  _|_  (M x + q) >= 0   solved as an LVI on K = R_+^2.
// Construction with known solution x* = (1, 0):
//     M = [[4, 1], [1, 3]],  q = (-4, 1).
//     M x* + q = (0, 2): component 1 active (x1 > 0, residual 0),
//     component 2 inactive (x2 = 0, residual 2 >= 0).  Complementarity holds.
//
// This is the template for requirement (E): replace M, q, x0 and the expected
// vector with one of your verified Octave cases and compare to a tolerance.
int main() {
    VINCP::ScopedUtcTimer timer("lvi_demo");

    using Eigen::VectorXd;
    Eigen::MatrixXd M(2, 2);
    M << 4.0, 1.0,
         1.0, 3.0;

    VectorXd q(2);
    q << -4.0, 1.0;

    VectorXd xStar(2);
    xStar << 1.0, 0.0;

    const VectorXd x0 = VectorXd::Zero(2);

    const double magTol = 1.0e-14;
    const int iterMax = 100000;
    const int iterFreq = 2000;

    const VINCP::IterationLogger logger =
        [](int iter, int itMax, double mag, double tol) {
            printf("dHan06 iteration %4d/%4d, %.3e/%.3e\n", iter, itMax, mag, tol);
        };

    const VINCP::VIResult r =
        VINCP::dHan06(x0, M, q, VINCP::projectNonnegative,
                    magTol, iterMax, iterFreq, VINCP::DHan06Params{}, logger);

    const double solErr = (r.z - xStar).norm();
    printf("\nsolution       = (%.12f, %.12f)\n", r.z(0), r.z(1));
    printf("expected       = (%.12f, %.12f)\n", xStar(0), xStar(1));
    VINCP::printSolveStats("dHan06", r);
    printf("solution error = %.3e\n", solErr);

    const double solTol = 1.0e-6;
    if (solErr < solTol) {
        printf("PASS (within %.1e)\n", solTol);
        return 0;
    } else {
        printf("FAIL (exceeds %.1e)\n", solTol);
        return 1;
    }
}
// Copyright Ben Paul Wise. All Rights Reserved.

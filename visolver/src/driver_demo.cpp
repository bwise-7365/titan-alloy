#include "josephynewton.hpp"

#include <Eigen/Dense>
#include <cstdio>

using std::printf;
using Eigen::VectorXd;

// Demonstration / regression test for the outer Josephy-Newton driver.
//
// A genuinely nonlinear mixed VI with a known closed-form solution, chosen so
// that every linearization is monotone (positive semidefinite Jacobian), which
// keeps Han's inner method in its convergent regime.
//
// Free block x in R, non-negative block y in R.  F = (H, G) = grad(phi) with
//     phi(x, y) = (1/4) x^4 + (1/2) x^2 + x y + (1/2) y^2 - 2 x,
// so
//     H(x, y) = x^3 + x + y - 2,        (free:  H = 0)
//     G(x, y) = x + y.                  (0 <= G _|_ y >= 0)
// Its Jacobian [[3 x^2 + 1, 1], [1, 1]] is symmetric PSD for all x, so F is
// monotone.  The unique solution is (x*, y*) = (1, 0):
//     H(1, 0) = 1 + 1 + 0 - 2 = 0,
//     G(1, 0) = 1 >= 0 with y* = 0      (complementarity holds),
// and no solution with y > 0 exists (G = 0 there forces x = -y, whence
// H = -y^3 - 2 = 0 has no non-negative root).
//
// This is the template for requirement (E): replace the model and the expected
// vector with one of your verified Octave cases and compare to a tolerance.
int main() {
    VINCP::VIModel model;
    model.n = 1;
    model.m = 1;
    model.H = [](const VectorXd& x, const VectorXd& y) -> VectorXd {
        VectorXd h(1);
        h(0) = x(0) * x(0) * x(0) + x(0) + y(0) - 2.0;
        return h;
    };
    model.G = [](const VectorXd& x, const VectorXd& y) -> VectorXd {
        VectorXd g(1);
        g(0) = x(0) + y(0);
        return g;
    };

    const VectorXd z0 = VectorXd::Zero(2);

    VectorXd zStar(2);
    zStar << 1.0, 0.0;

    VINCP::JosephyNewtonParams params;   // defaults
    params.outerIterFreq = 1;

    const VINCP::OuterLogger logger =
        [](int iter, int itMax, double res, double tol) {
            printf("outer iter %3d/%3d, residual^2 %.3e/%.3e\n", iter, itMax, res, tol);
        };

    const VINCP::VIResult r = VINCP::solveVI(model, z0, params, logger);

    const double solErr = (r.z - zStar).norm();
    printf("\nsolution       = (%.12f, %.12f)\n", r.z(0), r.z(1));
    printf("expected       = (%.12f, %.12f)\n", zStar(0), zStar(1));
    printf("outer iters    = %d\n", r.iter);
    printf("residual^2     = %.3e (squared natural residual)\n", r.residual);
    printf("converged      = %s\n", r.converged ? "true" : "false");
    printf("solution error = %.3e\n", solErr);

    const double solTol = 1.0e-6;
    if (r.converged && solErr < solTol) {
        printf("PASS (within %.1e)\n", solTol);
        return 0;
    }
    printf("FAIL (exceeds %.1e)\n", solTol);
    return 1;
}

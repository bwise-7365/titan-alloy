// Copyright Ben Paul Wise. All Rights Reserved.
#include "utils.hpp"
#include "levenbergmarquardt.hpp"

#include <Eigen/Dense>
#include <cstdint>
#include <cstdio>
#include <exception>
#include <random>

using Eigen::Index;
using Eigen::VectorXd;
using std::printf;

// Unit test for levenbergMarquardtSolve (and, through it, the LM building blocks
// levenbergMarquardtDamp / levenbergMarquardtUpdate). It solves an overdetermined
// nonlinear least squares problem: a cubic map F: R^nIn -> R^nOut with nOut >= nIn
// and a known zero-residual root x* (with mixed-sign components). From a random
// starting point, LM should recover x*.
//
// The cubic is the NON-PSD form of makeCubicProblem: F(x) = g(A x) + k with
// g(u) = u.^3 + u, A drawn nOut x nIn, and k = -g(A x*) so F(x*) = 0. Since g is
// strictly monotone (injective) and A has full column rank almost surely, x* is
// the unique root -- the global least-squares minimum.
//
// LM is a local method, so the start is a random perturbation of x* (a genuine
// random point, but in the basin) that still exercises the full damped iteration.
int main() {
    VINCP::ScopedUtcTimer timer("lm_test");
    const Index nIn  = 10;    // input dimension  (change here)
    const Index nOut = 15;    // output dimension (change here)
    const std::uint_fast32_t seed = 56639775; // VINCP::makeSeed(0, true); // fixed for a reproducible test

    const int    magLo   = 1,    magHi   = 3;     // magnitude range for x* components
    const double aLo     = -0.5, aHi     = 0.5;   // forms-matrix range
    const double startLo = -15.0, startHi = 15.0; // random offset of the start from x*

    const double meritTol = 1.0e-16;  // stop when ||F(x)||^2 < meritTol (SQUARED)
    const int    lmIterMax  = 200;    // outer LM iteration cap
    const int    lmInnerMax = 40;     // max lambda increases per outer step
    const double solTol   = 1.0e-6;   // acceptance bound on ||x - x*||

    std::mt19937 rng(seed);

    // Known root x* with mixed signs: magnitude random in [magLo, magHi], sign
    // alternating so both positive and negative components are present.
    std::uniform_int_distribution<int> magDist(magLo, magHi);
    VectorXd xStar(nIn);
    for (Index i = 0; i < nIn; ++i) {
        const double sign = (i % 2 == 0) ? 1.0 : -1.0;
        xStar(i) = sign * static_cast<double>(magDist(rng));
    }

    // Cubic F: R^nIn -> R^nOut with F(x*) = 0 (non-PSD / general cubic).
    const VectorXd fStar = VectorXd::Zero(nOut);
    const VINCP::CubicProblem prob =
        VINCP::makeCubicProblem(nIn, nOut, rng, xStar, fStar, /*forcePSD=*/false, aLo, aHi);

    // Random starting point (a random offset from x*).
    std::uniform_real_distribution<double> startDist(startLo, startHi);
    VectorXd x0(nIn);
    for (Index i = 0; i < nIn; ++i) {
        x0(i) = xStar(i) + startDist(rng);
    }

    printf("Levenberg-Marquardt: cubic F: R^%td -> R^%td, known root recovery\n",
           static_cast<long long>(nIn), static_cast<long long>(nOut));
    VINCP::printVector("x* (root)", xStar);
    VINCP::printVector("start    ", x0);

    try {
        VINCP::LevenbergMarquardtSolveParams lmp;
        lmp.meritTol = meritTol;
        lmp.iterMax  = lmIterMax;
        lmp.innerMax = lmInnerMax;

        const VINCP::VIResult r = VINCP::levenbergMarquardtSolve(prob.F, x0, lmp);

        const double solErr = (r.z - xStar).norm();
        printf("\n");
        VINCP::printVector("x (solved)", r.z);
        VINCP::printSolveStats("LM", r);
        printf("solution err = %.3e (||x - x*||)\n", solErr);

        if (r.converged && solErr < solTol) {
            printf("PASS (within %.1e)\n", solTol);
            return 0;
        }
        printf("FAIL (exceeds %.1e)\n", solTol);
        return 1;
    } catch (const std::exception& ex) {
        printf("FAIL: levenbergMarquardtSolve threw: %s\n", ex.what());
        return 1;
    }
}
// Copyright Ben Paul Wise. All Rights Reserved.

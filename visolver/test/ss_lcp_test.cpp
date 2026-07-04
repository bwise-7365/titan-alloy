// Copyright Ben Paul Wise. All Rights Reserved.
#include "chainedsolver.hpp"
#include "solodovsvaiter.hpp"
#include "utils.hpp"
#include "testsupport.hpp"

#include <Eigen/Dense>
#include <cstdint>
#include <random>
#include <stdexcept>

using namespace VINCP;

// Mirror of lcp_psd_test for the Solodov-Svaiter solver: random MONOTONE LCP
// with a known complementary solution (makeComplementaryPair; M = A^T A is
// PSD, positive definite almost surely, so the solution is unique).
//
// TOLERANCES ARE GLOBALIZATION-GRADE, deliberately looser than the dHan06 /
// bsHe94b mirrors (E2b finding, 2026-07-04): the hyperplane step is
// x+ = P_K(x - lambda F(y)) with lambda ~ ||r||^2 / ||F(y)||^2, so wherever
// the field is NONZERO at the solution -- every strict-complementarity LCP,
// since w* has nonzero entries -- the step collapses quadratically with the
// residual and the error decays only like O(1/sqrt(k)). Tight tolerances are
// therefore unreachable in ANY practical budget; that tail is the smoothing-
// Newton engine's job (task E3). SS's job is to get into the neighborhood
// from anywhere, which is what this test now checks (and prints).
TEST(SsLcpPsd, MonotoneKnownSolution) {
    const Index N = 10;                          // problem dimension (edit here)
    const std::uint_fast32_t seed = makeSeed(0, true);       // PRNG seed

    const int    intLo  = 1,    intHi  = 10;     // integer range for w, z entries
    const double realLo = -5.0, realHi = 10.0;   // double range for A entries

    const double magTol  = 1.0e-10;              // squared-residual tolerance
                                                 // (typically NOT reached; the
                                                 // cap binds and that is fine)
    const int    iterMax = 300000;               // cap (iterations are matrix-free)
    const double solTol  = 5.0e-2;               // globalization-grade closeness

    std::mt19937 rng(seed);

    // Complementary w and z, drawn first.
    VectorXd w, z;
    makeComplementaryPair(N, rng, intLo, intHi, w, z);

    // Random A, drawn second; M = A^T A is symmetric positive semidefinite.
    std::uniform_real_distribution<double> realDist(realLo, realHi);
    MatrixXd A(N, N);
    for (Index r = 0; r < N; ++r) {
        for (Index c = 0; c < N; ++c) {
            A(r, c) = realDist(rng);
        }
    }
    const MatrixXd M = A.transpose() * A;

    const VectorXd q  = w - M * z;
    const VectorXd x0 = VectorXd::Zero(N);

    printConstructed(z, w);

    VIResult result;
    const SolveFn solve = [&](const VectorXd& z0) {
        return solodovSvaiter(z0, M, q, projectNonnegative, magTol, iterMax, 0);
    };
    ASSERT_NO_THROW({ result = solve(x0); });
    printSolveStats("solodovSvaiter", result);   // expect converged = 0 at the
                                                 // cap: the O(1/sqrt(k)) tail
    const CheckFn close = checkCloseToKnown(z, solTol);
    EXPECT_TRUE(close(result).pass) << close(result).report;
}

// A hand-checkable 1-D LCP (M = 1, q = -2 over R_+ has x* = 2) solved from a
// deliberately INFEASIBLE start: the method projects the start onto K first.
// CONTRAST with the test above: here F(x*) = w* = 0, the hyperplane step's
// denominator vanishes along with the residual, steps stay healthy, and SS
// reaches a TIGHT tolerance quickly -- the F(x*) = 0 / F(x*) != 0 dichotomy
// is exactly the user's recalled experience with this method.
TEST(SsLcpPsd, HandLcpFromInfeasibleStart) {
    const double magTol = 1.0e-16;
    const int iterMax = 10000;
    const double solTol = 1.0e-6;

    MatrixXd M(1, 1);
    M << 1.0;
    VectorXd q(1);
    q << -2.0;
    VectorXd known(1);
    known << 2.0;
    VectorXd start(1);
    start << -7.0;                               // outside R_+

    const SolveFn solve = [&](const VectorXd& z0) {
        return solodovSvaiter(z0, M, q, projectNonnegative, magTol, iterMax, 0);
    };
    expectSolvePasses(solve, start, { checkCloseToKnown(known, solTol) });
}

// The E3a chain on the same PSD-LCP construction, at the TIGHT tolerance the
// pure-SS test above cannot reach: phase 1 (SS) supplies a cheap global warm
// start, phase 2 (bsHe94b) contracts to 1e-14. solTol is lcp_psd_test's
// original bar; the accounting check confirms both phases actually ran.
TEST(SsLcpPsd, ChainReachesTightTolerance) {
    const Index N = 10;
    const std::uint_fast32_t seed = makeSeed(0, true);

    const int    intLo  = 1,    intHi  = 10;
    const double realLo = -5.0, realHi = 10.0;

    const double magTol  = 1.0e-14;              // squared; phase-2 grade
    const int    iterMax = 100000;
    const double solTol  = 3.0e-6;               // the tight lcp_psd_test bar

    std::mt19937 rng(seed);
    VectorXd w, z;
    makeComplementaryPair(N, rng, intLo, intHi, w, z);
    std::uniform_real_distribution<double> realDist(realLo, realHi);
    MatrixXd A(N, N);
    for (Index r = 0; r < N; ++r) {
        for (Index c = 0; c < N; ++c) {
            A(r, c) = realDist(rng);
        }
    }
    const MatrixXd M = A.transpose() * A;
    const VectorXd q = w - M * z;
    const VectorXd x0 = VectorXd::Zero(N);

    printConstructed(z, w);

    VIResult result;
    ASSERT_NO_THROW({
        result = chainedSolodovHe(x0, M, q, projectNonnegative, magTol,
                                  iterMax, 0);
    });
    printSolveStats("ssHeChain", result);
    EXPECT_TRUE(result.converged);
    EXPECT_GT(result.innerIters, result.iter);   // phase 1 really ran
    const CheckFn close = checkCloseToKnown(z, solTol);
    EXPECT_TRUE(close(result).pass) << close(result).report;
}

// Parameter and input guards throw rather than proceed.
TEST(SsLcpPsd, RejectsBadParamsAndInputs) {
    MatrixXd M(2, 2);
    M << 1.0, 0.0,
         0.0, 1.0;
    const VectorXd q = VectorXd::Zero(2);
    const VectorXd x0 = VectorXd::Zero(2);
    const double magTol = 1.0e-12;
    const int iterMax = 100;

    SolodovSvaiterParams badSigma;
    badSigma.sigma = 1.5;
    EXPECT_THROW(solodovSvaiter(x0, M, q, projectNonnegative, magTol, iterMax,
                                0, badSigma),
                 std::invalid_argument);

    SolodovSvaiterParams badMu;
    badMu.mu = 0.0;
    EXPECT_THROW(solodovSvaiter(x0, M, q, projectNonnegative, magTol, iterMax,
                                0, badMu),
                 std::invalid_argument);

    EXPECT_THROW(solodovSvaiter(x0, M, q, projectNonnegative, -1.0, iterMax, 0),
                 std::invalid_argument);
    EXPECT_THROW(solodovSvaiter(x0, M, VectorXd::Zero(3), projectNonnegative,
                                magTol, iterMax, 0),
                 std::invalid_argument);
}
// Copyright Ben Paul Wise. All Rights Reserved.

// Copyright Ben Paul Wise. All Rights Reserved.
#include "dhan06.hpp"
#include "utils.hpp"
#include "testsupport.hpp"

#include <Eigen/Dense>
#include <cstdint>
#include <random>

using namespace VIMCP;

// Unit test: random MONOTONE LCP with a known complementary solution.
//
// Same construction as lcp_random_test (complementary pair via
// makeComplementaryPair; see utils.hpp), except M is built as M = A^T A with A a
// random matrix. A^T A is symmetric positive semidefinite (positive definite
// almost surely for a random square A), so the LCP is monotone and Han's
// self-adaptive projection method is guaranteed to converge -- and, M being
// positive definite, to the unique solution, which is z by construction.
TEST(LcpPsd, MonotoneKnownSolution) {
    const Index N = 10;                          // problem dimension (edit here)
    const std::uint_fast32_t seed = makeSeed(0, true);       // PRNG seed

    const int    intLo  = 1,    intHi  = 10;     // integer range for w, z entries
    const double realLo = -5.0, realHi = 10.0;   // double range for A entries

    const double magTol  = 1.0e-14;              // squared-residual tolerance (dHan06)
    const int    iterMax = 100000;               // iteration cap
    const double solTol  = 3.0e-6;               // "very close" bound on ||x - z||

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

    const SolveFn solve = [&](const VectorXd& z0) {
        return dHan06(z0, M, q, projectNonnegative, magTol, iterMax, 0);
    };
    expectSolvePasses(solve, x0, { checkCloseToKnown(z, solTol) });
}
// Copyright Ben Paul Wise. All Rights Reserved.

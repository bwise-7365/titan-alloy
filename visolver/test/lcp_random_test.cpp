// Copyright Ben Paul Wise. All Rights Reserved.
#include "dhan06.hpp"
#include "utils.hpp"

#include <Eigen/Dense>
#include <gtest/gtest.h>

#include <cstdint>
#include <random>
#include <stdexcept>

using namespace VIMCP;

// Unit test: random INDEFINITE-M LCP -- a divergence stress test.
//
// Builds the LCP  0 <= z _|_ w >= 0  with  w = M z + q  on K = R_+^N (the
// complementary pair via makeComplementaryPair; see utils.hpp). M has random
// double entries in [-5, 10] and q = w - M z, so (z, w) solves the LCP.
//
// Han's self-adaptive projection method is only guaranteed to converge for a
// MONOTONE problem (M positive semidefinite). A random M in [-5, 10] is not
// monotone, so on this fixed instance dHan06 trips its divergence guard, which
// throws std::runtime_error (residual exceeding divergenceFactor * initialMag).
// This asserts that guard fires -- replacing the plain-exe version's WILL_FAIL
// exit-code inversion with a direct expectation. (If this instance ever converged
// to the known solution instead, the throw would not fire and this would fail,
// which is exactly the surprise worth surfacing.)
TEST(LcpRandom, IndefiniteMTripsDivergenceGuard) {
    const Index N = 10;                          // problem dimension (edit here)
    const std::uint_fast32_t seed = 123456u;     // PRNG seed

    const int    intLo  = 1,    intHi  = 10;     // integer range for w, z entries
    const double realLo = -5.0, realHi = 10.0;   // double range for M entries

    const double magTol  = 1.0e-14;              // squared-residual tolerance (dHan06)
    const int    iterMax = 100000;               // iteration cap

    std::mt19937 rng(seed);

    // Complementary w and z, drawn first.
    VectorXd w, z;
    makeComplementaryPair(N, rng, intLo, intHi, w, z);

    // Random (generally indefinite) M, drawn second.
    std::uniform_real_distribution<double> realDist(realLo, realHi);
    MatrixXd M(N, N);
    for (Index r = 0; r < N; ++r) {
        for (Index c = 0; c < N; ++c) {
            M(r, c) = realDist(rng);
        }
    }

    const VectorXd q  = w - M * z;
    const VectorXd x0 = VectorXd::Zero(N);

    printConstructed(z, w);

    EXPECT_THROW((void)dHan06(x0, M, q, projectNonnegative, magTol, iterMax, 0),
                 std::runtime_error);
}
// Copyright Ben Paul Wise. All Rights Reserved.

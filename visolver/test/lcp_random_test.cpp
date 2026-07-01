// Copyright Ben Paul Wise. All Rights Reserved.
#include "dhan06.hpp"
#include "utils.hpp"

#include <Eigen/Dense>
#include <cstdint>
#include <cstdio>
#include <exception>
#include <random>

using Eigen::Index;
using Eigen::MatrixXd;
using Eigen::VectorXd;
using std::printf;

// Unit test: random LCP with a known complementary solution.
//
// Builds the LCP  0 <= z _|_ w >= 0  with  w = M z + q  on K = R_+^N (the
// complementary pair via makeComplementaryPair; see utils.hpp), then checks that
// dHan06 recovers z. Here M has random double entries in [-5, 10] and
// q = w - M z, so (z, w) solves the LCP.
//
// NOTE: Han's self-adaptive projection method is guaranteed to converge for a
// MONOTONE problem (M positive semidefinite). A random M in [-5, 10] is not
// generally monotone, so this is also a stress test of the divergence guard: a
// throw here is a legitimate "did not converge", reported as FAIL, not a crash.
int main() {
    const Index N = 10;                          // problem dimension (edit here)
    const std::uint_fast32_t seed = 123456u;     // PRNG seed

    const int    intLo  = 1,    intHi  = 10;     // integer range for w, z entries
    const double realLo = -5.0, realHi = 10.0;   // double range for M entries

    const double magTol  = 1.0e-14;              // squared-residual tolerance (dHan06)
    const int    iterMax = 100000;               // iteration cap
    const double solTol  = 1.0e-6;               // "very close" bound on ||x - z||

    std::mt19937 rng(seed);

    // Complementary w and z, drawn first.
    VectorXd w, z;
    VINCP::makeComplementaryPair(N, rng, intLo, intHi, w, z);

    // Random M, drawn second.
    std::uniform_real_distribution<double> realDist(realLo, realHi);
    MatrixXd M(N, N);
    for (Index r = 0; r < N; ++r) {
        for (Index c = 0; c < N; ++c) {
            M(r, c) = realDist(rng);
        }
    }

    const VectorXd q  = w - M * z;
    const VectorXd x0 = VectorXd::Zero(N);

    printf("N = %lld\n", static_cast<long long>(N));
    VINCP::printConstructed(z, w);

    try {
        const VINCP::VIResult result =
            VINCP::dHan06(x0, M, q, VINCP::projectNonnegative, magTol, iterMax, 0);
        return VINCP::reportAndCheck(result, M, q, z, solTol);
    } catch (const std::exception& ex) {
        printf("FAIL: dHan06 threw: %s\n", ex.what());
        return 1;
    }
}
// Copyright Ben Paul Wise. All Rights Reserved.

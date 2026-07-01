#include "dhan06.hpp"

#include <Eigen/Dense>
#include <cstdint>
#include <cstdio>
#include <exception>
#include <random>

using Eigen::Index;
using Eigen::MatrixXd;
using Eigen::VectorXd;
using std::printf;

// Unit test: random MONOTONE LCP with a known complementary solution.
//
// Same construction as lcp_random_test, except M is built as M = A^T A with A a
// random matrix. A^T A is symmetric positive semidefinite (positive definite
// almost surely for a random square A), so the LCP is monotone and Han's
// self-adaptive projection method is guaranteed to converge -- and, M being
// positive definite, to the unique solution, which is z by construction.
//
// Builds the LCP  0 <= z _|_ w >= 0  with  w = M z + q  on K = R_+^N:
//   - For even i (0-based, i.e. the 1st, 3rd, ... entries): w[i] is a random
//     integer in [1, 10] and z[i] = 0. For odd i this is flipped. So exactly
//     one of w[i], z[i] is positive and the other zero -- complementarity holds.
//   - A has random double entries in [-5, 10]; M = A^T A.
//   - q = w - M z, so that M z + q = w; hence (z, w) solves the LCP.
int main() {
    const Index N = 10;                          // problem dimension (edit here)
    const std::uint_fast32_t seed = 123456u;     // PRNG seed

    const int    intLo  = 1,    intHi  = 10;     // integer range for w, z entries
    const double realLo = -5.0, realHi = 10.0;   // double range for A entries

    const double magTol  = 1.0e-14;              // squared-residual tolerance (dHan06)
    const int    iterMax = 100000;               // iteration cap
    const double solTol  = 1.0e-6;               // "very close" bound on ||x - z||

    std::mt19937 rng(seed);
    std::uniform_int_distribution<int>     intDist(intLo, intHi);
    std::uniform_real_distribution<double> realDist(realLo, realHi);

    // Complementary w and z, drawn first.
    VectorXd w = VectorXd::Zero(N);
    VectorXd z = VectorXd::Zero(N);
    for (Index i = 0; i < N; ++i) {
        if (i % 2 == 0) {
            w(i) = static_cast<double>(intDist(rng));   // even index: w active, z = 0
        } else {
            z(i) = static_cast<double>(intDist(rng));   // odd  index: z active, w = 0
        }
    }

    // Random A, drawn second; M = A^T A is symmetric positive semidefinite.
    MatrixXd A(N, N);
    for (Index r = 0; r < N; ++r) {
        for (Index c = 0; c < N; ++c) {
            A(r, c) = realDist(rng);
        }
    }
    const MatrixXd M = A.transpose() * A;

    const VectorXd q  = w - M * z;
    const VectorXd x0 = VectorXd::Zero(N);

    try {
        const VINCP::VIResult result =
            VINCP::dHan06(x0, M, q, VINCP::projectNonnegative, magTol, iterMax, 0);

        const double solErr = (result.z - z).norm();
        printf("N              = %lld\n", static_cast<long long>(N));
        printf("iterations     = %d\n", result.iter);
        printf("residual       = %.3e (squared)\n", result.residual);
        printf("converged      = %s\n", result.converged ? "true" : "false");
        printf("solution error = %.3e (||x - z||)\n", solErr);

        if (result.converged && solErr < solTol) {
            printf("PASS (within %.1e)\n", solTol);
            return 0;
        }
        printf("FAIL (exceeds %.1e)\n", solTol);
        return 1;
    } catch (const std::exception& ex) {
        printf("FAIL: dHan06 threw: %s\n", ex.what());
        return 1;
    }
}

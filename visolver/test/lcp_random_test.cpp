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

// Unit test: random LCP with a known complementary solution.
//
// Builds the LCP  0 <= z _|_ w >= 0  with  w = M z + q  on K = R_+^N, then
// checks that dHan06 recovers z. Construction (component indices are 0-based;
// "even" means i % 2 == 0, i.e. the 1st, 3rd, 5th, ... entries):
//   - For even i: w[i] is a random integer in [1, 10] and z[i] = 0.
//     For odd  i: this is flipped (w[i] = 0, z[i] a random integer in [1, 10]).
//     Exactly one of w[i], z[i] is positive and the other is zero, so the
//     complementarity 0 <= z _|_ w >= 0 holds by construction.
//   - M has random double entries in [-5, 10].
//   - q = w - M z, so that M z + q = w; hence (z, w) solves the LCP.
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

    // Random M, drawn second.
    MatrixXd M(N, N);
    for (Index r = 0; r < N; ++r) {
        for (Index c = 0; c < N; ++c) {
            M(r, c) = realDist(rng);
        }
    }

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

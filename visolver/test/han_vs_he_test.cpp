// Copyright Ben Paul Wise. All Rights Reserved.
#include "dhan06.hpp"
#include "bshe94b.hpp"
#include "josephynewton.hpp"
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

// Side-by-side comparison of the two inner LVI solvers -- Han (dHan06, self-
// adaptive projection) and He (bsHe94b, fixed-metric projection-contraction) --
// on identical problems:
//   (1) a monotone linear VI  (LCP with M = A^T A), solved directly by each; and
//   (2) a monotone nonlinear "PSD" VI (cubic gradient map) driven through the
//       SAME Josephy-Newton outer loop with each solver plugged in as the inner
//       solver via the InnerSolver seam.
// Both problems are constructed with a known solution; the test passes iff both
// solvers recover it on both problems. Iteration counts are reported so the
// relative performance is visible.

// Report one run against a known solution; return 0 on success, 1 on failure.
// 'inner' is the total inner-solver iterations (0 for a direct/linear solve).
static int report(const char* tag, const VINCP::VIResult& r,
                  const VectorXd& known, double solTol) {
    const double solErr = (r.z - known).norm();
    printf("  %-8s iters=%6d  inner=%8d  residual=%.3e  converged=%-5s  solErr=%.3e\n",
           tag, r.iter, r.innerIters, r.residual, r.converged ? "true" : "false", solErr);
    return (r.converged && solErr < solTol) ? 0 : 1;
}

int main() {
    VINCP::ScopedUtcTimer timer("han_vs_he_test");
    const std::uint_fast32_t seed = 424242u;  // fixed for a reproducible comparison

    // Shared draw ranges / tolerances.
    const int    intLo = 1,    intHi = 10;    // complementary-pair magnitudes
    const double aLo   = -1.0, aHi   = 1.0;   // forms-matrix range
    const double magTol   = 1.0e-14;          // inner squared-residual tolerance
    const int    iterMax  = 100000;           // inner iteration cap
    const double solTol   = 1.0e-6;           // acceptance bound on ||x - x*||

    std::mt19937 rng(seed);
    std::uniform_real_distribution<double> aDist(aLo, aHi);

    int fail = 0;

    // ---- (1) Linear VI: monotone LCP  0 <= z _|_ (M z + q) >= 0, M = A^T A ----
    {
        const Index N = 8;                    // dimension (edit here)
        VectorXd w, zSol;
        VINCP::makeComplementaryPair(N, rng, intLo, intHi, w, zSol);
        MatrixXd A(N, N);
        for (Index r = 0; r < N; ++r) {
            for (Index c = 0; c < N; ++c) {
                A(r, c) = aDist(rng);
            }
        }
        const MatrixXd M = A.transpose() * A;
        const VectorXd q  = w - M * zSol;
        const VectorXd x0 = VectorXd::Zero(N);

        printf("=== (1) Linear VI: monotone LCP, M = A^T A, N = %td ===\n",
               static_cast<long long>(N));
        const VINCP::VIResult rHan =
            VINCP::dHan06(x0, M, q, VINCP::projectNonnegative, magTol, iterMax, 0);
        const VINCP::VIResult rHe =
            VINCP::bsHe94b(x0, M, q, VINCP::projectNonnegative, magTol, iterMax, 0);
        fail += report("dHan06",  rHan, zSol, solTol);
        fail += report("bsHe94b", rHe,  zSol, solTol);
    }

    // ---- (2) PSD nonlinear VI: cubic gradient map through the JN outer loop ----
    {
        const Index n = 4;                    // free block dimension (edit here)
        const Index m = 4;                    // non-negative block dimension
        const Index d = n + m;
        const int    xLo = -4, xHi = 4;       // free-block solution range
        const double outerTol   = 1.0e-10;    // squared natural-residual stop
        const int    outerIterMax = 200;

        // Known solution z* = (x*, y*) with complementary (y*, w*).
        std::uniform_int_distribution<int> xDist(xLo, xHi);
        VectorXd xStar(n);
        for (Index i = 0; i < n; ++i) {
            xStar(i) = static_cast<double>(xDist(rng));
        }
        VectorXd wStar, yStar;
        VINCP::makeComplementaryPair(m, rng, intLo, intHi, wStar, yStar);
        VectorXd zStar(d);
        zStar << xStar, yStar;

        // Monotone cubic F(z) = A^T(g(Az)) + k with F(z*) = [0; w*].
        VectorXd target(d);
        target << VectorXd::Zero(n), wStar;
        const VINCP::CubicProblem prob =
            VINCP::makeCubicProblem(d, d, rng, zStar, target, /*forcePSD=*/true, aLo, aHi);

        const VINCP::VIModel model = VINCP::makeVIModel(n, m, prob.F);

        const VectorXd z0 = VectorXd::Zero(d);
        VINCP::JosephyNewtonParams params;
        params.outerTol     = outerTol;
        params.outerIterMax = outerIterMax;

        // Same outer loop, same problem -- only the inner solver differs.
        const VINCP::InnerSolver innerHan = VINCP::makeDHan06Solver(magTol, iterMax, 0);
        const VINCP::InnerSolver innerHe  = VINCP::makeBsHe94bSolver(magTol, iterMax, 0);

        printf("\n=== (2) PSD nonlinear VI: cubic via JN outer loop, n = %td free, m = %td nonneg ===\n",
               static_cast<long long>(n), static_cast<long long>(m));
        printf("    (iters = OUTER Josephy-Newton iterations)\n");
        try {
            const VINCP::VIResult rHan = VINCP::solveVI(model, z0, innerHan, params);
            const VINCP::VIResult rHe  = VINCP::solveVI(model, z0, innerHe,  params);
            fail += report("dHan06",  rHan, zStar, solTol);
            fail += report("bsHe94b", rHe,  zStar, solTol);
        } catch (const std::exception& ex) {
            printf("  FAIL: solveVI threw: %s\n", ex.what());
            ++fail;
        }
    }

    printf("\n%s\n", (fail == 0) ? "PASS (both solvers solved both problems)"
                                 : "FAIL (a solver missed a problem)");
    return (fail == 0) ? 0 : 1;
}
// Copyright Ben Paul Wise. All Rights Reserved.

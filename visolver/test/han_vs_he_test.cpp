// Copyright Ben Paul Wise. All Rights Reserved.
#include "dhan06.hpp"
#include "bshe94b.hpp"
#include "josephynewton.hpp"
#include "utils.hpp"
#include "testsupport.hpp"

#include <Eigen/Dense>
#include <gtest/gtest.h>

#include <cstdint>
#include <random>

using namespace VINCP;

// Side-by-side comparison of the two inner LVI solvers -- Han (dHan06, self-
// adaptive projection) and He (bsHe94b, fixed-metric projection-contraction) --
// on identical problems:
//   (1) a monotone linear VI  (LCP with M = A^T A), solved directly by each; and
//   (2) a monotone nonlinear "PSD" VI (cubic gradient map) driven through the
//       SAME Josephy-Newton outer loop with each solver plugged in as the inner
//       solver via the InnerSolver seam.
// Both problems are constructed with a known solution; the test requires BOTH
// solvers to recover it on BOTH problems. The two problems share ONE RNG stream in
// sequence (problem 1's draws, then problem 2's), so they live in a single suite:
// splitting them into separate cases with their own seeds would draw a different --
// and empirically harder -- second instance. Order is preserved from the original.

namespace {
    const std::uint_fast32_t kSeed   = 424242u;   // reproducible instances
    const int    kIntLo   = 1,    kIntHi = 10;    // complementary-pair magnitudes
    const double kALo     = -1.0, kAHi   = 1.0;   // forms-matrix range
    const double kMagTol  = 1.0e-14;              // inner squared-residual tolerance
    const int    kIterMax = 100000;               // inner iteration cap
    const double kSolTol  = 1.0e-6;               // acceptance bound on ||x - x*||
} // namespace

TEST(HanVsHe, BothSolversRecoverBothProblems) {
    std::mt19937 rng(kSeed);

    // ---- (1) Linear VI: monotone LCP  0 <= z _|_ (M z + q) >= 0, M = A^T A ----
    {
        std::uniform_real_distribution<double> aDist(kALo, kAHi);

        const Index N = 8;                    // dimension (edit here)
        VectorXd w, zSol;
        makeComplementaryPair(N, rng, kIntLo, kIntHi, w, zSol);
        MatrixXd A(N, N);
        for (Index r = 0; r < N; ++r) {
            for (Index c = 0; c < N; ++c) {
                A(r, c) = aDist(rng);
            }
        }
        const MatrixXd M  = A.transpose() * A;
        const VectorXd q  = w - M * zSol;
        const VectorXd x0 = VectorXd::Zero(N);

        const SolveFn han = [&](const VectorXd& z){ return dHan06(z, M, q, projectNonnegative, kMagTol, kIterMax, 0); };
        const SolveFn he  = [&](const VectorXd& z){ return bsHe94b(z, M, q, projectNonnegative, kMagTol, kIterMax, 0); };
        { SCOPED_TRACE("dHan06 (LCP)");  expectSolvePasses(han, x0, { checkCloseToKnown(zSol, kSolTol) }); }
        { SCOPED_TRACE("bsHe94b (LCP)"); expectSolvePasses(he,  x0, { checkCloseToKnown(zSol, kSolTol) }); }
    }

    // ---- (2) PSD nonlinear VI: cubic gradient map through the JN outer loop ----
    // Draws continue from the SAME rng, after problem (1) above -- do not reseed.
    {
    const Index n = 4;                    // free block dimension (edit here)
    const Index m = 4;                    // non-negative block dimension
    const Index d = n + m;
    const int    xLo = -4, xHi = 4;       // free-block solution range
    const double outerTol     = 1.0e-10;  // squared natural-residual stop
    const int    outerIterMax = 200;

    // Known solution z* = (x*, y*) with complementary (y*, w*).
    std::uniform_int_distribution<int> xDist(xLo, xHi);
    VectorXd xStar(n);
    for (Index i = 0; i < n; ++i) {
        xStar(i) = static_cast<double>(xDist(rng));
    }
    VectorXd wStar, yStar;
    makeComplementaryPair(m, rng, kIntLo, kIntHi, wStar, yStar);
    VectorXd zStar(d);
    zStar << xStar, yStar;

    // Monotone cubic F(z) = A^T(g(Az)) + k with F(z*) = [0; w*].
    VectorXd target(d);
    target << VectorXd::Zero(n), wStar;
    const CubicProblem prob =
        makeCubicProblem(d, d, rng, zStar, target, /*forcePSD=*/true, kALo, kAHi);

    const VIModel model = makeVIModel(n, m, prob.F);

    const VectorXd z0 = VectorXd::Zero(d);
    JosephyNewtonParams params;
    params.outerTol     = outerTol;
    params.outerIterMax = outerIterMax;

    // Same outer loop, same problem -- only the inner solver differs.
    const InnerSolver innerHan = makeDHan06Solver(kMagTol, kIterMax, 0);
    const InnerSolver innerHe  = makeBsHe94bSolver(kMagTol, kIterMax, 0);

    const SolveFn han = [&](const VectorXd& start){ return solveVI(model, start, innerHan, params); };
    const SolveFn he  = [&](const VectorXd& start){ return solveVI(model, start, innerHe,  params); };
    { SCOPED_TRACE("dHan06 (cubic VI)");  expectSolvePasses(han, z0, { checkCloseToKnown(zStar, kSolTol) }); }
    { SCOPED_TRACE("bsHe94b (cubic VI)"); expectSolvePasses(he,  z0, { checkCloseToKnown(zStar, kSolTol) }); }
    }
}
// Copyright Ben Paul Wise. All Rights Reserved.

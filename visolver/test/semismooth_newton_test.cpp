// Copyright Ben Paul Wise. All Rights Reserved.
#include "semismoothnewton.hpp"
#include "mehrotraipm.hpp"
#include "utils.hpp"
#include "testsupport.hpp"

#include <Eigen/Dense>
#include <cstdint>
#include <random>
#include <stdexcept>
#include <string>

using namespace VIMCP;

// GoogleTest suite for the direct mixed-NCP semismooth Newton solver (MC2).
// The claims under test: it solves the NONLINEAR mixed problem directly (no
// Josephy-Newton nesting), it is exact at the FB kink (degenerate solutions),
// the NCP-function seam swaps cleanly, and every convergent solve stays
// within a Newton-grade iteration budget.

namespace {

    constexpr int kIterBudget = 60;    // Newton-grade budget on convergent solves
    constexpr int kIterMax    = 200;

    const std::uint_fast32_t kSeed = 424242u;   // reproducible instances

    CheckFn checkConverged() {
        return [](const VIResult& r) {
            return CheckResult{ r.converged,
                                r.converged ? string("converged flag set")
                                            : string("converged flag NOT set") };
        };
    }

    CheckFn checkIterAtMost(int budget) {
        return [budget](const VIResult& r) {
            const bool passP = r.iter <= budget;
            return CheckResult{ passP,
                                "iterations " + std::to_string(r.iter)
                                + (passP ? " <= budget " : " > budget ")
                                + std::to_string(budget) };
        };
    }

    void expectChecksPass(const VIResult& result, const VectorXd& known,
                          double solTol) {
        for (const CheckFn& check : { checkConverged(), checkIterAtMost(kIterBudget),
                                      checkCloseToKnown(known, solTol) }) {
            const CheckResult cr = check(result);
            EXPECT_TRUE(cr.pass) << cr.report;
        }
    }

    // Random PSD (almost surely PD) matrix M = A^T A with A numRows x n.
    MatrixXd makeGramMatrix(Index numRows, Index n, std::mt19937& rng,
                            double realLo, double realHi) {
        std::uniform_real_distribution<double> realDist(realLo, realHi);
        MatrixXd A(numRows, n);
        for (Index r = 0; r < numRows; ++r) {
            for (Index c = 0; c < n; ++c) {
                A(r, c) = realDist(rng);
            }
        }
        return A.transpose() * A;
    }

} // namespace

// The nonlinear headline case: a monotone cubic mixed VI (the han_vs_he
// problem-2 construction) with a known solution, solved DIRECTLY -- one
// semismooth Newton loop on the nonlinear system, FD Jacobian, no outer/inner
// nesting. Default (penalized-FB) NCP function.
TEST(SemismoothNewton, CubicMixedViKnownSolution) {
    const Index n = 4;                    // free block dimension
    const Index m = 4;                    // non-negative block dimension
    const Index d = n + m;
    const int    intLo = 1,    intHi = 10;
    const double aLo   = -1.0, aHi   = 1.0;
    const int    xLo   = -4,   xHi   = 4;

    const double magTol = 1.0e-14;        // squared natural residual
    const double solTol = 1.0e-6;

    std::mt19937 rng(kSeed);

    std::uniform_int_distribution<int> xDist(xLo, xHi);
    VectorXd xStar(n);
    for (Index i = 0; i < n; ++i) {
        xStar(i) = static_cast<double>(xDist(rng));
    }
    VectorXd wStar, yStar;
    makeComplementaryPair(m, rng, intLo, intHi, wStar, yStar);
    VectorXd zStar(d);
    zStar << xStar, yStar;

    VectorXd target(d);
    target << VectorXd::Zero(n), wStar;
    const CubicProblem prob =
        makeCubicProblem(d, d, rng, zStar, target, /*forcePSD=*/true, aLo, aHi);
    const VIModel model = makeVIModel(n, m, prob.F);

    SemismoothNewtonParams params;
    params.magTol = magTol;
    params.iterMax = kIterMax;

    VIResult result;
    ASSERT_NO_THROW({
        result = semismoothNewtonSolve(model, VectorXd::Zero(d), params);
    });
    printSolveStats("semismoothNewton", result);
    expectChecksPass(result, zStar, solTol);
}

// The seam swap: the same cubic instance under the PLAIN Fischer-Burmeister
// pair. Exercises that NcpFunctionPair is genuinely interchangeable.
TEST(SemismoothNewton, PlainFbPairAlsoSolves) {
    const Index n = 4;
    const Index m = 4;
    const Index d = n + m;
    const int    intLo = 1,    intHi = 10;
    const double aLo   = -1.0, aHi   = 1.0;
    const int    xLo   = -4,   xHi   = 4;

    const double magTol = 1.0e-14;
    const double solTol = 1.0e-6;

    std::mt19937 rng(kSeed);

    std::uniform_int_distribution<int> xDist(xLo, xHi);
    VectorXd xStar(n);
    for (Index i = 0; i < n; ++i) {
        xStar(i) = static_cast<double>(xDist(rng));
    }
    VectorXd wStar, yStar;
    makeComplementaryPair(m, rng, intLo, intHi, wStar, yStar);
    VectorXd zStar(d);
    zStar << xStar, yStar;

    VectorXd target(d);
    target << VectorXd::Zero(n), wStar;
    const CubicProblem prob =
        makeCubicProblem(d, d, rng, zStar, target, /*forcePSD=*/true, aLo, aHi);
    const VIModel model = makeVIModel(n, m, prob.F);

    SemismoothNewtonParams params;
    params.magTol = magTol;
    params.iterMax = kIterMax;
    params.ncp = fischerBurmeisterPair();

    VIResult result;
    ASSERT_NO_THROW({
        result = semismoothNewtonSolve(model, VectorXd::Zero(d), params);
    });
    printSolveStats("semismoothNewton(FB)", result);
    expectChecksPass(result, zStar, solTol);
}

// Degenerate affine case (strict complementarity destroyed at every third
// index, M positive definite so the solution stays unique and checkable):
// the FB-kink regime the smoothing solver needs a continuation to survive.
// Solved with an ANALYTIC Jacobian (constant M) and cross-checked against
// the interior-point engine on the identical data.
TEST(SemismoothNewton, DegenerateAffineMatchesIpm) {
    const Index m = 12;
    const Index degenerateStride = 3;
    const int    intLo  = 1,    intHi  = 10;
    const double realLo = -5.0, realHi = 10.0;

    const double magTol = 1.0e-14;
    const double solTol = 3.0e-6;
    const double crossTol = 1.0e-6;       // engine-agreement bar

    std::mt19937 rng(kSeed);

    VectorXd w, zKnown;
    makeComplementaryPair(m, rng, intLo, intHi, w, zKnown);
    for (Index i = 0; i < m; i += degenerateStride) {
        w(i) = 0.0;
        zKnown(i) = 0.0;
    }
    const MatrixXd M = makeGramMatrix(m, m, rng, realLo, realHi);
    const VectorXd q = w - M * zKnown;

    printConstructed(zKnown, w);

    const VIModel model =
        makeVIModel(0, m, [&](const VectorXd& z) -> VectorXd { return M * z + q; });

    SemismoothNewtonParams params;
    params.magTol = magTol;
    params.iterMax = kIterMax;
    params.jacobian = [&](const VectorXd&) -> MatrixXd { return M; };

    VIResult ssn;
    ASSERT_NO_THROW({
        ssn = semismoothNewtonSolve(model, VectorXd::Zero(m), params);
    });
    printSolveStats("semismoothNewton", ssn);
    expectChecksPass(ssn, zKnown, solTol);

    VIResult ipm;
    ASSERT_NO_THROW({
        ipm = mehrotraIpm(M, q, 0, magTol, kIterMax, 0);
    });
    printSolveStats("mehrotraIpm", ipm);
    EXPECT_TRUE(ipm.converged);
    EXPECT_LT((ssn.z - ipm.z).norm(), crossTol);
}

// Mixed affine hand case: the projection QP (project (1,1) onto u1+u2 <= 1),
// KKT z* = (1/2, 1/2, 1/2) -- free block exercised alongside the orthant
// block, analytic Jacobian.
TEST(SemismoothNewton, MixedHandQpKnownSolution) {
    const Index numU = 2;
    const Index numLambda = 1;
    const Index total = numU + numLambda;
    const double magTol = 1.0e-14;
    const double solTol = 1.0e-6;

    MatrixXd M = MatrixXd::Zero(total, total);
    M(0, 0) = 1.0;  M(1, 1) = 1.0;
    M(0, 2) = 1.0;  M(1, 2) = 1.0;
    M(2, 0) = -1.0; M(2, 1) = -1.0;

    VectorXd q(total);
    q << -1.0, -1.0, 1.0;

    VectorXd known(total);
    known << 0.5, 0.5, 0.5;

    const VIModel model =
        makeVIModel(numU, numLambda,
                    [&](const VectorXd& z) -> VectorXd { return M * z + q; });

    SemismoothNewtonParams params;
    params.magTol = magTol;
    params.iterMax = kIterMax;
    params.jacobian = [&](const VectorXd&) -> MatrixXd { return M; };

    VIResult result;
    ASSERT_NO_THROW({
        result = semismoothNewtonSolve(model, VectorXd::Zero(total), params);
    });
    printSolveStats("semismoothNewton", result);
    expectChecksPass(result, known, solTol);
}

// Parameter and input guards throw rather than proceed.
TEST(SemismoothNewton, RejectsBadParamsAndInputs) {
    const Index m = 2;
    MatrixXd M(m, m);
    M << 1.0, 0.0,
         0.0, 1.0;
    const VectorXd q = VectorXd::Zero(m);
    const VIModel model =
        makeVIModel(0, m, [&](const VectorXd& z) -> VectorXd { return M * z + q; });
    const VectorXd z0 = VectorXd::Zero(m);

    // Penalization weight outside (0, 1).
    EXPECT_THROW(penalizedFischerBurmeisterPair(0.0), std::invalid_argument);
    EXPECT_THROW(penalizedFischerBurmeisterPair(1.0), std::invalid_argument);

    SemismoothNewtonParams badP;
    badP.pExp = 2.0;                       // theory needs p > 2, strictly
    EXPECT_THROW(semismoothNewtonSolve(model, z0, badP), std::invalid_argument);

    SemismoothNewtonParams badRho;
    badRho.rho = 0.0;
    EXPECT_THROW(semismoothNewtonSolve(model, z0, badRho), std::invalid_argument);

    SemismoothNewtonParams badMemory;
    badMemory.nonmonotoneMemory = 0;
    EXPECT_THROW(semismoothNewtonSolve(model, z0, badMemory), std::invalid_argument);

    SemismoothNewtonParams badNcp;
    badNcp.ncp = NcpFunctionPair{};        // both members unset
    EXPECT_THROW(semismoothNewtonSolve(model, z0, badNcp), std::invalid_argument);

    EXPECT_THROW(semismoothNewtonSolve(model, VectorXd::Zero(m + 1)),
                 std::invalid_argument);   // start of the wrong length
}
// Copyright Ben Paul Wise. All Rights Reserved.

// Copyright Ben Paul Wise. All Rights Reserved.
//
// E2b: Solodov-Svaiter on the LVI-over-ellipsoid problem (the user's recalled
// trouble case for this method). Same setup, closed-form optimum, and printout
// as lvi_ellipsoid_test:
//
//     minimize   sum_i c_i x_i
//     subject to sum_i (x_i / a_i)^2 <= 1        (x in the ellipsoid K = E(a))
//
// x in R^5, c_i ~ U[1, 10], radii a_i ~ U[10, 100]; the optimum
// x*_i = -c_i a_i^2 / ||c .* a||_2 lies ON the curved boundary. As an LVI this
// is M = 0, q = c -- monotone but not strictly, so it exercises exactly the
// flat-field / curved-boundary regime where projection methods crawl if they
// are going to. Two tests:
//   1. solodovSvaiter DIRECTLY on the affine VI over the ellipsoid projector;
//   2. all three inner solvers through the Josephy-Newton outer loop, with
//      per-solver stats printed for side-by-side comparison (han-vs-he style).
#include "ellipsoidprojector.hpp"
#include "josephynewton.hpp"
#include "solodovsvaiter.hpp"
#include "utils.hpp"
#include "testsupport.hpp"

#include <Eigen/Dense>
#include <gtest/gtest.h>

#include <cstdint>
#include <cstdio>
#include <random>
#include <string>

using namespace VINCP;

namespace {

    // One shared instance so both tests (and the older lvi_ellipsoid_test,
    // which uses the same seed) see the same c, a, and x*.
    struct EllipsoidCase {
        VectorXd c, a, xStar;
    };

    EllipsoidCase makeCase(int dim) {
        const std::uint64_t seed = makeSeed(20260703, true);
        std::mt19937_64 rng(seed);
        std::uniform_real_distribution<double> cDist(1.0, 10.0);
        std::uniform_real_distribution<double> aDist(10.0, 100.0);

        EllipsoidCase problem;
        problem.c.resize(dim);
        problem.a.resize(dim);
        for (int i = 0; i < dim; ++i) {
            problem.c(i) = cDist(rng);
            problem.a(i) = aDist(rng);
        }
        const double caNorm = problem.c.cwiseProduct(problem.a).norm();
        problem.xStar =
            -(problem.c.array() * problem.a.array().square() / caNorm).matrix();

        printComment(false,
                     "SS ellipsoid test: min c^T x  s.t.  sum_i (x_i/a_i)^2 <= 1");
        printVector("c        ", problem.c);
        printVector("a (radii)", problem.a);
        printVector("x* exact ", problem.xStar);
        return problem;
    }

    CheckFn makeFeasCheck(const VectorXd& a, double feasTol) {
        return [&a, feasTol](const VIResult& r) -> CheckResult {
            const double en = ellipsoidNorm(r.z, a);
            char buf[96];
            std::snprintf(buf, sizeof buf,
                          "ellipsoidNorm(x) = %.12f (feasible if <= 1)", en);
            return CheckResult{ en <= 1.0 + feasTol, std::string(buf) };
        };
    }

    CheckFn makeAccCheck(const VectorXd& xStar, double relTol) {
        return [&xStar, relTol](const VIResult& r) -> CheckResult {
            const double err = (r.z - xStar).norm();
            const double relErr = err / xStar.norm();
            char buf[128];
            std::snprintf(buf, sizeof buf,
                          "||x - x*|| = %.3e, relative = %.3e (tol %.1e)",
                          err, relErr, relTol);
            return CheckResult{ relErr <= relTol, std::string(buf) };
        };
    }

} // namespace

// Solodov-Svaiter DIRECTLY on the affine VI (M = 0, q = c) over E(a): the
// inner problem of the ellipsoid test, with no outer loop to help.
//
// F = c is NONZERO everywhere, so this sits squarely in the O(1/sqrt(k))
// tail regime (see ss_lcp_test's header comment): the accuracy target is
// GLOBALIZATION-GRADE (1e-2 relative), not the 1e-6 the metric-based solvers
// hit. Feasibility stays tight -- every SS iterate is a projection onto K.
TEST(SsEllipsoid, DirectLviOverEllipsoid) {
    const int    dim     = 5;
    const double feasTol = 1.0e-6;
    const double relTol  = 1.0e-2;    // globalization-grade (see above)
    const double magTol  = 1.0e-12;   // squared; typically the cap binds first
    const int    iterMax = 500000;    // iterations are matrix-free and cheap

    const EllipsoidCase problem = makeCase(dim);
    const Projector K = makeEllipsoidProjector(problem.a);
    const MatrixXd M = MatrixXd::Zero(dim, dim);
    const VectorXd z0 = VectorXd::Zero(dim);   // origin: strictly inside E(a)

    VIResult result;
    const SolveFn solve = [&](const VectorXd& start) {
        return solodovSvaiter(start, M, problem.c, K, magTol, iterMax, 0);
    };
    ASSERT_NO_THROW({ result = solve(z0); });
    printSolveStats("solodovSvaiter (direct)", result);

    const CheckFn feasCheck = makeFeasCheck(problem.a, feasTol);
    const CheckFn accCheck = makeAccCheck(problem.xStar, relTol);
    EXPECT_TRUE(feasCheck(result).pass) << feasCheck(result).report;
    EXPECT_TRUE(accCheck(result).pass) << accCheck(result).report;
}

// All three inner solvers through the Josephy-Newton loop on the same
// problem, printing each solver's stats for side-by-side comparison. Each
// method carries its own accuracy bar: 1e-6 for the metric-based solvers,
// globalization-grade 1e-2 for SS (whose inner stalls also keep the outer
// loop from ever meeting outerTol, so outerIterMax is small to bound the
// grind -- the affine problem needs only 1-2 outer steps anyway).
TEST(SsEllipsoid, ThreeWayThroughJosephyNewton) {
    const int    dim          = 5;
    const double feasTol      = 1.0e-6;
    const double innerMagTol  = 1.0e-14;
    const int    innerIterMax = 100000;
    const int    outerIterCap = 5;

    const EllipsoidCase problem = makeCase(dim);
    const Projector K = makeEllipsoidProjector(problem.a);
    const VectorXd z0 = VectorXd::Zero(dim);

    VIModel model;
    model.n = dim;
    model.m = 0;
    const VectorXd c = problem.c;
    model.H = [c](const VectorXd&, const VectorXd&) -> VectorXd { return c; };
    model.G = [](const VectorXd&, const VectorXd&) -> VectorXd {
        return VectorXd();
    };

    struct Method { const char* name; InnerSolver inner; double relTol; };
    const Method methods[] = {
        { "dHan06        ",
          makeDHan06Solver(innerMagTol, innerIterMax, 0), 1.0e-6 },
        { "bsHe94b       ",
          makeBsHe94bSolver(innerMagTol, innerIterMax, 0), 1.0e-6 },
        { "solodovSvaiter",
          makeSolodovSvaiterSolver(innerMagTol, innerIterMax, 0), 1.0e-2 },
        // The E3a chain must hit the TIGHT bar its own phase 1 cannot.
        { "ssHeChain     ",
          makeChainedSolver(innerMagTol, innerIterMax, 0), 1.0e-6 },
    };

    const CheckFn feasCheck = makeFeasCheck(problem.a, feasTol);
    JosephyNewtonParams jn;
    jn.outerIterMax = outerIterCap;

    for (const Method& m : methods) {
        SCOPED_TRACE(m.name);
        VIResult result;
        const SolveFn solve = [&](const VectorXd& start) {
            return solveVI(model, start, m.inner, jn, {}, K);
        };
        ASSERT_NO_THROW({ result = solve(z0); });
        printSolveStats(m.name, result);
        const CheckFn accCheck = makeAccCheck(problem.xStar, m.relTol);
        EXPECT_TRUE(feasCheck(result).pass) << feasCheck(result).report;
        EXPECT_TRUE(accCheck(result).pass) << accCheck(result).report;
    }
}
// Copyright Ben Paul Wise. All Rights Reserved.

// Copyright Ben Paul Wise. All Rights Reserved.
//
// LVI test: minimize a linear objective over a solid ellipsoid, driven by the
// Josephy-Newton outer loop around BOTH inner solvers (dHan06 and bsHe94b).
//
//     minimize   sum_i c_i x_i
//     subject to sum_i (x_i / a_i)^2 <= 1        (x in the ellipsoid K = E(a))
//
// with x in R^5, c_i ~ U[1, 10], and radii a_i ~ U[10, 100]. As a variational
// inequality this is F(x) = grad(c^T x) = c (a constant field) over K; the inner
// Josephy-Newton linearization is therefore M = J(x) = 0, q = c, solved over K via
// the ellipsoid projector. This problem has a closed-form optimum,
//     x*_i = -c_i a_i^2 / ||c .* a||_2,
// which lies on the boundary (ellipsoidNorm(x*) = 1). The pass criterion is both
// FEASIBILITY (ellipsoidNorm(x) <= 1 within tolerance) AND ACCURACY (x matches x*
// in relative 2-norm) for both inner solvers.
#include "ellipsoidprojector.hpp"
#include "josephynewton.hpp"
#include "utils.hpp"
#include "testsupport.hpp"

#include <Eigen/Dense>
#include <gtest/gtest.h>

#include <cstdint>
#include <cstdio>
#include <random>
#include <string>

using namespace VINCP;

TEST(LviEllipsoid, LinearObjectiveOverEllipsoid) {
    const bool   latex        = false;    // ASCII output for the test log
    const int    dim          = 5;        // problem dimension
    const double feasTol      = 1.0e-6;   // ellipsoidNorm(x) <= 1 + feasTol is feasible
    const double relTol       = 1.0e-6;   // max ||x - x*|| / ||x*|| for an accurate match
    const double innerMagTol  = 1.0e-14;  // inner LVI squared-residual tolerance
    const int    innerIterMax = 100000;   // inner iteration cap (cheap 5x5 projections)

    // Reproducible random instance: fixed non-zero seed => same c, a every run.
    const std::uint64_t seed = makeSeed(20260703, true);
    std::mt19937_64 rng(seed);
    std::uniform_real_distribution<double> cDist(1.0, 10.0);
    std::uniform_real_distribution<double> aDist(10.0, 100.0);

    VectorXd c(dim), a(dim);
    for (int i = 0; i < dim; ++i) {
        c(i) = cDist(rng);
        a(i) = aDist(rng);
    }

    printComment(latex, "LVI test: min c^T x  s.t.  sum_i (x_i/a_i)^2 <= 1  (ellipsoid K)");
    printVector("c        ", c);
    printVector("a (radii)", a);

    // Closed-form optimum x*_i = -c_i a_i^2 / ||c .* a||_2 (on the boundary), used to
    // cross-check each solver's result below.
    const double caNorm = c.cwiseProduct(a).norm();
    const VectorXd xStar = -(c.array() * a.array().square() / caNorm).matrix();
    printVector("x* exact ", xStar);

    // VI model: F(z) = c (constant). All components free; the ellipsoid, not
    // non-negativity, is the feasible set, so it is supplied as the projector below.
    VIModel model;
    model.n = dim;
    model.m = 0;
    model.H = [c](const VectorXd&, const VectorXd&) -> VectorXd { return c; };
    model.G = [](const VectorXd&, const VectorXd&) -> VectorXd { return VectorXd(); };

    const Projector K = makeEllipsoidProjector(a);
    const VectorXd z0 = VectorXd::Zero(dim);   // origin: strictly inside E(a)

    struct Method { const char* name; InnerSolver inner; };
    const Method methods[] = {
        { "dHan06",  makeDHan06Solver(innerMagTol, innerIterMax, 0) },
        { "bsHe94b", makeBsHe94bSolver(innerMagTol, innerIterMax, 0) },
    };

    // Checks: feasibility (on/inside E(a)) and accuracy vs the closed-form optimum x*.
    const CheckFn feasCheck = [&](const VIResult& r) -> CheckResult {
        const double en = ellipsoidNorm(r.z, a);
        char buf[96];
        std::snprintf(buf, sizeof buf, "ellipsoidNorm(x) = %.12f (feasible if <= 1)", en);
        return CheckResult{ en <= 1.0 + feasTol, std::string(buf) };
    };
    const CheckFn accCheck = [&](const VIResult& r) -> CheckResult {
        const double err    = (r.z - xStar).norm();
        const double relErr = err / xStar.norm();
        char buf[128];
        std::snprintf(buf, sizeof buf, "||x - x*|| = %.3e, relative = %.3e (tol %.1e)", err, relErr, relTol);
        return CheckResult{ relErr <= relTol, std::string(buf) };
    };
    const JosephyNewtonParams jn;   // defaults

    // All-of: both inner solvers must reach the optimum.
    for (const Method& m : methods) {
        SCOPED_TRACE(m.name);
        const SolveFn solve = [&](const VectorXd& start) {
            return solveVI(model, start, m.inner, jn, {}, K);
        };
        expectSolvePasses(solve, z0, { feasCheck, accCheck });
    }
}
// Copyright Ben Paul Wise. All Rights Reserved.

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
// in relative 2-norm) for both inner solvers. The output mirrors the SAOE test
// (run time, iterations, squared residual).
#include "ellipsoidprojector.hpp"
#include "josephynewton.hpp"
#include "utils.hpp"

#include <Eigen/Dense>
#include <cstdint>
#include <cstdio>
#include <exception>
#include <random>

using Eigen::VectorXd;
using std::printf;

int main() {
    VINCP::ScopedUtcTimer timer("lvi_ellipsoid_test");
    const bool   latex        = false;    // ASCII output for the test log
    const int    dim          = 5;        // problem dimension
    const double feasTol      = 1.0e-6;   // ellipsoidNorm(x) <= 1 + feasTol is feasible
    const double relTol       = 1.0e-6;   // max ||x - x*|| / ||x*|| for an accurate match
    const double innerMagTol  = 1.0e-14;  // inner LVI squared-residual tolerance
    const int    innerIterMax = 100000;   // inner iteration cap (cheap 5x5 projections)

    // Reproducible random instance: fixed non-zero seed => same c, a every run.
    const std::uint64_t seed = VINCP::makeSeed(20260703, true);
    std::mt19937_64 rng(seed);
    std::uniform_real_distribution<double> cDist(1.0, 10.0);
    std::uniform_real_distribution<double> aDist(10.0, 100.0);

    VectorXd c(dim), a(dim);
    for (int i = 0; i < dim; ++i) {
        c(i) = cDist(rng);
        a(i) = aDist(rng);
    }

    VINCP::printComment(latex, "LVI test: min c^T x  s.t.  sum_i (x_i/a_i)^2 <= 1  (ellipsoid K)");
    VINCP::printVector("c        ", c);
    VINCP::printVector("a (radii)", a);

    // Closed-form optimum x*_i = -c_i a_i^2 / ||c .* a||_2 (on the boundary), used to
    // cross-check each solver's result below.
    const double caNorm = c.cwiseProduct(a).norm();
    const VectorXd xStar = -(c.array() * a.array().square() / caNorm).matrix();
    VINCP::printVector("x* exact ", xStar);

    // VI model: F(z) = c (constant). All components free; the ellipsoid, not
    // non-negativity, is the feasible set, so it is supplied as the projector below.
    VINCP::VIModel model;
    model.n = dim;
    model.m = 0;
    model.H = [c](const VectorXd&, const VectorXd&) -> VectorXd { return c; };
    model.G = [](const VectorXd&, const VectorXd&) -> VectorXd { return VectorXd(); };

    const VINCP::Projector K = VINCP::makeEllipsoidProjector(a);
    const VectorXd z0 = VectorXd::Zero(dim);   // origin: strictly inside E(a)

    struct Method { const char* name; VINCP::InnerSolver inner; };
    const Method methods[] = {
        { "dHan06",  VINCP::makeDHan06Solver(innerMagTol, innerIterMax, 0) },
        { "bsHe94b", VINCP::makeBsHe94bSolver(innerMagTol, innerIterMax, 0) },
    };

    bool allPass = true;
    char linebuf[192];
    for (const Method& m : methods) {
        printf("\n");
        std::snprintf(linebuf, sizeof linebuf, "inner solver: %s", m.name);
        VINCP::printComment(latex, linebuf);

        const VINCP::JosephyNewtonParams jn;   // defaults
        const auto tStart = VINCP::utcNow();
        try {
            const VINCP::VIResult r = VINCP::solveVI(model, z0, m.inner, jn, {}, K);
            VINCP::utcElapsed(tStart);

            VINCP::printSolveStats(m.name, r);
            VINCP::printVector("x", r.z);

            // Feasibility: on/inside the ellipsoid.
            const double en = VINCP::ellipsoidNorm(r.z, a);
            const bool feasible = (en <= 1.0 + feasTol);
            std::snprintf(linebuf, sizeof linebuf,
                          "ellipsoidNorm(x) = %.12f, feasible = %s", en,
                          feasible ? "true" : "false");
            VINCP::printComment(latex, linebuf);

            // Accuracy: agreement with the closed-form optimum x*.
            const double err    = (r.z - xStar).norm();
            const double relErr = err / xStar.norm();
            const bool   accurate = (relErr <= relTol);
            std::snprintf(linebuf, sizeof linebuf,
                          "||x - x*|| = %.3e, relative = %.3e, accurate = %s", err, relErr,
                          accurate ? "true" : "false");
            VINCP::printComment(latex, linebuf);

            allPass = allPass && feasible && accurate;
        } catch (const std::exception& ex) {
            VINCP::utcElapsed(tStart);
            std::snprintf(linebuf, sizeof linebuf, "solve threw (no result to show): %s", ex.what());
            VINCP::printComment(latex, linebuf);
            allPass = false;
        }
    }

    printf("\n");
    if (allPass) {
        printf("PASS (both inner solvers reached the analytic optimum on the ellipsoid)\n");
        return 0;
    }
    printf("FAIL (an inner solver was infeasible, off the optimum, or threw)\n");
    return 1;
}
// Copyright Ben Paul Wise. All Rights Reserved.

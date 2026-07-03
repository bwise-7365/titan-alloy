// Copyright Ben Paul Wise. All Rights Reserved.
#include "utils.hpp"
#include "dampednewton.hpp"

#include <Eigen/Dense>
#include <cstdio>

using namespace VINCP;
using std::printf;

// Unit test for dampedNewtonSolve, the C++ port of dNewton.m (smoothed damped-Newton
// with a theta-blend of Gauss-Newton and scaled gradient descent, plus an Armijo line
// search). Like lm_test it solves BOTH rectangular shapes of a cubic F: R^nIn -> R^nOut
// with F(x*) = 0 -- overdetermined 10 -> 15 (unique root, check ||x - x*||) and
// underdetermined 15 -> 10 (root manifold, check convergence and ||A (x - x*)||) -- via
// the shared harness runCubicLsqCase (same problems and checks as lm_test).
int main() {
    VINCP::ScopedUtcTimer timer("damped_newton_test");

    // Bind dampedNewtonSolve (defaults: meritTol 1e-16, iterMax 200, theta 0.618034).
    const auto solve = [](const VINCP::VectorField& F, const VectorXd& x0) -> VINCP::VIResult {
        return VINCP::dampedNewtonSolve(F, x0, VINCP::DampedNewtonParams{});
    };

    int failures = 0;
    failures += VINCP::runCubicLsqCase("dNewton", 10, 15, 56639775, solve);  // overdetermined
    failures += VINCP::runCubicLsqCase("dNewton", 15, 10, 20260703, solve);  // underdetermined

    printf("\n%s\n", (failures == 0) ? "PASS (both rectangular shapes solved)"
                                     : "FAIL (a damped-Newton case did not solve)");
    return (failures == 0) ? 0 : 1;
}
// Copyright Ben Paul Wise. All Rights Reserved.

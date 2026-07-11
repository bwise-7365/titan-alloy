// Copyright Ben Paul Wise. All Rights Reserved.
#include "utils.hpp"
#include "dampednewton.hpp"

#include <Eigen/Dense>
#include <gtest/gtest.h>

using namespace VIMCP;

// Unit test for dampedNewtonSolve, the C++ port of dNewton.m (smoothed damped-Newton
// with a theta-blend of Gauss-Newton and scaled gradient descent, plus an Armijo line
// search). Like lm_test it solves BOTH rectangular shapes of a cubic F: R^nIn -> R^nOut
// with F(x*) = 0 -- overdetermined 10 -> 15 (unique root, check ||x - x*||) and
// underdetermined 15 -> 10 (root manifold, check convergence and ||A (x - x*)||) -- via
// the shared harness runCubicLsqCase (same problems and checks as lm_test).
namespace {
    // Bind dampedNewtonSolve (defaults: meritTol 1e-16, iterMax 200, theta 0.618034).
    VIResult solveDampedNewton(const VectorField& F, const VectorXd& x0) {
        return dampedNewtonSolve(F, x0, DampedNewtonParams{});
    }
} // namespace

TEST(DampedNewton, OverdeterminedCubic) {
    EXPECT_EQ(0, runCubicLsqCase("dNewton", 10, 15, 56639775, solveDampedNewton));
}

TEST(DampedNewton, UnderdeterminedCubic) {
    EXPECT_EQ(0, runCubicLsqCase("dNewton", 15, 10, 20260703, solveDampedNewton));
}
// Copyright Ben Paul Wise. All Rights Reserved.

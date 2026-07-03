// Copyright Ben Paul Wise. All Rights Reserved.
#include "utils.hpp"
#include "levenbergmarquardt.hpp"

#include <Eigen/Dense>
#include <gtest/gtest.h>

using namespace VINCP;

// Unit test for levenbergMarquardtSolve (and, through it, the LM building blocks
// levenbergMarquardtDamp / levenbergMarquardtUpdate). It runs BOTH rectangular shapes
// of a cubic map F: R^nIn -> R^nOut with F(x*) = 0, exercising the generalized
// normal-equations operator J^T J + lambda I for m > n and m < n:
//   - 10 -> 15: OVERdetermined, unique root x*  (checked with ||x - x*||).
//   - 15 -> 10: UNDERdetermined, root manifold x* + null(A), where J^T J is rank
//     deficient and the lambda I damping is essential (checked with convergence and
//     ||A (x - x*)||). The shared harness runCubicLsqCase does the setup and checks
//     and returns 0 on pass; each shape is asserted as its own GoogleTest case.
namespace {
    // Bind levenbergMarquardtSolve (with its tolerances) into a (F, x0) -> VIResult.
    VIResult solveLm(const VectorField& F, const VectorXd& x0) {
        LevenbergMarquardtSolveParams lmp;
        lmp.meritTol = 1.0e-16;
        lmp.iterMax  = 200;
        lmp.innerMax = 40;
        return levenbergMarquardtSolve(F, x0, lmp);
    }
} // namespace

TEST(LevenbergMarquardt, OverdeterminedCubic) {
    EXPECT_EQ(0, runCubicLsqCase("LM", 10, 15, 56639775, solveLm));
}

TEST(LevenbergMarquardt, UnderdeterminedCubic) {
    EXPECT_EQ(0, runCubicLsqCase("LM", 15, 10, 20260703, solveLm));
}
// Copyright Ben Paul Wise. All Rights Reserved.

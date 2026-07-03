// Copyright Ben Paul Wise. All Rights Reserved.
#include "utils.hpp"
#include "levenbergmarquardt.hpp"

#include <Eigen/Dense>
#include <cstdio>

using Eigen::VectorXd;
using std::printf;

// Unit test for levenbergMarquardtSolve (and, through it, the LM building blocks
// levenbergMarquardtDamp / levenbergMarquardtUpdate). It runs BOTH rectangular shapes
// of a cubic map F: R^nIn -> R^nOut with F(x*) = 0, exercising the generalized
// normal-equations operator J^T J + lambda I for m > n and m < n:
//   - 10 -> 15: OVERdetermined, unique root x*  (checked with ||x - x*||).
//   - 15 -> 10: UNDERdetermined, root manifold x* + null(A), where J^T J is rank
//     deficient and the lambda I damping is essential (checked with convergence and
//     ||A (x - x*)||). The shared harness runCubicLsqCase does the setup and checks.
int main() {
    VINCP::ScopedUtcTimer timer("lm_test");

    // Bind levenbergMarquardtSolve (with its tolerances) into a (F, x0) -> VIResult.
    const auto solve = [](const VINCP::VectorField& F, const VectorXd& x0) -> VINCP::VIResult {
        VINCP::LevenbergMarquardtSolveParams lmp;
        lmp.meritTol = 1.0e-16;
        lmp.iterMax  = 200;
        lmp.innerMax = 40;
        return VINCP::levenbergMarquardtSolve(F, x0, lmp);
    };

    int failures = 0;
    failures += VINCP::runCubicLsqCase("LM", 10, 15, 56639775, solve);  // overdetermined
    failures += VINCP::runCubicLsqCase("LM", 15, 10, 20260703, solve);  // underdetermined

    printf("\n%s\n", (failures == 0) ? "PASS (both rectangular shapes solved)"
                                     : "FAIL (an LM case did not solve)");
    return (failures == 0) ? 0 : 1;
}
// Copyright Ben Paul Wise. All Rights Reserved.

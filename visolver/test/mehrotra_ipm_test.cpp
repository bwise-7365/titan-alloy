// Copyright Ben Paul Wise. All Rights Reserved.
#include "mehrotraipm.hpp"
#include "bshe94b.hpp"
#include "utils.hpp"
#include "testsupport.hpp"

#include <Eigen/Dense>
#include <cstdint>
#include <random>
#include <stdexcept>
#include <string>

using namespace VINCP;

// GoogleTest suite for the Mehrotra predictor-corrector interior-point engine.
//
// The headline property under test -- the reason this engine exists -- is
// DEGENERACY-INSENSITIVE iteration counts: tens of iterations whether or not
// strict complementarity holds at the solution, on instances of the kind that
// drive the projection-contraction engines' rates toward 1. So every solve
// here asserts an iteration budget, not just closeness.

namespace {

    // Iteration budget asserted on every convergent solve: "tens of
    // iterations", with headroom for unlucky random draws.
    constexpr int kIterBudget = 50;
    constexpr int kIterMax    = 100;   // cap handed to the solver
    constexpr Index kNumFree  = 0;     // the pure-LCP cases; mixed cases pass their own

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

// Mirror of lcp_psd_test: random monotone LCP with a known STRICTLY
// complementary solution (makeComplementaryPair; M = A^T A with square A is
// positive definite almost surely, so the solution is unique). Unlike the
// Solodov-Svaiter mirror, the bars here are TIGHT: an interior-point method
// owes us full convergence in tens of iterations.
TEST(MehrotraIpm, MonotoneKnownSolution) {
    const Index N = 10;                          // problem dimension
    const std::uint_fast32_t seed = makeSeed(0, true);

    const int    intLo  = 1,    intHi  = 10;     // integer range for w, z entries
    const double realLo = -5.0, realHi = 10.0;   // double range for A entries

    const double magTol = 1.0e-14;               // squared-residual tolerance
    const double solTol = 3.0e-6;                // the tight lcp_psd_test bar

    std::mt19937 rng(seed);

    VectorXd w, z;
    makeComplementaryPair(N, rng, intLo, intHi, w, z);
    const MatrixXd M = makeGramMatrix(N, N, rng, realLo, realHi);
    const VectorXd q = w - M * z;

    printConstructed(z, w);

    VIResult result;
    ASSERT_NO_THROW({
        result = mehrotraIpm(M, q, kNumFree, magTol, kIterMax, 0);
    });
    printSolveStats("mehrotraIpm", result);

    for (const CheckFn& check : { checkConverged(), checkIterAtMost(kIterBudget),
                                  checkCloseToKnown(z, solTol) }) {
        const CheckResult cr = check(result);
        EXPECT_TRUE(cr.pass) << cr.report;
    }
}

// THE test this engine was built for: strict complementarity FAILS at the
// solution (every third index has y*_i = w*_i = 0), which is exactly the
// degeneracy that stalls active-set-style tails and flattens the projection
// engines' contraction. M stays positive definite, so the solution is still
// unique and can be checked exactly. The interior-point path ends at the
// analytic center of the face regardless, and the iteration budget must hold.
TEST(MehrotraIpm, DegenerateSolutionStaysCheap) {
    const Index N = 12;
    const Index degenerateStride = 3;            // indices 0, 3, 6, 9 doubly zero
    const std::uint_fast32_t seed = makeSeed(0, true);

    const int    intLo  = 1,    intHi  = 10;
    const double realLo = -5.0, realHi = 10.0;

    const double magTol = 1.0e-14;
    const double solTol = 3.0e-6;

    std::mt19937 rng(seed);

    VectorXd w, z;
    makeComplementaryPair(N, rng, intLo, intHi, w, z);
    for (Index i = 0; i < N; i += degenerateStride) {
        w(i) = 0.0;                              // kill strict complementarity:
        z(i) = 0.0;                              // both sides zero at index i
    }
    const MatrixXd M = makeGramMatrix(N, N, rng, realLo, realHi);
    const VectorXd q = w - M * z;

    printConstructed(z, w);

    VIResult result;
    ASSERT_NO_THROW({
        result = mehrotraIpm(M, q, kNumFree, magTol, kIterMax, 0);
    });
    printSolveStats("mehrotraIpm", result);

    for (const CheckFn& check : { checkConverged(), checkIterAtMost(kIterBudget),
                                  checkCloseToKnown(z, solTol) }) {
        const CheckResult cr = check(result);
        EXPECT_TRUE(cr.pass) << cr.report;
    }
}

// Rank-deficient M = A^T A with wide A (rank N/2): the solution set can be a
// whole face, the flow-planning failure geometry in miniature. No closeness
// check (the solver may legitimately land anywhere on the face); convergence
// of the natural residual within the iteration budget is the claim.
TEST(MehrotraIpm, RankDeficientFaceConverges) {
    const Index N = 12;
    const Index rankRows = N / 2;                // A is rankRows x N
    const std::uint_fast32_t seed = makeSeed(0, true);

    const int    intLo  = 1,    intHi  = 10;
    const double realLo = -5.0, realHi = 10.0;

    const double magTol = 1.0e-14;

    std::mt19937 rng(seed);

    VectorXd w, z;
    makeComplementaryPair(N, rng, intLo, intHi, w, z);
    const MatrixXd M = makeGramMatrix(rankRows, N, rng, realLo, realHi);
    const VectorXd q = w - M * z;                // z is A (not THE) solution

    printConstructed(z, w);

    VIResult result;
    ASSERT_NO_THROW({
        result = mehrotraIpm(M, q, kNumFree, magTol, kIterMax, 0);
    });
    printSolveStats("mehrotraIpm", result);

    for (const CheckFn& check : { checkConverged(), checkIterAtMost(kIterBudget) }) {
        const CheckResult cr = check(result);
        EXPECT_TRUE(cr.pass) << cr.report;
    }
}

// Mixed problem, hand-checkable: project p = (1, 1) onto the halfspace
// u1 + u2 <= 1, i.e. min 1/2 ||u - p||^2 s.t. A u <= b with A = [1 1], b = 1.
// KKT as a mixed LCP over K = R^2 x R_+ with z = (u, lambda):
//     M = [ Q   A^T ]      q = ( -p )
//         [ -A   0  ]          (  b )
// Solution u* = (1/2, 1/2), lambda* = 1/2 (constraint active, strictly
// complementary), verifiable by hand from the KKT stationarity row.
TEST(MehrotraIpm, MixedHandQpKnownSolution) {
    const Index numU = 2;                        // free block (primal u)
    const Index numLambda = 1;                   // multiplier block
    const Index total = numU + numLambda;
    const double magTol = 1.0e-14;
    const double solTol = 1.0e-6;

    MatrixXd M = MatrixXd::Zero(total, total);
    M(0, 0) = 1.0;  M(1, 1) = 1.0;               // Q = I
    M(0, 2) = 1.0;  M(1, 2) = 1.0;               // A^T
    M(2, 0) = -1.0; M(2, 1) = -1.0;              // -A

    VectorXd q(total);
    q << -1.0, -1.0, 1.0;                        // (-p; b)

    VectorXd known(total);
    known << 0.5, 0.5, 0.5;

    VIResult result;
    ASSERT_NO_THROW({
        result = mehrotraIpm(M, q, numU, magTol, kIterMax, 0);
    });
    printSolveStats("mehrotraIpm", result);

    for (const CheckFn& check : { checkConverged(), checkIterAtMost(kIterBudget),
                                  checkCloseToKnown(known, solTol) }) {
        const CheckResult cr = check(result);
        EXPECT_TRUE(cr.pass) << cr.report;
    }
}

// Random inequality-constrained convex QP (Q positive definite almost surely,
// so the primal block is unique; independent random A makes the multipliers
// unique too), solved twice: interior-point here, bsHe94b on the SAME mixed
// affine VI over makeMixedProjector. The engines share nothing but the
// problem data, so agreement is a strong cross-check of both.
TEST(MehrotraIpm, MixedRandomQpMatchesBsHe94b) {
    const Index numU = 4;                        // primal variables
    const Index numCon = 3;                      // inequality constraints
    const Index total = numU + numCon;
    const std::uint_fast32_t seed = makeSeed(0, true);

    const double gramLo = -1.0, gramHi = 1.0;    // G entries for Q = G^T G
    const double aLo = -2.0, aHi = 2.0;          // A entries
    const double bLo = 1.0,  bHi = 5.0;          // b entries (u = 0 stays feasible)
    const double costLo = -5.0, costHi = 5.0;    // linear-cost entries

    const double magTol = 1.0e-14;
    const int heIterMax = 200000;
    const double crossTol = 1.0e-5;              // ||z_ipm - z_he|| agreement bar

    std::mt19937 rng(seed);
    const MatrixXd Q = makeGramMatrix(numU, numU, rng, gramLo, gramHi);

    std::uniform_real_distribution<double> aDist(aLo, aHi);
    MatrixXd A(numCon, numU);
    for (Index r = 0; r < numCon; ++r) {
        for (Index col = 0; col < numU; ++col) {
            A(r, col) = aDist(rng);
        }
    }
    std::uniform_real_distribution<double> bDist(bLo, bHi);
    VectorXd b(numCon);
    for (Index i = 0; i < numCon; ++i) {
        b(i) = bDist(rng);
    }
    std::uniform_real_distribution<double> costDist(costLo, costHi);
    VectorXd cost(numU);
    for (Index i = 0; i < numU; ++i) {
        cost(i) = costDist(rng);
    }

    MatrixXd M = MatrixXd::Zero(total, total);
    M.topLeftCorner(numU, numU) = Q;
    M.topRightCorner(numU, numCon) = A.transpose();
    M.bottomLeftCorner(numCon, numU) = -A;

    VectorXd q(total);
    q.head(numU) = cost;
    q.tail(numCon) = b;

    VIResult ipm;
    ASSERT_NO_THROW({
        ipm = mehrotraIpm(M, q, numU, magTol, kIterMax, 0);
    });
    printSolveStats("mehrotraIpm", ipm);

    VIResult he;
    const VectorXd z0 = VectorXd::Zero(total);
    ASSERT_NO_THROW({
        he = bsHe94b(z0, M, q, makeMixedProjector(numU), magTol, heIterMax, 0);
    });
    printSolveStats("bsHe94b", he);

    EXPECT_TRUE(ipm.converged);
    EXPECT_TRUE(he.converged);
    EXPECT_LE(ipm.iter, kIterBudget);
    EXPECT_LT((ipm.z - he.z).norm(), crossTol);
}

// The regularization rescue: Q = diag(1, 0) leaves u2 with no curvature and
// no constraint touches it, so the Newton matrix carries an exact zero row in
// the free block -- the unregularized predictor solve is singular from the
// first iteration. The solver must add regEpsilon to the free diagonal
// (stickily), converge to u1 = 1, lambda = 0 (constraint u1 <= 10 inactive),
// and leave the flat coordinate u2 pinned at its start value 0 (its Newton
// row decouples under the regularization and its right-hand side is 0).
TEST(MehrotraIpm, RegularizationRescuesSingularFreeBlock) {
    const Index numU = 2;
    const Index numCon = 1;
    const Index total = numU + numCon;
    const double magTol = 1.0e-14;
    const double solTol = 1.0e-5;

    MatrixXd M = MatrixXd::Zero(total, total);
    M(0, 0) = 1.0;                               // Q = diag(1, 0)
    M(0, 2) = 1.0;                               // A^T, A = [1 0]
    M(2, 0) = -1.0;                              // -A

    VectorXd q(total);
    q << -1.0, 0.0, 10.0;                        // cost (-1, 0); b = 10

    VectorXd known(total);
    known << 1.0, 0.0, 0.0;                      // u* = (1, 0), lambda* = 0

    VIResult result;
    ASSERT_NO_THROW({
        result = mehrotraIpm(M, q, numU, magTol, kIterMax, 0);
    });
    printSolveStats("mehrotraIpm", result);

    for (const CheckFn& check : { checkConverged(), checkIterAtMost(kIterBudget),
                                  checkCloseToKnown(known, solTol) }) {
        const CheckResult cr = check(result);
        EXPECT_TRUE(cr.pass) << cr.report;
    }
}

// Parameter and input guards throw rather than proceed.
TEST(MehrotraIpm, RejectsBadParamsAndInputs) {
    MatrixXd M(2, 2);
    M << 1.0, 0.0,
         0.0, 1.0;
    const VectorXd q = VectorXd::Zero(2);
    const double magTol = 1.0e-12;

    MehrotraIpmParams badTau;
    badTau.tauFraction = 1.5;
    EXPECT_THROW(mehrotraIpm(M, q, kNumFree, magTol, kIterMax, 0, badTau),
                 std::invalid_argument);

    MehrotraIpmParams badSigma;
    badSigma.sigmaMin = 0.0;
    EXPECT_THROW(mehrotraIpm(M, q, kNumFree, magTol, kIterMax, 0, badSigma),
                 std::invalid_argument);

    // numFree must leave at least one complementarity component.
    EXPECT_THROW(mehrotraIpm(M, q, 2, magTol, kIterMax, 0),
                 std::invalid_argument);
    EXPECT_THROW(mehrotraIpm(M, q, -1, magTol, kIterMax, 0),
                 std::invalid_argument);

    MehrotraIpmParams badReg;
    badReg.regEpsilon = 0.0;
    EXPECT_THROW(mehrotraIpm(M, q, kNumFree, magTol, kIterMax, 0, badReg),
                 std::invalid_argument);

    EXPECT_THROW(mehrotraIpm(M, q, kNumFree, -1.0, kIterMax, 0),
                 std::invalid_argument);
    EXPECT_THROW(mehrotraIpm(M, VectorXd::Zero(3), kNumFree, magTol, kIterMax, 0),
                 std::invalid_argument);
}
// Copyright Ben Paul Wise. All Rights Reserved.

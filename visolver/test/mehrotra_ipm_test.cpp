// Copyright Ben Paul Wise. All Rights Reserved.
#include "mehrotraipm.hpp"
#include "bshe94b.hpp"
#include "utils.hpp"
#include "testsupport.hpp"

#include <Eigen/Dense>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <random>
#include <stdexcept>
#include <string>
#include <vector>

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

    // Squared-residual bar for the newtonCheckTol drift-guard tests: honest LU
    // solves sit many orders below it; a wrong factory sits far above it.
    constexpr double kNewtonCheckTol = 1.0e-8;

    // Record of the NS1 seam protocol as a factory sees it: how often the
    // factory is invoked (one factorization each), how many solves those
    // factorizations serve, and every freeRegularization value passed.
    struct SeamTrace {
        int factoryCalls = 0;
        int solveCalls = 0;
        std::vector<double> regValues;
    };

    // Counting dense wrapper: reproduces the built-in dense-LU factory's
    // arithmetic exactly (same K assembly, same partial-pivot LU) while
    // recording the seam protocol in 'trace'. M and trace must outlive it.
    NewtonSolverFactory makeCountingDenseFactory(const MatrixXd& M, Index numFree,
                                                 SeamTrace& trace) {
        return [&M, numFree, &trace](const VectorXd& sOverY,
                                     double freeRegularization) -> NewtonSolve {
            ++trace.factoryCalls;
            trace.regValues.push_back(freeRegularization);
            MatrixXd K = M;
            K.diagonal().tail(sOverY.size()) += sOverY;
            if (0.0 < freeRegularization) {
                K.diagonal().head(numFree).array() += freeRegularization;
            }
            return [&trace, luK = PartialPivLU<MatrixXd>(K)](const VectorXd& rhs) {
                ++trace.solveCalls;
                return luK.solve(rhs);
            };
        };
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
    const double objectiveTol = 1.0e-6;          // relative objective agreement
    const double crossGrossTol = 1.0e-2;         // gross ||z_ipm - z_he|| bar

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

    // This test rerolls its QP every run (makeSeed(0): microsecond clock),
    // and on an unlucky draw the KKT solution set is nearly flat, so the two
    // engines' converged POINTS legitimately differ (observed 2026-07-08:
    // 1.4e-5 apart with both residuals ~1e-14 -- the IPM ends at the
    // analytic center, the projection-contraction wherever it lands). The
    // draw-independent invariant is the OBJECTIVE, constant across the
    // solution set of a convex QP: compare it tightly, and keep only a
    // gross-disagreement bar on the points themselves.
    const VectorXd uIpm = ipm.z.head(numU);
    const VectorXd uHe = he.z.head(numU);
    const double fIpm = 0.5 * uIpm.dot(Q * uIpm) + cost.dot(uIpm);
    const double fHe = 0.5 * uHe.dot(Q * uHe) + cost.dot(uHe);
    EXPECT_LE(std::abs(fIpm - fHe), objectiveTol * (1.0 + std::abs(fHe)));
    EXPECT_LT((ipm.z - he.z).norm(), crossGrossTol);
}

// Singular free block, consistent system: Q = diag(1, 0) leaves u2 with no
// curvature, no constraint touches it, and nothing couples it -- row AND
// column 1 of the Newton matrix are exactly zero, every iteration. The
// system is nonetheless CONSISTENT (that rhs component is structurally 0:
// anything else would make the free rows infeasible), and Eigen's
// partial-pivot LU returns a FINITE solution with the flat coordinate pinned
// at 0 (a zero pivot whose numerator is exactly zero yields 0, not NaN), so
// the free-block rescue does NOT fire here -- established 2026-07-06 by the
// NS1 seam trace; the rescue itself is exercised deterministically in
// NewtonFactorySeamCarriesRescueProtocol. The assertions below are
// black-box and hold either way: converge to u1 = 1, lambda = 0 (constraint
// u1 <= 10 inactive) with u2 left at its start value 0, even should a future
// Eigen route this through the rescue instead.
TEST(MehrotraIpm, SingularConsistentFreeBlockConverges) {
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

// NS1 seam: a counting dense wrapper performs the identical arithmetic to the
// built-in factory, so the engine must reproduce the default-path result
// EXACTLY (bit for bit), invoke the factory once per iteration (one
// factorization each), draw two solves from every factorization (predictor +
// corrector), and pass freeRegularization = 0 throughout (a PD pure-LCP
// Newton matrix never needs the rescue).
TEST(MehrotraIpm, NewtonFactorySeamMatchesDefaultExactly) {
    const Index N = 10;
    const std::uint_fast32_t seed = makeSeed(0, true);

    const int    intLo  = 1,    intHi  = 10;
    const double realLo = -5.0, realHi = 10.0;
    const double magTol = 1.0e-14;

    std::mt19937 rng(seed);

    VectorXd w, z;
    makeComplementaryPair(N, rng, intLo, intHi, w, z);
    const MatrixXd M = makeGramMatrix(N, N, rng, realLo, realHi);
    const VectorXd q = w - M * z;

    VIResult viaDefault;
    ASSERT_NO_THROW({
        viaDefault = mehrotraIpm(M, q, kNumFree, magTol, kIterMax, 0);
    });

    SeamTrace trace;
    const NewtonSolverFactory counting = makeCountingDenseFactory(M, kNumFree, trace);
    VIResult viaFactory;
    ASSERT_NO_THROW({
        viaFactory = mehrotraIpm(M, q, kNumFree, magTol, kIterMax, 0,
                                 MehrotraIpmParams{}, IterationLogger{}, counting);
    });
    printSolveStats("mehrotraIpm(counting factory)", viaFactory);

    EXPECT_TRUE(viaFactory.converged);
    EXPECT_EQ(viaDefault.iter, viaFactory.iter);
    EXPECT_EQ(viaDefault.residual, viaFactory.residual);
    EXPECT_EQ(0.0, (viaDefault.z - viaFactory.z).norm());

    EXPECT_EQ(viaFactory.iter, trace.factoryCalls);
    EXPECT_EQ(2 * trace.factoryCalls, trace.solveCalls);
    for (const double reg : trace.regValues) {
        EXPECT_EQ(0.0, reg);
    }
}

// MF1: the matrix-free overload with applyM = M v and a dense factory of the
// identical arithmetic must reproduce the dense overload's result EXACTLY
// (bit for bit) -- the engine touches M only through the operator, so the
// two paths perform the same floating-point operations in the same order.
TEST(MehrotraIpm, MatrixFreeOverloadMatchesDenseExactly) {
    const Index N = 10;
    const std::uint_fast32_t seed = makeSeed(0, true);

    const int    intLo  = 1,    intHi  = 10;
    const double realLo = -5.0, realHi = 10.0;
    const double magTol = 1.0e-14;

    std::mt19937 rng(seed);

    VectorXd w, z;
    makeComplementaryPair(N, rng, intLo, intHi, w, z);
    const MatrixXd M = makeGramMatrix(N, N, rng, realLo, realHi);
    const VectorXd q = w - M * z;

    VIResult viaDense;
    ASSERT_NO_THROW({
        viaDense = mehrotraIpm(M, q, kNumFree, magTol, kIterMax, 0);
    });

    SeamTrace trace;
    const NewtonSolverFactory denseFactory =
        makeCountingDenseFactory(M, kNumFree, trace);
    const MatrixApply applyM = [&M](const VectorXd& v) -> VectorXd {
        return M * v;
    };
    VIResult viaFree;
    ASSERT_NO_THROW({
        viaFree = mehrotraIpm(applyM, q, kNumFree, magTol, kIterMax, 0,
                              MehrotraIpmParams{}, IterationLogger{},
                              denseFactory);
    });
    printSolveStats("mehrotraIpm(matrix-free)", viaFree);

    EXPECT_TRUE(viaFree.converged);
    EXPECT_EQ(viaDense.iter, viaFree.iter);
    EXPECT_EQ(viaDense.residual, viaFree.residual);
    EXPECT_EQ(0.0, (viaDense.z - viaFree.z).norm());
}

// MF1 guards: the matrix-free form requires a non-empty operator AND a
// non-empty factory (there is no dense fallback without the explicit M),
// and an operator returning the wrong size is refused, not iterated on.
TEST(MehrotraIpm, MatrixFreeFormValidatesItsInputs) {
    const Index N = 4;
    const double magTol = 1.0e-12;
    const MatrixXd M = MatrixXd::Identity(N, N);
    const VectorXd q = VectorXd::Constant(N, -1.0);

    SeamTrace trace;
    const NewtonSolverFactory denseFactory =
        makeCountingDenseFactory(M, kNumFree, trace);
    const MatrixApply applyM = [&M](const VectorXd& v) -> VectorXd {
        return M * v;
    };

    EXPECT_THROW(mehrotraIpm(MatrixApply{}, q, kNumFree, magTol, kIterMax, 0,
                             MehrotraIpmParams{}, IterationLogger{},
                             denseFactory),
                 std::invalid_argument);
    EXPECT_THROW(mehrotraIpm(applyM, q, kNumFree, magTol, kIterMax, 0),
                 std::invalid_argument);

    const MatrixApply wrongSize = [](const VectorXd& v) -> VectorXd {
        return VectorXd::Zero(v.size() + 1);
    };
    EXPECT_THROW(mehrotraIpm(wrongSize, q, kNumFree, magTol, kIterMax, 0,
                             MehrotraIpmParams{}, IterationLogger{},
                             denseFactory),
                 std::invalid_argument);
}

// NS1 seam, rescue protocol: the engine cannot know WHY a factorization is
// bad -- its contract is simply "a non-finite predictor solve requests the
// rescue". (A genuinely singular-but-CONSISTENT Newton matrix does NOT
// trigger it: Eigen's LU returns a finite vector there, see
// SingularConsistentFreeBlockConverges.) So the rescue is driven directly: a
// factory whose FIRST factorization reports failure (a NaN solve) must be
// re-invoked within the same iteration with freeRegularization = regEpsilon,
// see regEpsilon on every later iteration (the rescue is sticky), and -- the
// delegate being the honest dense solve -- still converge to the known
// hand-QP solution.
TEST(MehrotraIpm, NewtonFactorySeamCarriesRescueProtocol) {
    const Index numU = 2;                        // the MixedHandQpKnownSolution problem
    const Index numLambda = 1;
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

    std::vector<double> regValues;
    bool failNextP = true;                       // fail exactly the first factorization
    const NewtonSolverFactory failFirst =
        [&](const VectorXd& sOverY, double freeRegularization) -> NewtonSolve {
            regValues.push_back(freeRegularization);
            if (failNextP) {
                failNextP = false;
                return [](const VectorXd& rhs) {
                    return VectorXd(VectorXd::Constant(
                        rhs.size(), std::numeric_limits<double>::quiet_NaN()));
                };
            }
            MatrixXd K = M;
            K.diagonal().tail(sOverY.size()) += sOverY;
            if (0.0 < freeRegularization) {
                K.diagonal().head(numU).array() += freeRegularization;
            }
            return [luK = PartialPivLU<MatrixXd>(K)](const VectorXd& rhs) {
                return luK.solve(rhs);
            };
        };

    VIResult result;
    ASSERT_NO_THROW({
        result = mehrotraIpm(M, q, numU, magTol, kIterMax, 0,
                             MehrotraIpmParams{}, IterationLogger{}, failFirst);
    });
    printSolveStats("mehrotraIpm(fail-first factory)", result);

    for (const CheckFn& check : { checkConverged(), checkIterAtMost(kIterBudget),
                                  checkCloseToKnown(known, solTol) }) {
        const CheckResult cr = check(result);
        EXPECT_TRUE(cr.pass) << cr.report;
    }

    // Iteration 0 invokes the factory twice (the failed attempt + the
    // rescue); every later iteration invokes it once, always with the sticky
    // regEpsilon.
    ASSERT_EQ(result.iter + 1, static_cast<int>(regValues.size()));
    const double regEpsilon = MehrotraIpmParams{}.regEpsilon;
    EXPECT_EQ(0.0, regValues[0]);
    for (std::size_t k = 1; k < regValues.size(); ++k) {
        EXPECT_EQ(regEpsilon, regValues[k]);
    }
}

// The drift guard accepts honest solves: with newtonCheckTol enabled, the
// built-in factory must pass the per-solve consistency check on every
// iteration -- including the regularized ones after the rescue, which is why
// this runs on the singular-free-block QP -- and converge as before.
TEST(MehrotraIpm, NewtonCheckTolAcceptsHonestSolves) {
    const Index numU = 2;
    const Index numCon = 1;
    const Index total = numU + numCon;
    const double magTol = 1.0e-14;
    const double solTol = 1.0e-5;

    MatrixXd M = MatrixXd::Zero(total, total);
    M(0, 0) = 1.0;
    M(0, 2) = 1.0;
    M(2, 0) = -1.0;

    VectorXd q(total);
    q << -1.0, 0.0, 10.0;

    VectorXd known(total);
    known << 1.0, 0.0, 0.0;

    MehrotraIpmParams params;
    params.newtonCheckTol = kNewtonCheckTol;

    VIResult result;
    ASSERT_NO_THROW({
        result = mehrotraIpm(M, q, numU, magTol, kIterMax, 0, params);
    });
    printSolveStats("mehrotraIpm(checked)", result);

    for (const CheckFn& check : { checkConverged(), checkIterAtMost(kIterBudget),
                                  checkCloseToKnown(known, solTol) }) {
        const CheckResult cr = check(result);
        EXPECT_TRUE(cr.pass) << cr.report;
    }
}

// The drift guard catches a factory whose K disagrees with the engine's M: a
// "solver" that echoes the right-hand side back is refused on the very first
// predictor solve, with the newtonCheckTol check named in the error. (q = +1
// so that the interior start y = s = 1 is NOT already the solution -- with
// q = -1 it would be, and the engine would converge without ever calling the
// factory.)
TEST(MehrotraIpm, NewtonCheckTolCatchesWrongFactory) {
    const Index N = 4;
    const double magTol = 1.0e-14;

    const MatrixXd M = MatrixXd::Identity(N, N);
    const VectorXd q = VectorXd::Constant(N, 1.0);

    MehrotraIpmParams params;
    params.newtonCheckTol = kNewtonCheckTol;

    const NewtonSolverFactory echo =
        [](const VectorXd&, double) -> NewtonSolve {
            return [](const VectorXd& rhs) { return rhs; };
        };

    bool caughtP = false;
    try {
        mehrotraIpm(M, q, kNumFree, magTol, kIterMax, 0, params,
                    IterationLogger{}, echo);
    }
    catch (const std::runtime_error& err) {
        caughtP = true;
        EXPECT_NE(string::npos, string(err.what()).find("newtonCheckTol"));
    }
    EXPECT_TRUE(caughtP) << "the wrong factory was not caught";
}

// An empty NewtonSolve from the factory is refused, not dereferenced.
// (Same q = +1 caveat as above: the factory must actually be reached.)
TEST(MehrotraIpm, RejectsEmptySolverFromFactory) {
    const Index N = 4;
    const double magTol = 1.0e-14;

    const MatrixXd M = MatrixXd::Identity(N, N);
    const VectorXd q = VectorXd::Constant(N, 1.0);

    const NewtonSolverFactory broken =
        [](const VectorXd&, double) -> NewtonSolve { return NewtonSolve{}; };

    EXPECT_THROW(mehrotraIpm(M, q, kNumFree, magTol, kIterMax, 0,
                             MehrotraIpmParams{}, IterationLogger{}, broken),
                 std::runtime_error);
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

    MehrotraIpmParams badCheck;
    badCheck.newtonCheckTol = -1.0;
    EXPECT_THROW(mehrotraIpm(M, q, kNumFree, magTol, kIterMax, 0, badCheck),
                 std::invalid_argument);

    EXPECT_THROW(mehrotraIpm(M, q, kNumFree, -1.0, kIterMax, 0),
                 std::invalid_argument);
    EXPECT_THROW(mehrotraIpm(M, VectorXd::Zero(3), kNumFree, magTol, kIterMax, 0),
                 std::invalid_argument);
}
// Copyright Ben Paul Wise. All Rights Reserved.

// Copyright Ben Paul Wise. All Rights Reserved.
#include "chooseengine.hpp"
#include "utils.hpp"

#include <Eigen/Dense>
#include <gtest/gtest.h>

#include <random>
#include <stdexcept>
#include <vector>

using namespace VIMCP;

// Unit tests for the chooseEngine dispatcher: the monotonicity probe, the
// pure decision table (each rule of report Part II's engine-selection table),
// and the two executors -- including the model executor's evidence-backed
// fallback, driven deterministically by capping the first attempt (the
// protocol-testing pattern: force the rare path, don't hunt for it).

namespace {
    constexpr std::uint_fast32_t kSeed = 20260706u;
    constexpr Index  kDim     = 8;
    constexpr int    kIntLo   = 1;
    constexpr int    kIntHi   = 10;
    constexpr double kALo     = -1.0;
    constexpr double kAHi     = 1.0;
    constexpr double kSolTol  = 1.0e-4;   // acceptance on ||z - z*|| (sits above
                                          //   the model executor's 1e-10 SQUARED
                                          //   stop, i.e. ~1e-5 in the norm)

    // A constructed monotone LCP with known solution (the standard scaffold).
    struct KnownLcp {
        MatrixXd M;
        VectorXd q;
        VectorXd zStar;
    };

    KnownLcp
    makeKnownPsdLcp()
    {
        std::mt19937 rng(kSeed);
        VectorXd w, z;
        makeComplementaryPair(kDim, rng, kIntLo, kIntHi, w, z);
        std::uniform_real_distribution<double> aDist(kALo, kAHi);
        MatrixXd A(kDim, kDim);
        for (Index r = 0; r < kDim; ++r) {
            for (Index c = 0; c < kDim; ++c) {
                A(r, c) = aDist(rng);
            }
        }
        KnownLcp lcp;
        lcp.M = A.transpose() * A;
        lcp.zStar = z;
        lcp.q = w - lcp.M * z;
        return lcp;
    }

    // Record every dispatcher choice for assertions.
    ChoiceLogger
    recordChoices(std::vector<EngineChoice>& into)
    {
        return [&into](EngineChoice choice, const char*) {
            into.push_back(choice);
            return;
        };
    }
} // namespace

TEST(ChooseEngine, ProbeMonotoneClassifiesCorrectly) {
    const KnownLcp lcp = makeKnownPsdLcp();
    EXPECT_TRUE(probeMonotone(lcp.M));                    // PSD by construction

    MatrixXd indefinite = MatrixXd::Identity(3, 3);
    indefinite(1, 1) = -1.0;
    EXPECT_FALSE(probeMonotone(indefinite));              // eigenvalue -1

    // Rank-deficient PSD (a flat direction): the relative shift admits it.
    MatrixXd rankDeficient = MatrixXd::Zero(3, 3);
    rankDeficient(0, 0) = 1.0;
    EXPECT_TRUE(probeMonotone(rankDeficient));

    // Skew part contributes nothing to sym(M): still monotone.
    MatrixXd skew = MatrixXd::Zero(2, 2);
    skew(0, 1) = 1.0;
    skew(1, 0) = -1.0;
    EXPECT_TRUE(probeMonotone(skew));

    EXPECT_THROW(probeMonotone(MatrixXd()), std::invalid_argument);
    EXPECT_THROW(probeMonotone(MatrixXd::Zero(2, 3)), std::invalid_argument);
    EXPECT_THROW(probeMonotone(MatrixXd::Identity(2, 2), -1.0),
                 std::invalid_argument);
}

TEST(ChooseEngine, DecisionTableMatchesTheReport) {
    ProblemTraits t;
    t.dimension = 100;
    t.numFree = 10;

    // Affine, orthant, monotone, cold start -> interior point.
    t.affineP = true;  t.orthantKP = true;  t.monotoneP = true;  t.warmStartP = false;
    EXPECT_EQ(EngineChoice::MehrotraIpm, chooseEngine(t));

    // ... with a warm start -> contraction engine.
    t.warmStartP = true;
    EXPECT_EQ(EngineChoice::BsHe94b, chooseEngine(t));

    // Monotone over a non-orthant K -> contraction engine (IPM is orthant-only).
    t.warmStartP = false;  t.orthantKP = false;
    EXPECT_EQ(EngineChoice::BsHe94b, chooseEngine(t));

    // Monotonicity failed or unknown -> the globalizer chain, whatever K is.
    t.monotoneP = false;
    EXPECT_EQ(EngineChoice::ChainedSolodovHe, chooseEngine(t));
    t.orthantKP = true;
    EXPECT_EQ(EngineChoice::ChainedSolodovHe, chooseEngine(t));

    // Nonlinear mixed NCP -> the direct semismooth solver.
    t.affineP = false;  t.monotoneP = false;
    EXPECT_EQ(EngineChoice::SemismoothNewton, chooseEngine(t));

    // Nonlinear over a general K -> the Josephy-Newton driver.
    t.orthantKP = false;
    EXPECT_EQ(EngineChoice::JosephyNewton, chooseEngine(t));

    // Guards.
    ProblemTraits bad;
    bad.dimension = 0;
    EXPECT_THROW(chooseEngine(bad), std::invalid_argument);
    bad.dimension = 5;
    bad.numFree = 6;
    EXPECT_THROW(chooseEngine(bad), std::invalid_argument);
}

// Monotone + cold start dispatches to the interior-point engine and solves.
TEST(ChooseEngine, AffineAutoMonotoneColdUsesIpm) {
    const KnownLcp lcp = makeKnownPsdLcp();
    std::vector<EngineChoice> choices;
    AutoAffineParams params;
    params.onChoice = recordChoices(choices);

    const VIResult r = solveAffineAuto(VectorXd::Zero(kDim), lcp.M, lcp.q,
                                       /*numFree=*/0, params);
    ASSERT_EQ(1u, choices.size());
    EXPECT_EQ(EngineChoice::MehrotraIpm, choices.front());
    EXPECT_TRUE(r.converged);
    EXPECT_LT((r.z - lcp.zStar).norm(), kSolTol);
}

// The same problem with a declared warm start dispatches to bsHe94b instead,
// and the warm start pays: it converges from the nearby start.
TEST(ChooseEngine, AffineAutoWarmStartUsesBsHe94b) {
    const KnownLcp lcp = makeKnownPsdLcp();
    std::vector<EngineChoice> choices;
    AutoAffineParams params;
    params.warmStartP = true;
    params.onChoice = recordChoices(choices);

    const VectorXd warm =
        (lcp.zStar + VectorXd::Constant(kDim, 1.0e-3)).cwiseMax(0.0);
    const VIResult r = solveAffineAuto(warm, lcp.M, lcp.q, /*numFree=*/0, params);
    ASSERT_EQ(1u, choices.size());
    EXPECT_EQ(EngineChoice::BsHe94b, choices.front());
    EXPECT_TRUE(r.converged);
    EXPECT_LT((r.z - lcp.zStar).norm(), kSolTol);
}

// An indefinite M must dispatch to the globalizer chain. (Whether the chain
// then converges depends on the problem -- off-monotone there is no
// guarantee, so only the CHOICE is asserted; the solve may legitimately
// throw its divergence guard.)
TEST(ChooseEngine, AffineAutoIndefiniteChoosesChain) {
    MatrixXd M = MatrixXd::Identity(kDim, kDim);
    M(1, 1) = -1.0;
    const VectorXd q = VectorXd::Constant(kDim, 1.0);
    std::vector<EngineChoice> choices;
    AutoAffineParams params;
    params.onChoice = recordChoices(choices);

    try {
        solveAffineAuto(VectorXd::Zero(kDim), M, q, /*numFree=*/0, params);
    }
    catch (const std::runtime_error&) {
        // acceptable off-monotone
    }
    ASSERT_EQ(1u, choices.size());
    EXPECT_EQ(EngineChoice::ChainedSolodovHe, choices.front());
}

// The model executor's direct path: a monotone cubic mixed VI, solved by the
// semismooth first attempt with no fallback.
TEST(ChooseEngine, ModelAutoDirectSemismoothPath) {
    std::mt19937 rng(kSeed);
    const Index n = 3, m = 3, d = n + m;
    VectorXd wStar, yStar;
    makeComplementaryPair(m, rng, kIntLo, kIntHi, wStar, yStar);
    VectorXd zStar(d);
    zStar << VectorXd::Constant(n, 1.0), yStar;
    VectorXd target(d);
    target << VectorXd::Zero(n), wStar;
    const CubicProblem prob =
        makeCubicProblem(d, d, rng, zStar, target, /*forcePSD=*/true, kALo, kAHi);
    const VIModel model = makeVIModel(n, m, prob.F);

    std::vector<EngineChoice> choices;
    AutoModelParams params;
    params.onChoice = recordChoices(choices);

    const VIResult r = solveModelAuto(model, VectorXd::Zero(d), params);
    ASSERT_EQ(1u, choices.size());   // no fallback taken
    EXPECT_EQ(EngineChoice::SemismoothNewton, choices.front());
    EXPECT_TRUE(r.converged);
    EXPECT_LT((r.z - zStar).norm(), kSolTol);
}

// The fallback protocol, forced deterministically: capping the first attempt
// at one iteration makes it stall honestly, and the alternating chain must
// then run and solve the (easy) problem.
TEST(ChooseEngine, ModelAutoFallsBackToChainOnStall) {
    std::mt19937 rng(kSeed);
    const Index n = 3, m = 3, d = n + m;
    VectorXd wStar, yStar;
    makeComplementaryPair(m, rng, kIntLo, kIntHi, wStar, yStar);
    VectorXd zStar(d);
    zStar << VectorXd::Constant(n, 1.0), yStar;
    VectorXd target(d);
    target << VectorXd::Zero(n), wStar;
    const CubicProblem prob =
        makeCubicProblem(d, d, rng, zStar, target, /*forcePSD=*/true, kALo, kAHi);
    const VIModel model = makeVIModel(n, m, prob.F);

    std::vector<EngineChoice> choices;
    AutoModelParams params;
    params.ssnIterMax = 1;   // force the honest stall
    params.onChoice = recordChoices(choices);

    const VIResult r = solveModelAuto(model, VectorXd::Zero(d), params);
    ASSERT_EQ(2u, choices.size());
    EXPECT_EQ(EngineChoice::SemismoothNewton, choices[0]);
    EXPECT_EQ(EngineChoice::AlternatingChain, choices[1]);
    EXPECT_TRUE(r.converged);
    EXPECT_LT((r.z - zStar).norm(), kSolTol);
}

TEST(ChooseEngine, AffineAutoRejectsBadInputs) {
    const KnownLcp lcp = makeKnownPsdLcp();
    EXPECT_THROW(solveAffineAuto(VectorXd::Zero(kDim), lcp.M, lcp.q,
                                 /*numFree=*/kDim, AutoAffineParams{}),
                 std::invalid_argument);
    EXPECT_THROW(solveAffineAuto(VectorXd::Zero(kDim - 1), lcp.M, lcp.q,
                                 /*numFree=*/0, AutoAffineParams{}),
                 std::invalid_argument);
}
// Copyright Ben Paul Wise. All Rights Reserved.

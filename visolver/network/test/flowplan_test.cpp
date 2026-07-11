// Copyright Ben Paul Wise. All Rights Reserved.
#include "flowplan.hpp"

#include "gentlesupport.hpp"
#include "oracle.hpp"

#include <gtest/gtest.h>

#include <cstdint>
#include <stdexcept>

using namespace VIMCP;
using namespace VIMCP::Network;
using namespace VIMCP::Network::TestSupport;

namespace {

    const std::uint64_t kSeed = 20260703;
    const double kShortfallTol = 1.0e-3;

    // A mid-size REAL-SCALE instance (tonnages 1000-5000, costs 100-2000):
    // big enough to exercise the scaling and the screen, small enough for
    // fast Debug-build solves. 26 nodes, 14 sources, 16 sinks.
    Instance makeMidInstance() {
        InstanceProfile profile;
        profile.numSupplyOnly = 8;
        profile.numBoth = 6;
        profile.numDemandOnly = 10;
        profile.numNeither = 2;
        return makeRandomInstance(profile, kSeed);
    }

    // Feasibility slack for solver-produced plans in REAL units: the scaled
    // residual (~sqrt(magTol)) re-inflates by tonScale (balance rows) and by
    // tonScale*mileScale (the budget row), so size the bound to the budget.
    double feasTol(const Instance& inst) {
        return 1.0e-6 * inst.tonMileLimit;
    }

} // namespace

// The production wrapper agrees with the independent oracle on the gentle
// instance: theta, the unique R*, feasibility, and a certified exact solve.
TEST(NetworkFlowPlan, MatchesOracleOnGentleInstance) {
    const Instance inst = makeGentleInstance(kSeed);

    const FlowPlanResult produced = solveFlowPlan(inst);
    ASSERT_TRUE(produced.vi.converged);
    EXPECT_TRUE(produced.certifiedP);
    EXPECT_EQ(produced.certificateRounds, 0);        // keep-all: no screen
    EXPECT_EQ(produced.keptPairs, produced.totalPairs);
    EXPECT_LE(maxViolation(checkPlan(inst, produced.plan)), 1.0e-4);
    EXPECT_GE(produced.budgetShadowPrice, 0.0);

    const OracleResult oracle = solveOracle(inst);
    ASSERT_TRUE(oracle.vi.converged);
    EXPECT_NEAR(produced.shortfall, oracle.shortfall, kShortfallTol);
    for (Index i = 0; i < inst.numNodes; ++i) {
        if (0.0 < inst.demand(i)) {
            EXPECT_NEAR(produced.plan.resupply(i), oracle.plan.resupply(i),
                        1.0e-2);
        }
    }
}

// An aggressive 3-cheapest screen plus the certificate loop must land on the
// same optimum as the exact keep-all solve (Proposition R3), with fewer
// variables in play.
TEST(NetworkFlowPlan, ScreenCertificateRecoversExact) {
    Instance inst = makeMidInstance();
    inst.tonMileLimit = greedyPlan(inst).suggestedLimit;

    const FlowPlanResult exact = solveFlowPlan(inst);
    ASSERT_TRUE(exact.vi.converged);
    ASSERT_TRUE(exact.certifiedP);

    FlowPlanParams screenParams;
    screenParams.maxSourcesPerSink = 3;
    const FlowPlanResult screened = solveFlowPlan(inst, screenParams);
    ASSERT_TRUE(screened.vi.converged);
    ASSERT_TRUE(screened.certifiedP);
    EXPECT_LT(screened.keptPairs, screened.totalPairs);

    EXPECT_NEAR(screened.shortfall, exact.shortfall,
                kShortfallTol * (1.0 + exact.shortfall));
    for (Index i = 0; i < inst.numNodes; ++i) {
        if (0.0 < inst.demand(i)) {
            EXPECT_NEAR(screened.plan.resupply(i), exact.plan.resupply(i),
                        1.0e-3 * inst.demand(i) + 1.0e-2);
        }
    }
    EXPECT_LE(maxViolation(checkPlan(inst, exact.plan)), feasTol(inst));
    EXPECT_LE(maxViolation(checkPlan(inst, screened.plan)), feasTol(inst));

    // The gap rule (E1) must land on the same certified optimum too.
    FlowPlanParams gapParams;
    gapParams.gapFraction = 0.10;
    const FlowPlanResult gapped = solveFlowPlan(inst, gapParams);
    ASSERT_TRUE(gapped.vi.converged);
    ASSERT_TRUE(gapped.certifiedP);
    EXPECT_NEAR(gapped.shortfall, exact.shortfall,
                kShortfallTol * (1.0 + exact.shortfall));
}

// The G6-corrected baseline checks: at the greedy plan's own budget the
// optimizer is never worse than greedy; at the 80% working budget the greedy
// objective is a LOWER bound; and the optimal value is monotone in L.
TEST(NetworkFlowPlan, RespectsGreedyBaselineBounds) {
    const Instance base = makeMidInstance();
    const GreedyResult greedy = greedyPlan(base);
    const double thetaGreedy = shortfallObjective(base, greedy.plan);
    const double kBoundSlack = 1.0e-3;

    Instance atFull = base;
    atFull.tonMileLimit = greedy.tonMilesUsed;       // greedy plan feasible
    const FlowPlanResult full = solveFlowPlan(atFull);
    ASSERT_TRUE(full.certifiedP);
    EXPECT_LE(full.shortfall, thetaGreedy + kBoundSlack);

    Instance atWorking = base;
    atWorking.tonMileLimit = greedy.suggestedLimit;  // 80%: greedy infeasible
    const FlowPlanResult working = solveFlowPlan(atWorking);
    ASSERT_TRUE(working.certifiedP);
    EXPECT_GE(working.shortfall, thetaGreedy - kBoundSlack);   // G6 direction
    EXPECT_GE(working.shortfall, full.shortfall - kBoundSlack); // L4 monotone

    // The rationing optimum floors everything (with iterate slack).
    const double thetaRation =
        shortfallOfResupply(base, rationTargets(base));
    EXPECT_GE(full.shortfall, thetaRation - kBoundSlack);
    EXPECT_GE(working.shortfall, thetaRation - kBoundSlack);

    // Scarcity is NOT guaranteed at 80%: shortest-path routing can save more
    // than 20% of greedy's ton-miles, leaving the budget slack and priced at
    // zero (it does on this instance). What always holds is complementarity:
    // a positive price only with a (numerically) exhausted budget.
    if (0.0 < working.budgetShadowPrice) {
        EXPECT_LE(atWorking.tonMileLimit - working.tonMilesUsed,
                  1.0e-6 * atWorking.tonMileLimit);
    }

    // To assert a positive price, FORCE scarcity: budget half the ton-miles
    // the working optimum actually needs (its epsilon tie-break makes that
    // usage minimal for its resupply, so half of it cannot achieve it).
    ASSERT_GT(working.tonMilesUsed, 0.0);
    Instance atScarce = base;
    atScarce.tonMileLimit = 0.5 * working.tonMilesUsed;
    const FlowPlanResult scarce = solveFlowPlan(atScarce);
    ASSERT_TRUE(scarce.certifiedP);
    EXPECT_GT(scarce.budgetShadowPrice, 0.0);
    EXPECT_GE(scarce.shortfall, working.shortfall - kBoundSlack);
}

// The runtime engine switch (E4/IP3/MC3): every alternate engine (chain,
// ipm, ssn) must reach the same certified optimum as the default engine;
// unknown engines are refused.
TEST(NetworkFlowPlan, EngineSelectable) {
    Instance inst = makeMidInstance();
    inst.tonMileLimit = greedyPlan(inst).suggestedLimit;

    const FlowPlanResult he = solveFlowPlan(inst);
    ASSERT_TRUE(he.certifiedP);

    FlowPlanParams chainParams;
    chainParams.engine = "chain";
    const FlowPlanResult chained = solveFlowPlan(inst, chainParams);
    ASSERT_TRUE(chained.vi.converged);
    ASSERT_TRUE(chained.certifiedP);
    EXPECT_NEAR(chained.shortfall, he.shortfall,
                kShortfallTol * (1.0 + he.shortfall));
    EXPECT_LE(maxViolation(checkPlan(inst, chained.plan)), feasTol(inst));

    FlowPlanParams ipmParams;
    ipmParams.engine = "ipm";
    ipmParams.iterMax = 200;         // counts LU factorizations under "ipm"
    const FlowPlanResult ipm = solveFlowPlan(inst, ipmParams);
    ASSERT_TRUE(ipm.vi.converged);
    ASSERT_TRUE(ipm.certifiedP);
    EXPECT_NEAR(ipm.shortfall, he.shortfall,
                kShortfallTol * (1.0 + he.shortfall));
    EXPECT_LE(maxViolation(checkPlan(inst, ipm.plan)), feasTol(inst));

    FlowPlanParams ssnParams;
    ssnParams.engine = "ssn";
    ssnParams.iterMax = 200;         // counts LU factorizations under "ssn"
    const FlowPlanResult ssn = solveFlowPlan(inst, ssnParams);
    ASSERT_TRUE(ssn.vi.converged);
    ASSERT_TRUE(ssn.certifiedP);
    EXPECT_NEAR(ssn.shortfall, he.shortfall,
                kShortfallTol * (1.0 + he.shortfall));
    EXPECT_LE(maxViolation(checkPlan(inst, ssn.plan)), feasTol(inst));

    FlowPlanParams unknown;
    unknown.engine = "simplex";
    EXPECT_THROW(solveFlowPlan(inst, unknown), std::invalid_argument);
}

// Bad inputs are refused up front.
TEST(NetworkFlowPlan, RejectsBadInputs) {
    const Instance uncalibrated = makeMidInstance();   // tonMileLimit == 0
    EXPECT_THROW(solveFlowPlan(uncalibrated), std::invalid_argument);

    Instance inst = makeMidInstance();
    inst.tonMileLimit = greedyPlan(inst).suggestedLimit;
    FlowPlanParams badTol;
    badTol.magTol = -1.0;
    EXPECT_THROW(solveFlowPlan(inst, badTol), std::invalid_argument);
    FlowPlanParams badRounds;
    badRounds.maxCertificateRounds = -1;
    EXPECT_THROW(solveFlowPlan(inst, badRounds), std::invalid_argument);
}
// Copyright Ben Paul Wise. All Rights Reserved.

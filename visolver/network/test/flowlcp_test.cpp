// Copyright Ben Paul Wise. All Rights Reserved.
#include "flowlcp.hpp"
#include "greedy.hpp"
#include "lanesupport.hpp"

#include <gtest/gtest.h>

#include <cstdint>
#include <stdexcept>

using namespace VIMCP;
using namespace VIMCP::Network;
using namespace VIMCP::Network::TestSupport;

namespace {

    const std::uint64_t kSeed = 20260703;
    const double kTol = 1.0e-9;

} // namespace

// Shapes, the q slots, the skew incidence/budget blocks, the sink Q-blocks,
// and monotonicity (PSD symmetric part) on a small random instance.
TEST(NetworkFlowLcp, LayoutAndMonotonicity) {
    InstanceProfile profile;
    profile.numSupplyOnly = 4;
    profile.numBoth = 4;
    profile.numDemandOnly = 6;
    profile.numNeither = 1;
    Instance inst = makeRandomInstance(profile, kSeed);
    inst.tonMileLimit = greedyPlan(inst).suggestedLimit;

    const ShortestRoutes routes = computeShortestRoutes(inst);
    const ReducedProblem reduced = makeReducedProblem(inst, routes);
    const double eps = defaultTieBreakEpsilon(inst);
    const FlowLcp lcp = buildFlowLcp(inst, reduced, eps);

    const Index numSources = 8, numSinks = 10;
    ASSERT_EQ(lcp.numSources, numSources);
    ASSERT_EQ(lcp.numPairs, numSources * numSinks);   // keep-all screen
    const Index dim = lcp.numPairs + numSources + 1;
    ASSERT_EQ(lcp.M.rows(), dim);
    ASSERT_EQ(lcp.M.cols(), dim);
    ASSERT_EQ(lcp.q.size(), dim);

    // q slots: capacities and the budget.
    const Index muBase = lcp.numPairs;
    for (Index s = 0; s < numSources; ++s) {
        EXPECT_EQ(lcp.q(muBase + s),
                  inst.supplyCap(reduced.sources[static_cast<size_t>(s)]));
    }
    EXPECT_EQ(lcp.q(dim - 1), inst.tonMileLimit);

    // Skew pairs and Q-blocks.
    for (Index p = 0; p < lcp.numPairs; ++p) {
        const Index muRow = muBase + lcp.pairSourcePos[static_cast<size_t>(p)];
        EXPECT_EQ(lcp.M(p, muRow), 1.0);
        EXPECT_EQ(lcp.M(muRow, p), -1.0);
        EXPECT_EQ(lcp.M(p, dim - 1), lcp.pairCost(p));
        EXPECT_EQ(lcp.M(dim - 1, p), -lcp.pairCost(p));
        for (Index r = 0; r < lcp.numPairs; ++r) {
            const bool sameSinkP = lcp.pairSinkPos[static_cast<size_t>(p)]
                                   == lcp.pairSinkPos[static_cast<size_t>(r)];
            if (sameSinkP) {
                EXPECT_GT(lcp.M(p, r), 0.0);
                EXPECT_EQ(lcp.M(p, r), lcp.M(r, p));
            }
            else {
                EXPECT_EQ(lcp.M(p, r), 0.0);
            }
        }
    }

    // Monotone: the symmetric part has no negative eigenvalue (to round-off).
    const MatrixXd symmetric = 0.5 * (lcp.M + lcp.M.transpose());
    SelfAdjointEigenSolver<MatrixXd> eigs(symmetric, EigenvaluesOnly);
    ASSERT_EQ(eigs.info(), Success);
    EXPECT_GE(eigs.eigenvalues().minCoeff(), -1.0e-10);
}

// Slack capacity and budget: the analytic optimum t* = D - eps d D^2 / (2P)
// must zero the t-row of G = M z + q while the mu and lambda rows stay
// strictly positive (complementarity with mu = lambda = 0).
TEST(NetworkFlowLcp, KktHoldsAtUnconstrainedOptimum) {
    const double kBigBudget = 1.0e5;
    const double kEps = 1.0e-6;
    const Instance inst = makeLaneInstance(kBigBudget);
    const ShortestRoutes routes = computeShortestRoutes(inst);
    const ReducedProblem reduced = makeReducedProblem(inst, routes);
    const FlowLcp lcp = buildFlowLcp(inst, reduced, kEps);
    ASSERT_EQ(lcp.numPairs, 1);

    const double tStar =
        laneDemand - kEps * laneMiles * laneDemand * laneDemand / (2.0 * lanePriority);
    VectorXd z = VectorXd::Zero(3);
    z(0) = tStar;
    const VectorXd g = lcp.M * z + lcp.q;

    EXPECT_NEAR(g(0), 0.0, kTol);                    // stationarity, t > 0
    EXPECT_NEAR(g(1), laneCap - tStar, kTol);           // capacity slack
    EXPECT_GT(g(1), 0.0);
    EXPECT_NEAR(g(2), kBigBudget - laneMiles * tStar, kTol);   // budget slack
    EXPECT_GT(g(2), 0.0);
}

// Binding budget: t = L/d and the hand-derived shadow price
// lambda* = Q (D - t)/d - eps satisfy stationarity and complementarity
// (budget row exactly zero, lambda* > 0).
TEST(NetworkFlowLcp, KktHoldsAtBudgetBoundOptimum) {
    const double kTightBudget = 2000.0;              // forces t = 4 of 8 wanted
    const double kEps = 1.0e-6;
    const Instance inst = makeLaneInstance(kTightBudget);
    const ShortestRoutes routes = computeShortestRoutes(inst);
    const ReducedProblem reduced = makeReducedProblem(inst, routes);
    const FlowLcp lcp = buildFlowLcp(inst, reduced, kEps);

    const double tBound = kTightBudget / laneMiles;
    const double quad = 2.0 * lanePriority / (laneDemand * laneDemand);
    const double lambdaStar = quad * (laneDemand - tBound) / laneMiles - kEps;
    ASSERT_GT(lambdaStar, 0.0);

    VectorXd z = VectorXd::Zero(3);
    z(0) = tBound;
    z(2) = lambdaStar;
    const VectorXd g = lcp.M * z + lcp.q;

    EXPECT_NEAR(g(0), 0.0, kTol);                    // stationarity, t > 0
    EXPECT_NEAR(g(1), laneCap - tBound, kTol);          // capacity slack
    EXPECT_NEAR(g(2), 0.0, kTol);                    // budget EXACTLY spent
}

// The unpacker must route a shipment over a genuine multi-hop shortest path
// (the dominated-arc triangle), stay feasible, and use exactly d-hat
// ton-miles per ton (Lemma R1, constructive direction).
TEST(NetworkFlowLcp, UnpackerFollowsMultiHopRoute) {
    const double kTons = 3.0;
    Instance inst;
    inst.numNodes = 3;
    inst.supplyCap = VectorXd(3);
    inst.supplyCap << 10.0, 0.0, 0.0;
    inst.demand = VectorXd(3);
    inst.demand << 0.0, 0.0, 5.0;
    inst.priority = VectorXd::Ones(3);
    inst.cost = MatrixXd(3, 3);
    inst.cost <<   2.0, 100.0, 300.0,
                 120.0,   2.0, 100.0,
                 320.0, 110.0,   2.0;
    inst.tonMileLimit = 1.0e4;
    validateInstance(inst);

    const ShortestRoutes routes = computeShortestRoutes(inst);
    const ReducedProblem reduced = makeReducedProblem(inst, routes);
    const FlowLcp lcp = buildFlowLcp(inst, reduced, 0.0);
    ASSERT_EQ(lcp.numPairs, 1);
    ASSERT_EQ(lcp.pairCost(0), 200.0);               // 0 -> 1 -> 2

    VectorXd z = VectorXd::Zero(3);
    z(0) = kTons;
    const Plan plan = unpackFlowLcp(inst, routes, reduced, lcp, z);

    EXPECT_DOUBLE_EQ(plan.flow(0, 1), kTons);
    EXPECT_DOUBLE_EQ(plan.flow(1, 2), kTons);
    EXPECT_EQ(plan.flow(0, 2), 0.0);                 // dominated arc unused
    EXPECT_DOUBLE_EQ(plan.supplied(0), kTons);
    EXPECT_DOUBLE_EQ(plan.resupply(2), kTons);
    EXPECT_EQ(maxViolation(checkPlan(inst, plan)), 0.0);
    EXPECT_DOUBLE_EQ(tonMiles(inst, plan), kTons * lcp.pairCost(0));
}

// On a random type-1 instance with transit nodes: one ton to every sink from
// its cheapest source unpacks to a feasible plan whose resupply and ton-miles
// match the reduced solution exactly.
TEST(NetworkFlowLcp, UnpackerFeasibleOnRandomInstance) {
    InstanceProfile profile;
    profile.laydownType = 1;
    profile.numNeither = 2;
    Instance inst = makeRandomInstance(profile, kSeed);
    inst.tonMileLimit = greedyPlan(inst).suggestedLimit;

    const ShortestRoutes routes = computeShortestRoutes(inst);
    const ReducedProblem reduced = makeReducedProblem(inst, routes);
    const FlowLcp lcp = buildFlowLcp(inst, reduced,
                                     defaultTieBreakEpsilon(inst));

    // One ton to each sink on its first (cheapest) kept pair.
    const double kTonsPerSink = 1.0;
    VectorXd z = VectorXd::Zero(lcp.numPairs + lcp.numSources + 1);
    vector<bool> served(reduced.sinks.size(), false);
    for (Index p = 0; p < lcp.numPairs; ++p) {
        const size_t sink = static_cast<size_t>(lcp.pairSinkPos[static_cast<size_t>(p)]);
        if (!served[sink]) {
            z(p) = kTonsPerSink;
            served[sink] = true;
        }
    }

    const Plan plan = unpackFlowLcp(inst, routes, reduced, lcp, z);
    EXPECT_LE(maxViolation(checkPlan(inst, plan)), kTol);
    for (const Index sinkNode : reduced.sinks) {
        EXPECT_NEAR(plan.resupply(sinkNode), kTonsPerSink, kTol);
    }
    const double reducedTonMiles = lcp.pairCost.dot(z.head(lcp.numPairs));
    EXPECT_NEAR(tonMiles(inst, plan), reducedTonMiles,
                kTol * reducedTonMiles);
}

// Guard rails: uncalibrated budget, negative epsilon, and mis-sized z throw.
TEST(NetworkFlowLcp, RejectsBadInputs) {
    Instance uncalibrated = makeLaneInstance(0.0);
    const ShortestRoutes routes = computeShortestRoutes(uncalibrated);
    const ReducedProblem reduced = makeReducedProblem(uncalibrated, routes);
    EXPECT_THROW(defaultTieBreakEpsilon(uncalibrated), std::invalid_argument);
    EXPECT_THROW(buildFlowLcp(uncalibrated, reduced, 0.0),
                 std::invalid_argument);

    const Instance good = makeLaneInstance(1.0e5);
    EXPECT_THROW(buildFlowLcp(good, reduced, -1.0), std::invalid_argument);

    const FlowLcp lcp = buildFlowLcp(good, reduced, 0.0);
    const VectorXd wrongSize = VectorXd::Zero(2);
    EXPECT_THROW(unpackFlowLcp(good, routes, reduced, lcp, wrongSize),
                 std::invalid_argument);
}
// Copyright Ben Paul Wise. All Rights Reserved.

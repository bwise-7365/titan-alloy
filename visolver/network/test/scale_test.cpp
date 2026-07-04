// Copyright Ben Paul Wise. All Rights Reserved.
#include "flowplan.hpp"

#include "greedy.hpp"

#include <gtest/gtest.h>

#include <cstdint>

using namespace VINCP;
using namespace VINCP::Network;

namespace {

    const std::uint64_t kSeed = 20260703;

} // namespace

// The full 70-node spec profile through the production pipeline with the
// k = 10 screen: certified optimal, feasible, and inside the sandwich. (The
// keep-all 70-node solve is exercised by network_benchmark, in Release, where
// its 2k x 2k dense factorization belongs; task C5.)
TEST(NetworkScale, SeventyNodeScreenedCertified) {
    InstanceProfile profile;               // the spec's 70-node example
    profile.numNeither = 2;                // plus a couple of inert nodes
    Instance inst = makeRandomInstance(profile, kSeed);
    const GreedyResult greedy = greedyPlan(inst);
    inst.tonMileLimit = greedy.suggestedLimit;

    FlowPlanParams params;
    params.maxSourcesPerSink = 10;
    const FlowPlanResult result = solveFlowPlan(inst, params);

    ASSERT_TRUE(result.vi.converged);
    ASSERT_TRUE(result.certifiedP);
    EXPECT_LT(result.keptPairs, result.totalPairs);
    EXPECT_LE(maxViolation(checkPlan(inst, result.plan)),
              1.0e-6 * inst.tonMileLimit);

    // Sandwich (bound slack per the 2026-07-04 lesson: iterate feasibility
    // times the gradient scale, generously rounded up).
    const double kBoundSlack = 1.0e-3;
    const double thetaRation =
        shortfallOfResupply(inst, rationTargets(inst));
    EXPECT_GE(result.shortfall, thetaRation - kBoundSlack);
    double fullScale = 0.0;
    for (Index i = 0; i < inst.numNodes; ++i) {
        if (0.0 < inst.demand(i)) {
            fullScale += inst.priority(i);
        }
    }
    EXPECT_LE(result.shortfall, fullScale);
}
// Copyright Ben Paul Wise. All Rights Reserved.

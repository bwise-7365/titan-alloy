// Copyright Ben Paul Wise. All Rights Reserved.
#include "reduction.hpp"

#include <gtest/gtest.h>

#include <algorithm>
#include <cstdint>
#include <limits>

using namespace VINCP;
using namespace VINCP::Network;

namespace {

    const std::uint64_t kSeed = 20260703;
    const double kTol = 1.0e-9;

    // Minimal valid instance around a given cost matrix: node 0 supplies,
    // the last node demands, priorities are 1.
    Instance makeCostInstance(const MatrixXd& cost) {
        Instance inst;
        inst.numNodes = cost.rows();
        inst.supplyCap = VectorXd::Zero(inst.numNodes);
        inst.supplyCap(0) = 10.0;
        inst.demand = VectorXd::Zero(inst.numNodes);
        inst.demand(inst.numNodes - 1) = 5.0;
        inst.priority = VectorXd::Ones(inst.numNodes);
        inst.cost = cost;
        validateInstance(inst);
        return inst;
    }

    // All-simple-paths brute force for the ordinary shortest distance d0.
    double bruteShortest(const Instance& inst, Index from, Index to,
                         vector<bool>& visited) {
        if (from == to) {
            return 0.0;
        }
        visited[static_cast<size_t>(from)] = true;
        double best = std::numeric_limits<double>::infinity();
        for (Index k = 0; k < inst.numNodes; ++k) {
            if (!visited[static_cast<size_t>(k)]) {
                const double viaK = inst.cost(from, k)
                                    + bruteShortest(inst, k, to, visited);
                best = std::min(best, viaK);
            }
        }
        visited[static_cast<size_t>(from)] = false;
        return best;
    }

    double bruteShortest(const Instance& inst, Index from, Index to) {
        vector<bool> visited(static_cast<size_t>(inst.numNodes), false);
        return bruteShortest(inst, from, to, visited);
    }

    // Sum of arc costs along a recovered route.
    double routeCost(const Instance& inst, const vector<Index>& nodes) {
        double cost = 0.0;
        for (size_t step = 0; step + 1 < nodes.size(); ++step) {
            cost += inst.cost(nodes[step], nodes[step + 1]);
        }
        return cost;
    }

} // namespace

// A hand triangle where the direct arc 0 -> 2 is beaten by the two-hop route:
// distances, the recovered path, and the dominated-arc count are all known.
TEST(NetworkReduction, TriangleShortcutFound) {
    MatrixXd cost(3, 3);
    cost <<   2.0, 100.0, 300.0,
            120.0,   2.0, 100.0,
            320.0, 110.0,   2.0;
    const Instance inst = makeCostInstance(cost);
    const ShortestRoutes routes = computeShortestRoutes(inst);

    EXPECT_NEAR(routes.distance(0, 2), 200.0, kTol);   // 0 -> 1 -> 2
    EXPECT_NEAR(routes.distance(2, 0), 230.0, kTol);   // 2 -> 1 -> 0
    EXPECT_NEAR(routes.distance(0, 1), 100.0, kTol);   // direct

    const vector<Index> path = routeNodes(routes, 0, 2);
    ASSERT_EQ(path.size(), 3u);
    EXPECT_EQ(path[0], 0);
    EXPECT_EQ(path[1], 1);
    EXPECT_EQ(path[2], 2);

    // Exactly the arcs (0,2) and (2,0) are dominated (hand check in comments).
    EXPECT_EQ(countDominatedArcs(inst, routes), 2);
}

// The diagonal correction: when a round trip is cheaper than the self-arc the
// corrected distance and the recovered closed route must reflect it, and when
// it is not, the self-arc [n, n] stands.
TEST(NetworkReduction, SelfSupplyDiagonalCorrected) {
    MatrixXd cheapLoop(2, 2);
    cheapLoop << 5.0, 1.0,
                 1.0, 5.0;
    const Instance loopy = makeCostInstance(cheapLoop);
    const ShortestRoutes loopyRoutes = computeShortestRoutes(loopy);
    EXPECT_NEAR(loopyRoutes.selfDistance(0), 2.0, kTol);   // 0 -> 1 -> 0
    EXPECT_EQ(loopyRoutes.selfVia[0], 1);
    const vector<Index> loop = routeNodes(loopyRoutes, 0, 0);
    ASSERT_EQ(loop.size(), 3u);
    EXPECT_EQ(loop[0], 0);
    EXPECT_EQ(loop[1], 1);
    EXPECT_EQ(loop[2], 0);

    MatrixXd normal(2, 2);
    normal <<   3.0, 100.0,
              100.0,   3.0;
    const Instance plain = makeCostInstance(normal);
    const ShortestRoutes plainRoutes = computeShortestRoutes(plain);
    EXPECT_NEAR(plainRoutes.selfDistance(0), 3.0, kTol);   // the self-arc
    EXPECT_EQ(plainRoutes.selfVia[0], -1);
    const vector<Index> selfArc = routeNodes(plainRoutes, 0, 0);
    ASSERT_EQ(selfArc.size(), 2u);
    EXPECT_EQ(selfArc[0], 0);
    EXPECT_EQ(selfArc[1], 0);
}

// Floyd-Warshall against brute-force simple-path enumeration on a 6-node
// random instance, including the corrected diagonal.
TEST(NetworkReduction, MatchesBruteForceOnSmallRandom) {
    InstanceProfile small;
    small.numSupplyOnly = 1;
    small.numBoth = 2;
    small.numDemandOnly = 3;
    const Instance inst = makeRandomInstance(small, kSeed);
    const ShortestRoutes routes = computeShortestRoutes(inst);

    for (Index i = 0; i < inst.numNodes; ++i) {
        for (Index j = 0; j < inst.numNodes; ++j) {
            if (i != j) {
                EXPECT_NEAR(routes.distance(i, j), bruteShortest(inst, i, j),
                            kTol);
            }
        }
        double bruteSelf = inst.cost(i, i);
        for (Index k = 0; k < inst.numNodes; ++k) {
            if (k != i) {
                bruteSelf = std::min(bruteSelf,
                                     inst.cost(i, k) + bruteShortest(inst, k, i));
            }
        }
        EXPECT_NEAR(routes.selfDistance(i), bruteSelf, kTol);
    }
}

// On the full 70-node profile, every recovered route must cost exactly its
// distance: the successor matrix and the distance matrix agree everywhere.
TEST(NetworkReduction, RouteCostsMatchDistances) {
    const InstanceProfile profile;
    const Instance inst = makeRandomInstance(profile, kSeed);
    const ShortestRoutes routes = computeShortestRoutes(inst);

    for (Index i = 0; i < inst.numNodes; ++i) {
        for (Index j = 0; j < inst.numNodes; ++j) {
            const vector<Index> nodes = routeNodes(routes, i, j);
            EXPECT_EQ(nodes.front(), i);
            EXPECT_EQ(nodes.back(), j);
            const double expected = (i == j) ? routes.selfDistance(i)
                                             : routes.distance(i, j);
            EXPECT_NEAR(routeCost(inst, nodes), expected, kTol * expected);
        }
    }
}

// Reduced-problem construction: shapes, positivity, dominance by the direct
// cost, and the k-cheapest screen (ascending, and no excluded source beats a
// kept one).
TEST(NetworkReduction, ReducedProblemShapesAndScreen) {
    const InstanceProfile profile;
    const Instance inst = makeRandomInstance(profile, kSeed);
    const ShortestRoutes routes = computeShortestRoutes(inst);

    const ReducedProblem full = makeReducedProblem(inst, routes);
    const Index numSources = profile.numSupplyOnly + profile.numBoth;
    const Index numSinks = profile.numBoth + profile.numDemandOnly;
    EXPECT_EQ(static_cast<Index>(full.sources.size()), numSources);
    EXPECT_EQ(static_cast<Index>(full.sinks.size()), numSinks);
    EXPECT_EQ(full.shipCost.rows(), numSources);
    EXPECT_EQ(full.shipCost.cols(), numSinks);

    for (Index s = 0; s < numSources; ++s) {
        for (Index t = 0; t < numSinks; ++t) {
            const double ship = full.shipCost(s, t);
            EXPECT_GT(ship, 0.0);
            // The shortest route costs at most the direct arc.
            EXPECT_LE(ship, inst.cost(full.sources[static_cast<size_t>(s)],
                                      full.sinks[static_cast<size_t>(t)]));
        }
    }

    // k = 0 keeps everything, ascending.
    for (Index t = 0; t < numSinks; ++t) {
        const vector<Index>& kept = full.kept[static_cast<size_t>(t)];
        ASSERT_EQ(static_cast<Index>(kept.size()), numSources);
        for (size_t r = 0; r + 1 < kept.size(); ++r) {
            EXPECT_LE(full.shipCost(kept[r], t), full.shipCost(kept[r + 1], t));
        }
    }

    // k = 5 keeps each sink's 5 cheapest.
    const Index kKeep = 5;
    const ReducedProblem screened = makeReducedProblem(inst, routes, kKeep);
    for (Index t = 0; t < numSinks; ++t) {
        const vector<Index>& kept = screened.kept[static_cast<size_t>(t)];
        ASSERT_EQ(static_cast<Index>(kept.size()), kKeep);
        double worstKept = 0.0;
        for (const Index s : kept) {
            worstKept = std::max(worstKept, screened.shipCost(s, t));
        }
        for (Index s = 0; s < numSources; ++s) {
            if (kept.end() == std::find(kept.begin(), kept.end(), s)) {
                EXPECT_GE(screened.shipCost(s, t), worstKept);
            }
        }
    }
}
// Copyright Ben Paul Wise. All Rights Reserved.

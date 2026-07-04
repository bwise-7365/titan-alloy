// Copyright Ben Paul Wise. All Rights Reserved.
#include "instance.hpp"

#include <gtest/gtest.h>

#include <cmath>
#include <cstdint>
#include <stdexcept>

using namespace VINCP;
using namespace VINCP::Network;

namespace {

    const std::uint64_t kSeed = 20260703;
    const std::uint64_t kOtherSeed = kSeed + 1;

} // namespace

// The default profile is the 70-node spec example; the generated instance must
// reproduce its class counts and stay inside every documented data range.
TEST(NetworkInstance, GeneratorMatchesProfile) {
    const InstanceProfile profile;   // defaults = 20 supply-only / 20 both / 30 demand-only
    const Instance inst = makeRandomInstance(profile, kSeed);

    const Index numNodes =
        profile.numSupplyOnly + profile.numBoth + profile.numDemandOnly;
    EXPECT_EQ(inst.numNodes, numNodes);
    EXPECT_EQ(static_cast<Index>(sourceNodes(inst).size()),
              profile.numSupplyOnly + profile.numBoth);
    EXPECT_EQ(static_cast<Index>(sinkNodes(inst).size()),
              profile.numBoth + profile.numDemandOnly);
    EXPECT_EQ(inst.tonMileLimit, 0.0);   // uncalibrated until the greedy planner runs

    for (Index i = 0; i < numNodes; ++i) {
        if (0.0 < inst.supplyCap(i)) {
            EXPECT_GE(inst.supplyCap(i), profile.supplyLo);
            EXPECT_LE(inst.supplyCap(i), profile.supplyHi);
        }
        if (0.0 < inst.demand(i)) {
            EXPECT_GE(inst.demand(i), profile.demandLo);
            EXPECT_LE(inst.demand(i), profile.demandHi);
        }
        EXPECT_GE(inst.priority(i), profile.priorityLo);
        EXPECT_LE(inst.priority(i), profile.priorityHi);
    }

    // Cost ranges: diagonal in the self-cost band; off-diagonal in
    // [costFloor, (costFloor + milesPerUnit * diagonal-of-square) * maxJitter].
    const double maxSeparation = profile.squareSide * std::sqrt(2.0);
    const double offDiagMax =
        (profile.costFloor + profile.milesPerUnit * maxSeparation)
        * (1.0 + profile.asymmetryMax);
    for (Index i = 0; i < numNodes; ++i) {
        EXPECT_GE(inst.cost(i, i), profile.selfCostLo);
        EXPECT_LE(inst.cost(i, i), profile.selfCostHi);
        for (Index j = 0; j < numNodes; ++j) {
            if (i != j) {
                EXPECT_GE(inst.cost(i, j), profile.costFloor);
                EXPECT_LE(inst.cost(i, j), offDiagMax);
            }
        }
    }
}

// Directional asymmetry: the two directions share a geometric base, so their
// ratio is a ratio of independent jitters and must stay inside the jitter band.
TEST(NetworkInstance, GeneratorAsymmetryBounded) {
    const InstanceProfile profile;
    const Instance inst = makeRandomInstance(profile, kSeed);

    const double ratioHi = 1.0 + profile.asymmetryMax;
    const double ratioLo = 1.0 / ratioHi;
    for (Index i = 0; i < inst.numNodes; ++i) {
        for (Index j = i + 1; j < inst.numNodes; ++j) {
            const double ratio = inst.cost(i, j) / inst.cost(j, i);
            EXPECT_GE(ratio, ratioLo);
            EXPECT_LE(ratio, ratioHi);
        }
    }
}

// Same seed, same instance; different seed, different instance (within one
// platform; the distribution stream is implementation-defined across them).
TEST(NetworkInstance, GeneratorIsDeterministicPerSeed) {
    const InstanceProfile profile;
    const Instance instA = makeRandomInstance(profile, kSeed);
    const Instance instB = makeRandomInstance(profile, kSeed);
    const Instance instC = makeRandomInstance(profile, kOtherSeed);

    EXPECT_EQ((instA.cost - instB.cost).cwiseAbs().maxCoeff(), 0.0);
    EXPECT_EQ((instA.supplyCap - instB.supplyCap).cwiseAbs().maxCoeff(), 0.0);
    EXPECT_EQ((instA.demand - instB.demand).cwiseAbs().maxCoeff(), 0.0);
    EXPECT_EQ((instA.priority - instB.priority).cwiseAbs().maxCoeff(), 0.0);
    EXPECT_GT((instA.cost - instC.cost).cwiseAbs().maxCoeff(), 0.0);
}

// validateInstance must reject each structural defect family.
TEST(NetworkInstance, ValidateRejectsDefects) {
    const InstanceProfile profile;
    const Instance good = makeRandomInstance(profile, kSeed);
    EXPECT_NO_THROW(validateInstance(good));

    Instance negativeDemand = good;
    negativeDemand.demand(0) = -1.0;
    EXPECT_THROW(validateInstance(negativeDemand), std::invalid_argument);

    Instance zeroCost = good;
    zeroCost.cost(1, 2) = 0.0;
    EXPECT_THROW(validateInstance(zeroCost), std::invalid_argument);

    Instance badPriority = good;
    badPriority.demand(0) = 100.0;
    badPriority.priority(0) = 0.0;
    EXPECT_THROW(validateInstance(badPriority), std::invalid_argument);

    Instance negativeBudget = good;
    negativeBudget.tonMileLimit = -1.0;
    EXPECT_THROW(validateInstance(negativeBudget), std::invalid_argument);
}

// Type-1 laydown (alternate-laydown.txt): banded rectangles with
// bare-distance costs. Group A (supply-only) and group C (demand-only) bands
// are separated by a provable x-gap, so every A <-> C cost has a hard floor;
// all off-diagonal costs respect the global ceiling and the +/-5% directional
// jitter; the self-cost band is unchanged.
TEST(NetworkInstance, Type1LaydownBandsAndJitter) {
    InstanceProfile profile;
    profile.laydownType = 1;
    const Instance inst = makeRandomInstance(profile, kSeed);

    const Index endA = profile.numSupplyOnly;
    const Index endB = endA + profile.numBoth;
    const double jitterLo = 1.0 - profile.jitterHalfWidth;
    const double jitterHi = 1.0 + profile.jitterHalfWidth;

    // A spans x [0, 240] and C spans [320, 560]: the x-gap is 2*step - width.
    const double bandGap = 2.0 * profile.bandXStep - profile.bandXWidth;
    ASSERT_GT(bandGap, 0.0);
    for (Index a = 0; a < endA; ++a) {
        for (Index c = endB; c < inst.numNodes; ++c) {
            EXPECT_GE(inst.cost(a, c), bandGap * jitterLo);
            EXPECT_GE(inst.cost(c, a), bandGap * jitterLo);
        }
    }

    // Global bounds, the per-pair cost floor, and directional jitter for
    // every ordered pair. The ratio band applies only where NEITHER direction
    // was clamped to the floor (a clamped cost never exceeds bandMinCostHi).
    const double maxXSpan = 2.0 * profile.bandXStep + profile.bandXWidth;
    const double maxYSpan = profile.bandYHi - profile.bandYLo;
    const double costCeiling = std::hypot(maxXSpan, maxYSpan) * jitterHi;
    for (Index i = 0; i < inst.numNodes; ++i) {
        EXPECT_GE(inst.cost(i, i), profile.selfCostLo);
        EXPECT_LE(inst.cost(i, i), profile.selfCostHi);
        for (Index j = 0; j < inst.numNodes; ++j) {
            if (i != j) {
                EXPECT_GE(inst.cost(i, j), profile.bandMinCostLo);
                EXPECT_LE(inst.cost(i, j), costCeiling);
                if (inst.cost(i, j) > profile.bandMinCostHi
                        && inst.cost(j, i) > profile.bandMinCostHi) {
                    const double ratio = inst.cost(i, j) / inst.cost(j, i);
                    EXPECT_GE(ratio, jitterLo / jitterHi);
                    EXPECT_LE(ratio, jitterHi / jitterLo);
                }
            }
        }
    }
}

// Transit nodes (C_i = D_i = 0) join the node set without joining the source or
// sink lists, in both laydowns; they only transship.
TEST(NetworkInstance, TransitNodesHandled) {
    const Index kTransit = 3;
    for (int laydown = 0; laydown <= 1; ++laydown) {
        InstanceProfile profile;
        profile.laydownType = laydown;
        profile.numNeither = kTransit;
        const Instance inst = makeRandomInstance(profile, kSeed);

        const Index numClassed = profile.numSupplyOnly + profile.numBoth
                                 + profile.numDemandOnly;
        EXPECT_EQ(inst.numNodes, numClassed + kTransit);
        EXPECT_EQ(static_cast<Index>(sourceNodes(inst).size()),
                  profile.numSupplyOnly + profile.numBoth);
        EXPECT_EQ(static_cast<Index>(sinkNodes(inst).size()),
                  profile.numBoth + profile.numDemandOnly);
        for (Index i = numClassed; i < inst.numNodes; ++i) {
            EXPECT_EQ(inst.supplyCap(i), 0.0);
            EXPECT_EQ(inst.demand(i), 0.0);
        }
    }
}

// Each node gets a class-lettered, zero-padded, per-class label counter, in
// the contiguous block order [supply-only | both | demand-only | transit].
TEST(NetworkInstance, NodeLabelsByClass) {
    InstanceProfile profile;      // 20 supply-only, 20 both, 30 demand-only
    profile.numNeither = 3;       // plus 3 transit nodes
    const Instance inst = makeRandomInstance(profile, kSeed);

    ASSERT_EQ(static_cast<Index>(inst.labels.size()), inst.numNodes);
    EXPECT_EQ(inst.labels[0], "S000");    // supply-only block [0, 20)
    EXPECT_EQ(inst.labels[19], "S019");
    EXPECT_EQ(inst.labels[20], "M000");   // both block [20, 40)
    EXPECT_EQ(inst.labels[39], "M019");
    EXPECT_EQ(inst.labels[40], "D000");   // demand-only block [40, 70)
    EXPECT_EQ(inst.labels[69], "D029");
    EXPECT_EQ(inst.labels[70], "T000");   // transit block [70, 73)
    EXPECT_EQ(inst.labels[72], "T002");
    // nodeLabel mirrors the stored label, with a synthetic fallback when absent.
    EXPECT_EQ(nodeLabel(inst, 20), "M000");
    Instance bare;
    bare.numNodes = 2;
    EXPECT_EQ(nodeLabel(bare, 1), "#1");
}

// Laydown types beyond the defined ones are rejected up front.
TEST(NetworkInstance, UndefinedLaydownTypesRejected) {
    InstanceProfile future;
    future.laydownType = 2;
    EXPECT_THROW(validateProfile(future), std::invalid_argument);

    InstanceProfile negative;
    negative.laydownType = -1;
    EXPECT_THROW(validateProfile(negative), std::invalid_argument);
}

// validateProfile must reject non-realizable profiles.
TEST(NetworkInstance, ValidateProfileRejectsDefects) {
    InstanceProfile noSources;
    noSources.numSupplyOnly = 0;
    noSources.numBoth = 0;
    EXPECT_THROW(validateProfile(noSources), std::invalid_argument);

    InstanceProfile invertedRange;
    invertedRange.supplyLo = invertedRange.supplyHi + 1.0;
    EXPECT_THROW(validateProfile(invertedRange), std::invalid_argument);
}
// Copyright Ben Paul Wise. All Rights Reserved.

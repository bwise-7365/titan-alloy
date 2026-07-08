// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
// Tests for the fleet instance data model, validation, and generator.
// ----------------------------------------------
#include "fleetinstance.hpp"

#include <gtest/gtest.h>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <stdexcept>

using namespace VINCP;
using namespace VINCP::Network;

namespace {

    const std::uint64_t kSeed = 20260703;

    // Two nodes (one pure source, one pure sink), one asset, one vehicle
    // type; every distance positive. Small enough to perturb one field at a
    // time in the validation tests.
    FleetInstance makeTinyFleetInstance() {
        FleetInstance inst;
        inst.numNodes = 2;
        inst.assets = {{"crate", 2.0, 4.0}};
        inst.vehicles = {{"truck", 10.0, 100.0, 4.0, 50.0}};
        inst.supplyCap = MatrixXd(2, 1);
        inst.supplyCap << 100.0, 0.0;
        inst.demand = MatrixXd(2, 1);
        inst.demand << 0.0, 60.0;
        inst.priority = MatrixXd(2, 1);
        inst.priority << 1.0, 2.0;
        inst.distance = MatrixXd(2, 2);
        inst.distance << 2.0, 100.0,
                         120.0,  3.0;
        inst.horizonHours = 24.0;
        validateFleetInstance(inst);
        return inst;
    }

} // namespace

TEST(NetworkFleetInstance, ValidateAcceptsTinyInstance) {
    const FleetInstance inst = makeTinyFleetInstance();
    EXPECT_EQ(numAssets(inst), 1);
    EXPECT_EQ(numVehicleTypes(inst), 1);
    EXPECT_EQ(fleetSourceNodes(inst, 0), (std::vector<Index>{0}));
    EXPECT_EQ(fleetSinkNodes(inst, 0), (std::vector<Index>{1}));
    EXPECT_DOUBLE_EQ(totalFleetSupplyCap(inst, 0), 100.0);
    EXPECT_DOUBLE_EQ(totalFleetDemand(inst, 0), 60.0);
}

TEST(NetworkFleetInstance, BudgetIsCountTimesSpeedTimesHorizon) {
    const FleetInstance inst = makeTinyFleetInstance();
    EXPECT_DOUBLE_EQ(vehicleBudget(inst, 0), 4.0 * 50.0 * 24.0);
    EXPECT_THROW(vehicleBudget(inst, 1), std::invalid_argument);
    EXPECT_THROW(vehicleBudget(inst, -1), std::invalid_argument);
}

TEST(NetworkFleetInstance, ValidateRejectsDefects) {
    // One defect per case, each starting from the valid tiny instance.
    {
        FleetInstance bad = makeTinyFleetInstance();
        bad.assets.clear();
        EXPECT_THROW(validateFleetInstance(bad), std::invalid_argument);
    }
    {
        FleetInstance bad = makeTinyFleetInstance();
        bad.vehicles.clear();
        EXPECT_THROW(validateFleetInstance(bad), std::invalid_argument);
    }
    {
        FleetInstance bad = makeTinyFleetInstance();
        bad.assets[0].unitWeight = 0.0;      // with unitArea 0 too: G-F1
        bad.assets[0].unitArea = 0.0;
        EXPECT_THROW(validateFleetInstance(bad), std::invalid_argument);
    }
    {
        FleetInstance bad = makeTinyFleetInstance();
        bad.vehicles[0].weightCap = 0.0;     // carries nothing at all
        bad.vehicles[0].areaCap = 0.0;
        EXPECT_THROW(validateFleetInstance(bad), std::invalid_argument);
    }
    {
        FleetInstance bad = makeTinyFleetInstance();
        bad.vehicles[0].speedMph = 0.0;
        EXPECT_THROW(validateFleetInstance(bad), std::invalid_argument);
    }
    {
        FleetInstance bad = makeTinyFleetInstance();
        bad.vehicles[0].count = -1.0;
        EXPECT_THROW(validateFleetInstance(bad), std::invalid_argument);
    }
    {
        FleetInstance bad = makeTinyFleetInstance();
        bad.horizonHours = 0.0;
        EXPECT_THROW(validateFleetInstance(bad), std::invalid_argument);
    }
    {
        FleetInstance bad = makeTinyFleetInstance();
        bad.supplyCap(0, 0) = -1.0;
        EXPECT_THROW(validateFleetInstance(bad), std::invalid_argument);
    }
    {
        FleetInstance bad = makeTinyFleetInstance();
        bad.demand(1, 0) = -1.0;
        EXPECT_THROW(validateFleetInstance(bad), std::invalid_argument);
    }
    {
        FleetInstance bad = makeTinyFleetInstance();
        bad.priority(1, 0) = 0.0;            // demand cell needs P > 0
        EXPECT_THROW(validateFleetInstance(bad), std::invalid_argument);
    }
    {
        FleetInstance bad = makeTinyFleetInstance();
        bad.distance(0, 0) = 0.0;            // diagonal must be positive too
        EXPECT_THROW(validateFleetInstance(bad), std::invalid_argument);
    }
    {
        FleetInstance bad = makeTinyFleetInstance();
        bad.demand = MatrixXd::Zero(2, 2);   // shape mismatch (2 cols, 1 asset)
        EXPECT_THROW(validateFleetInstance(bad), std::invalid_argument);
    }
    {
        FleetInstance bad = makeTinyFleetInstance();
        bad.xCoord = VectorXd::Zero(2);      // xCoord without yCoord
        EXPECT_THROW(validateFleetInstance(bad), std::invalid_argument);
    }
    // Pure carriers are LEGAL (weight-only truck bed): must not throw.
    {
        FleetInstance pure = makeTinyFleetInstance();
        pure.vehicles[0].areaCap = 0.0;
        EXPECT_NO_THROW(validateFleetInstance(pure));
    }
}

TEST(NetworkFleetInstance, RandomFleetInstanceValidAndDeterministic) {
    const FleetProfile profile;
    const FleetInstance inst = makeRandomFleetInstance(profile, kSeed);
    EXPECT_NO_THROW(validateFleetInstance(inst));

    const Index numA = numAssets(inst);
    EXPECT_EQ(numA, 2);
    EXPECT_EQ(numVehicleTypes(inst), 2);

    // Same seed reproduces the instance exactly (max |difference| is 0).
    const FleetInstance again = makeRandomFleetInstance(profile, kSeed);
    EXPECT_EQ((inst.supplyCap - again.supplyCap).cwiseAbs().maxCoeff(), 0.0);
    EXPECT_EQ((inst.demand - again.demand).cwiseAbs().maxCoeff(), 0.0);
    EXPECT_EQ((inst.priority - again.priority).cwiseAbs().maxCoeff(), 0.0);
    EXPECT_EQ((inst.distance - again.distance).cwiseAbs().maxCoeff(), 0.0);

    // Placement carries over from the base geometry...
    const Instance base = makeRandomInstance(profile.geometry, kSeed);
    EXPECT_EQ(inst.labels, base.labels);
    EXPECT_EQ((inst.xCoord - base.xCoord).cwiseAbs().maxCoeff(), 0.0);
    EXPECT_EQ((inst.yCoord - base.yCoord).cwiseAbs().maxCoeff(), 0.0);

    // ...but distances are the G7 model: bare Euclidean separation times an
    // independent per-direction U[1, 1 + jitterMax] multiplier (1-mile
    // minimum separation guard), self-distances in the profile band. The
    // per-direction jitter keeps the matrix ALMOST symmetric.
    const double kJitterHi = 1.0 + profile.distanceJitterMax;
    for (Index i = 0; i < inst.numNodes; ++i) {
        EXPECT_GE(inst.distance(i, i), profile.selfDistanceLo);
        EXPECT_LE(inst.distance(i, i), profile.selfDistanceHi);
        for (Index j = 0; j < inst.numNodes; ++j) {
            if (i == j) {
                continue;
            }
            const double separation =
                std::max(std::hypot(inst.xCoord(i) - inst.xCoord(j),
                                    inst.yCoord(i) - inst.yCoord(j)),
                         1.0);
            EXPECT_GE(inst.distance(i, j), separation);
            EXPECT_LE(inst.distance(i, j), separation * kJitterHi);
            const double ratio = inst.distance(i, j) / inst.distance(j, i);
            EXPECT_GE(ratio, 1.0 / kJitterHi);
            EXPECT_LE(ratio, kJitterHi);
        }
    }

    // Node classes respected: demand-only nodes never supply, supply-only
    // nodes never demand (contiguous blocks of the base profile).
    const InstanceProfile& geometry = profile.geometry;
    const Index numClassed = geometry.numSupplyOnly + geometry.numBoth
                             + geometry.numDemandOnly;
    for (Index i = 0; i < inst.numNodes; ++i) {
        const bool suppliesP = i < geometry.numSupplyOnly + geometry.numBoth;
        const bool demandsP = geometry.numSupplyOnly <= i && i < numClassed;
        if (!suppliesP) {
            EXPECT_EQ(inst.supplyCap.row(i).sum(), 0.0) << "node " << i;
        }
        if (!demandsP) {
            EXPECT_EQ(inst.demand.row(i).sum(), 0.0) << "node " << i;
        }
    }

    // Every asset has at least one supply cell and one demand cell.
    for (Index a = 0; a < numA; ++a) {
        EXPECT_GT(totalFleetSupplyCap(inst, a), 0.0) << "asset " << a;
        EXPECT_GT(totalFleetDemand(inst, a), 0.0) << "asset " << a;
    }
}

// Every catalog prefix yields a valid profile; the first two entries are the
// FleetProfile defaults; out-of-range counts throw.
TEST(NetworkFleetInstance, CatalogsProvideValidTypePrefixes) {
    for (Index n = 1; n <= 10; ++n) {
        FleetProfile profile;
        profile.assets = assetCatalog(n);
        profile.vehicles = vehicleCatalog(n);
        EXPECT_NO_THROW(validateFleetProfile(profile)) << "count " << n;
        EXPECT_EQ(profile.assets.size(), static_cast<size_t>(n));
        EXPECT_EQ(profile.vehicles.size(), static_cast<size_t>(n));
    }
    EXPECT_EQ(assetCatalog(2)[0].name, "dense");
    EXPECT_EQ(assetCatalog(2)[1].name, "bulky");
    EXPECT_EQ(vehicleCatalog(2)[0].name, "truck");
    EXPECT_EQ(vehicleCatalog(2)[1].name, "airlift");
    EXPECT_THROW(assetCatalog(0), std::invalid_argument);
    EXPECT_THROW(assetCatalog(11), std::invalid_argument);
    EXPECT_THROW(vehicleCatalog(0), std::invalid_argument);
    EXPECT_THROW(vehicleCatalog(11), std::invalid_argument);
}
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------

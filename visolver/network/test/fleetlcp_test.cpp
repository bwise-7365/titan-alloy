// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
// Tests for the fleet LCP assembly and unpacker. Hand values mirror
// doc/fleet-mcp-check.mac (checks cited by number).
// ----------------------------------------------
#include "fleetlcp.hpp"

#include <gtest/gtest.h>

#include <Eigen/Eigenvalues>

#include <cstdint>
#include <stdexcept>

using namespace VIMCP;
using namespace VIMCP::Network;

namespace {

  const std::uint64_t kSeed = 20260703;
  const double kTol = 1.0e-9;

  // One demand cell, one pair, one capable type, engineered to the Maxima
  // one-cell fixture: P = 3/2, D = 7/2, rho = 5/2 (distances 1 + 3/2 = 5/2
  // round trip, kappa = 1), C = 6. The budget is the test's knob.
  FleetInstance makeMaximaCellInstance(double budgetMiles) {
    FleetInstance inst;
    inst.numNodes = 2;
    inst.assets = {{"box", 1.0, 0.0}};                 // pure weight cargo
    // kappa = T/w = 1; budget B = N * v * H = budgetMiles with v = H = 1.
    inst.vehicles = {{"cart", 1.0, 0.0, budgetMiles, 1.0}};
    inst.horizonHours = 1.0;
    inst.supplyCap = MatrixXd(2, 1);
    inst.supplyCap << 6.0, 0.0;
    inst.demand = MatrixXd(2, 1);
    inst.demand << 0.0, 3.5;
    inst.priority = MatrixXd(2, 1);
    inst.priority << 1.0, 1.5;
    inst.distance = MatrixXd(2, 2);
    inst.distance << 0.5, 1.0,
                     1.5, 0.5;
    inst.horizonHours = 1.0;
    validateFleetInstance(inst);
    return inst;
  }

} // namespace

// Layout, skew borders, per-cell Q blocks, and monotonicity on a small
// random profile, keep-all (Maxima checks 1-3 at scale). The node counts
// are deliberately modest: the assertions sweep numVars^2 entries and the
// eigensolve is O(dim^3), which at the 70-node default (~6,500 variables)
// takes tens of Debug minutes.
TEST(NetworkFleetLcp, LayoutAndMonotonicity) {
  FleetProfile profile;
  profile.geometry.numSupplyOnly = 5;
  profile.geometry.numBoth = 4;
  profile.geometry.numDemandOnly = 6;
  const FleetInstance inst = makeRandomFleetInstance(profile, kSeed);
  const FleetReducedProblem reduced = makeFleetReducedProblem(inst);
  const double epsilon = defaultFleetTieBreakEpsilon(inst);
  const FleetLcp lcp = buildFleetLcp(inst, reduced, epsilon);

  const Index dim = lcp.numVars + lcp.numSupplyCells + lcp.numTypes;
  ASSERT_EQ(lcp.M.rows(), dim);
  ASSERT_EQ(lcp.M.cols(), dim);
  ASSERT_EQ(lcp.q.size(), dim);
  EXPECT_EQ(lcp.numTypes, numVehicleTypes(inst));
  EXPECT_GT(lcp.numVars, 0);

  const Index muBase = lcp.numVars;
  const Index laBase = lcp.numVars + lcp.numSupplyCells;

  // q slots: budgets on the lambda rows.
  for (Index k = 0; k < lcp.numTypes; ++k) {
    EXPECT_DOUBLE_EQ(lcp.q(laBase + k), vehicleBudget(inst, k));
  }

  // Per-variable skew pairs and same-cell Q blocks.
  for (Index p = 0; p < lcp.numVars; ++p) {
    const Index muRow = muBase + lcp.varMuIndex[static_cast<size_t>(p)];
    const Index laRow = laBase + lcp.varType[static_cast<size_t>(p)];
    EXPECT_EQ(lcp.M(p, muRow), 1.0);
    EXPECT_EQ(lcp.M(muRow, p), -1.0);
    EXPECT_EQ(lcp.M(p, laRow), lcp.varRho(p));
    EXPECT_EQ(lcp.M(laRow, p), -lcp.varRho(p));
    for (Index r = 0; r < lcp.numVars; ++r) {
      if (lcp.varCell[static_cast<size_t>(p)]
          == lcp.varCell[static_cast<size_t>(r)]) {
        EXPECT_GT(lcp.M(p, r), 0.0);
        EXPECT_EQ(lcp.M(p, r), lcp.M(r, p));
      }
      else {
        EXPECT_EQ(lcp.M(p, r), 0.0);   // no cross-cell coupling, even for
      }                                // the same sink under another asset
    }
  }

  // Monotone: the symmetric part has no meaningfully negative eigenvalue.
  const MatrixXd sym = 0.5 * (lcp.M + lcp.M.transpose());
  Eigen::SelfAdjointEigenSolver<MatrixXd> eigen(sym);
  EXPECT_GE(eigen.eigenvalues().minCoeff(), -1.0e-10);
}

// KKT at the unconstrained interior optimum (Maxima check 4):
// y* = D - eps rho D^2 / (2P); G_y = 0, both slacks positive.
TEST(NetworkFleetLcp, KktHoldsAtUnconstrainedOptimum) {
  const double kEps = 0.01;                      // 1/100, as in the .mac
  const FleetInstance inst = makeMaximaCellInstance(40.0);
  const FleetReducedProblem reduced = makeFleetReducedProblem(inst);
  const FleetLcp lcp = buildFleetLcp(inst, reduced, kEps);
  ASSERT_EQ(lcp.numVars, 1);
  ASSERT_NEAR(lcp.varRho(0), 2.5, kTol);

  const double kP = 1.5, kD = 3.5, kRho = 2.5;
  const double yStar = kD - kEps * kRho * kD * kD / (2.0 * kP);

  VectorXd z = VectorXd::Zero(3);                // [y | mu | la]
  z(0) = yStar;
  const VectorXd g = lcp.M * z + lcp.q;
  EXPECT_NEAR(g(0), 0.0, kTol);
  EXPECT_NEAR(g(1), 6.0 - yStar, kTol);          // capacity slack
  EXPECT_NEAR(g(2), 40.0 - kRho * yStar, kTol);  // budget slack
  EXPECT_GT(g(1), 0.0);
  EXPECT_GT(g(2), 0.0);
}

// KKT at the budget-bound optimum (Maxima check 5): y* = B/rho,
// lambda* = Q (D - y*)/rho - eps; G_y = 0 and the budget row exactly 0.
TEST(NetworkFleetLcp, KktHoldsAtBudgetBoundOptimum) {
  const double kEps = 0.01;
  const FleetInstance inst = makeMaximaCellInstance(4.0);
  const FleetReducedProblem reduced = makeFleetReducedProblem(inst);
  const FleetLcp lcp = buildFleetLcp(inst, reduced, kEps);

  const double kP = 1.5, kD = 3.5, kRho = 2.5, kBudget = 4.0;
  const double quad = 2.0 * kP / (kD * kD);
  const double yStar = kBudget / kRho;           // 8/5 < D: budget binds
  const double laStar = quad * (kD - yStar) / kRho - kEps;
  ASSERT_GT(laStar, 0.0);

  VectorXd z = VectorXd::Zero(3);
  z(0) = yStar;
  z(2) = laStar;
  const VectorXd g = lcp.M * z + lcp.q;
  EXPECT_NEAR(g(0), 0.0, kTol);                  // stationarity
  EXPECT_GT(g(1), 0.0);                          // capacity slack
  EXPECT_NEAR(g(2), 0.0, kTol);                  // budget row tight
}

// The unpacker walks multi-hop routes both ways: cargo outbound, vehicles
// outbound AND on the reverse deadhead; the dominated direct arc is unused;
// the result passes checkFleetPlan.
TEST(NetworkFleetLcp, UnpackerFollowsMultiHopRoundTrip) {
  FleetInstance inst;
  inst.numNodes = 3;
  inst.assets = {{"box", 1.0, 1.0}};
  inst.vehicles = {{"truck", 10.0, 10.0, 4.0, 50.0}};   // kappa = 10
  inst.supplyCap = MatrixXd::Zero(3, 1);
  inst.supplyCap(0, 0) = 100.0;
  inst.demand = MatrixXd::Zero(3, 1);
  inst.demand(2, 0) = 60.0;
  inst.priority = MatrixXd::Ones(3, 1);
  inst.distance = MatrixXd(3, 3);
  inst.distance << 2.0,  30.0, 100.0,
                   30.0,  2.0,  20.0,
                   100.0, 20.0,  2.0;
  inst.horizonHours = 24.0;
  validateFleetInstance(inst);

  const FleetReducedProblem reduced = makeFleetReducedProblem(inst);
  const FleetLcp lcp = buildFleetLcp(inst, reduced, 0.0);
  ASSERT_EQ(lcp.numVars, 1);
  ASSERT_NEAR(lcp.varRho(0), 10.0, kTol);        // 100 round-trip / kappa 10

  const double kUnits = 5.0;
  VectorXd z = VectorXd::Zero(3);
  z(0) = kUnits;
  const FleetPlan plan = unpackFleetLcp(inst, reduced, lcp, z);

  EXPECT_DOUBLE_EQ(plan.flow[0](0, 1), kUnits);  // via node 1, not direct
  EXPECT_DOUBLE_EQ(plan.flow[0](1, 2), kUnits);
  EXPECT_EQ(plan.flow[0](0, 2), 0.0);
  const double kVehicles = kUnits / 10.0;
  EXPECT_DOUBLE_EQ(plan.vehicles[0](0, 1), kVehicles);   // loaded legs
  EXPECT_DOUBLE_EQ(plan.vehicles[0](1, 2), kVehicles);
  EXPECT_DOUBLE_EQ(plan.vehicles[0](2, 1), kVehicles);   // deadhead legs
  EXPECT_DOUBLE_EQ(plan.vehicles[0](1, 0), kVehicles);
  EXPECT_DOUBLE_EQ(plan.supplied(0, 0), kUnits);
  EXPECT_DOUBLE_EQ(plan.resupply(2, 0), kUnits);
  EXPECT_LE(maxViolation(checkFleetPlan(inst, plan)), kTol);
  EXPECT_NEAR(vehicleMiles(inst, plan, 0), 50.0, kTol);  // 0.5 veh x 100 mi
}

TEST(NetworkFleetLcp, RejectsBadInputs) {
  const FleetInstance inst = makeMaximaCellInstance(40.0);
  const FleetReducedProblem reduced = makeFleetReducedProblem(inst);
  EXPECT_THROW(buildFleetLcp(inst, reduced, -1.0), std::invalid_argument);

  const FleetLcp lcp = buildFleetLcp(inst, reduced, 0.0);
  EXPECT_THROW(unpackFleetLcp(inst, reduced, lcp, VectorXd::Zero(2)),
               std::invalid_argument);

  // Default tie-break: 1e-8 * (sum P over demand cells) / (sum budgets).
  EXPECT_NEAR(defaultFleetTieBreakEpsilon(inst),
              1.0e-8 * 1.5 / 40.0, 1.0e-20);
}

// MF1: the structural matvec agrees with the dense M on a random keep-all
// instance, for several deterministic vectors. The bar is a tight relative
// tolerance, not exactness: the two paths sum the same terms in different
// orders.
TEST(NetworkFleetLcp, ApplyMatchesDenseMatrix) {
  const double kApplyTol = 1.0e-12;
  FleetProfile profile;
  profile.geometry.numSupplyOnly = 5;
  profile.geometry.numBoth = 4;
  profile.geometry.numDemandOnly = 6;
  const FleetInstance inst = makeRandomFleetInstance(profile, kSeed);
  const FleetReducedProblem reduced = makeFleetReducedProblem(inst);
  const FleetLcp lcp =
      buildFleetLcp(inst, reduced, defaultFleetTieBreakEpsilon(inst));
  const Index dim = lcp.numVars + lcp.numSupplyCells + lcp.numTypes;

  for (int sweep = 0; sweep < 3; ++sweep) {
    VectorXd v(dim);
    for (Index i = 0; i < dim; ++i) {
      const double sign = (0 == (i + sweep) % 2) ? 1.0 : -1.0;
      v(i) = sign * (1.0 + static_cast<double>((i + 3 * sweep) % 11));
    }
    const VectorXd dense = lcp.M * v;
    const VectorXd structural = applyFleetLcpM(lcp, v);
    ASSERT_EQ(dense.size(), structural.size());
    EXPECT_LT((dense - structural).norm() / dense.norm(), kApplyTol);
  }

  EXPECT_THROW(applyFleetLcpM(lcp, VectorXd::Zero(dim + 1)),
               std::invalid_argument);
}

// MF1: the matrix-free build leaves M empty and everything else identical
// to the dense build -- q, the index lists, varQuad, and the counts.
TEST(NetworkFleetLcp, LeanBuildSkipsOnlyTheDenseMatrix) {
  FleetProfile profile;
  profile.geometry.numSupplyOnly = 5;
  profile.geometry.numBoth = 4;
  profile.geometry.numDemandOnly = 6;
  const FleetInstance inst = makeRandomFleetInstance(profile, kSeed);
  const FleetReducedProblem reduced = makeFleetReducedProblem(inst);
  const double epsilon = defaultFleetTieBreakEpsilon(inst);

  const FleetLcp dense = buildFleetLcp(inst, reduced, epsilon, true);
  const FleetLcp lean = buildFleetLcp(inst, reduced, epsilon, false);

  EXPECT_EQ(0, lean.M.size());
  ASSERT_GT(dense.M.size(), 0);
  EXPECT_EQ(dense.numVars, lean.numVars);
  EXPECT_EQ(dense.numSupplyCells, lean.numSupplyCells);
  EXPECT_EQ(dense.numCells, lean.numCells);
  EXPECT_EQ(dense.numTypes, lean.numTypes);
  EXPECT_EQ(0.0, (dense.q - lean.q).norm());
  EXPECT_EQ(0.0, (dense.varQuad - lean.varQuad).norm());
  EXPECT_EQ(0.0, (dense.varRho - lean.varRho).norm());
  EXPECT_EQ(dense.varCell, lean.varCell);
  EXPECT_EQ(dense.varMuIndex, lean.varMuIndex);
  EXPECT_EQ(dense.varType, lean.varType);

  // The structural matvec is available from the lean build and agrees with
  // the dense build's explicit M.
  const Index dim = lean.numVars + lean.numSupplyCells + lean.numTypes;
  const VectorXd v = VectorXd::LinSpaced(dim, -1.0, 1.0);
  EXPECT_LT((dense.M * v - applyFleetLcpM(lean, v)).norm(), 1.0e-10);
}
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------

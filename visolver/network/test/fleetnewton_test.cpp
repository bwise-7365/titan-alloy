// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
// Tests for the FN2 structured fleet Newton factory (fleetnewton.hpp). The
// algebra is machine-verified symbolically (fleet-newton-check.mac); these
// tests verify the TRANSCRIPTION numerically: the factory's solve must agree
// with a dense LU of the same K = M + diag(sOverY) on real fleet LCPs, at
// every cell shape and scaling the production path can produce.
// ----------------------------------------------
#include "fleetnewton.hpp"
#include "fleetsolve.hpp"
#include "newtonsupport.hpp"

#include <gtest/gtest.h>

#include <cmath>
#include <cstdint>
#include <stdexcept>

using namespace VINCP;
using namespace VINCP::Network;
using namespace VINCP::Network::TestSupport;

namespace {

  const std::uint64_t kSeed = 20260703;

  // Parity bars, as in flownewton_test: the backward error is the sharp
  // transcription check at any conditioning; the direct d-vs-d comparison
  // carries cond(K) and applies on well-scaled systems only.
  const double kDirectTol = 1.0e-8;
  const double kBackwardTol = 1.0e-12;

  // Nondimensionalize the way solveFleetPlan does internally, so the LCP is
  // the well-scaled system the engine actually factors.
  FleetInstance scaleFleetInstance(FleetInstance inst) {
    const double unitScale = inst.demand.maxCoeff();
    const double mileScale = inst.distance.maxCoeff();
    inst.supplyCap /= unitScale;
    inst.demand /= unitScale;
    inst.distance /= mileScale;
    for (VehicleType& vehicle : inst.vehicles) {
      vehicle.count /= unitScale * mileScale;
    }
    return inst;
  }

  // A 15-node fleet instance with the default 2 assets x 2 vehicle types
  // (all four (asset, type) pairs capable), so demand cells mix several
  // sources AND several types. Scaled as the production path scales.
  FleetInstance makeMediumFleetInstance(std::uint64_t seed) {
    FleetProfile profile;
    profile.geometry.numSupplyOnly = 4;
    profile.geometry.numBoth = 4;
    profile.geometry.numDemandOnly = 6;
    profile.geometry.numNeither = 1;
    return scaleFleetInstance(makeRandomFleetInstance(profile, seed));
  }

  // One asset, one vehicle type, 4 nodes on a metric line: with a screen of
  // 1 every demand cell holds exactly ONE variable -- the Sherman-Morrison
  // edge case (rank-one correction on a 1-cell).
  FleetInstance makeSingleTypeInstance() {
    FleetInstance inst;
    inst.numNodes = 4;
    inst.assets = {{"box", 1.0, 0.0}};
    inst.vehicles = {{"cart", 2.0, 0.0, 300.0, 1.0}};
    inst.horizonHours = 1.0;
    inst.supplyCap = MatrixXd::Zero(4, 1);
    inst.supplyCap(0, 0) = 10.0;
    inst.supplyCap(1, 0) = 10.0;
    inst.demand = MatrixXd::Zero(4, 1);
    inst.demand(2, 0) = 8.0;
    inst.demand(3, 0) = 6.0;
    inst.priority = MatrixXd::Ones(4, 1);
    inst.distance = MatrixXd(4, 4);
    for (Index i = 0; i < 4; ++i) {
      for (Index j = 0; j < 4; ++j) {
        inst.distance(i, j) =
            (i == j) ? 1.0 : 10.0 * std::abs(static_cast<double>(i - j));
      }
    }
    validateFleetInstance(inst);
    return scaleFleetInstance(inst);
  }

  FleetLcp buildLcpFor(const FleetInstance& inst, Index maxSourcesPerSink) {
    ScreenParams screen;
    screen.maxSourcesPerSink = maxSourcesPerSink;
    const FleetReducedProblem reduced =
        makeFleetReducedProblem(inst, screen);
    return buildFleetLcp(inst, reduced, defaultFleetTieBreakEpsilon(inst));
  }

  Index lcpDim(const FleetLcp& lcp) {
    return lcp.numVars + lcp.numSupplyCells + lcp.numTypes;
  }

  // Assert factory-vs-dense parity on one (lcp, sOverY, rhs) triple.
  void expectParity(const FleetLcp& lcp, const VectorXd& sOverY,
                    bool directCompareP) {
    MatrixXd K = lcp.M;
    K.diagonal() += sOverY;
    const VectorXd rhs = makeRhs(lcpDim(lcp));

    const VectorXd dDense = PartialPivLU<MatrixXd>(K).solve(rhs);
    const NewtonSolve solve = makeFleetNewtonFactory(lcp)(sOverY, 0.0);
    const VectorXd dFleet = solve(rhs);

    ASSERT_TRUE(dFleet.allFinite());
    EXPECT_LT(backwardError(K, dFleet, rhs), kBackwardTol);
    if (directCompareP) {
      EXPECT_LT((dFleet - dDense).norm() / dDense.norm(), kDirectTol);
    }
  }

} // namespace

// Keep-all (multi-variable cells mixing sources and types, the last cell
// ending exactly at the end of the y block): the structured solve must match
// a dense LU of the identical K, both directly and in backward error, on a
// moderate diagonal.
TEST(NetworkFleetNewton, ParityKeepAllModerateDiagonal) {
  const FleetInstance inst = makeMediumFleetInstance(kSeed);
  const FleetLcp lcp = buildLcpFor(inst, 0);
  expectParity(lcp, makeSpreadDiagonal(lcpDim(lcp), -2.0, 2.0), true);
}

// Late-iteration regime: the complementarity diagonal spans 1e-6..1e6 (the
// central path pushes s/y to both extremes as mu -> 0). Direct d-vs-d
// comparison is no longer meaningful at this conditioning, so the claim is
// the scale-aware backward error.
TEST(NetworkFleetNewton, ParityKeepAllExtremeDiagonal) {
  const FleetInstance inst = makeMediumFleetInstance(kSeed);
  const FleetLcp lcp = buildLcpFor(inst, 0);
  expectParity(lcp, makeSpreadDiagonal(lcpDim(lcp), -6.0, 6.0), false);
}

// Screen of 1 on the single-asset / single-type instance: every demand cell
// is a single variable, the Sherman-Morrison edge case.
TEST(NetworkFleetNewton, ParitySingletonCells) {
  const FleetInstance inst = makeSingleTypeInstance();
  const FleetLcp lcp = buildLcpFor(inst, 1);
  ASSERT_EQ(lcp.numVars, static_cast<Index>(2));   // one variable per sink
  expectParity(lcp, makeSpreadDiagonal(lcpDim(lcp), -2.0, 2.0), true);
}

// Extreme Q_cell spread: priorities spanning 1e-2..1e2 against single-digit
// demands push Q_cell = 2 P / D^2 across ~6 orders of magnitude, stressing
// the per-cell coefficient c_c = Q_c / (1 + Q_c sigma_c) at both ends.
TEST(NetworkFleetNewton, ParityExtremeQuadSpread) {
  FleetProfile profile;
  profile.geometry.numSupplyOnly = 4;
  profile.geometry.numBoth = 4;
  profile.geometry.numDemandOnly = 6;
  profile.geometry.numNeither = 1;
  profile.demandLo = 2.0;
  profile.demandHi = 10.0;
  profile.priorityLo = 0.01;
  profile.priorityHi = 100.0;
  const FleetInstance inst =
      scaleFleetInstance(makeRandomFleetInstance(profile, kSeed));
  const FleetLcp lcp = buildLcpFor(inst, 0);
  expectParity(lcp, makeSpreadDiagonal(lcpDim(lcp), -2.0, 2.0), false);
}

// End-to-end through the production path: solveFleetPlan with engine "ipm"
// must produce the same certified optimum under "dense" and "fleet" Newton
// linear algebra -- with the engine's newtonCheckTol drift guard ON for the
// structured run, so every predictor and corrector solve is verified against
// M inside the engine itself.
TEST(NetworkFleetNewton, EndToEndIpmParity) {
  const double kShortfallTol = 1.0e-8;
  const double kZTol = 1.0e-5;
  const double kCheckTol = 1.0e-8;               // squared drift bar

  FleetProfile profile;
  profile.geometry.numSupplyOnly = 4;
  profile.geometry.numBoth = 4;
  profile.geometry.numDemandOnly = 6;
  profile.geometry.numNeither = 1;
  const FleetInstance inst = makeRandomFleetInstance(profile, kSeed);

  FleetSolveParams dense;
  dense.maxSourcesPerSink = 0;                   // keep-all: no certificate
  dense.iterMax = 200;
  const FleetSolveResult viaDense = solveFleetPlan(inst, dense);

  FleetSolveParams fleet = dense;
  fleet.ipmNewton = "fleet";
  fleet.newtonCheckTol = kCheckTol;
  const FleetSolveResult viaFleet = solveFleetPlan(inst, fleet);

  ASSERT_TRUE(viaDense.vi.converged);
  ASSERT_TRUE(viaFleet.vi.converged);
  EXPECT_TRUE(viaDense.certifiedP);
  EXPECT_TRUE(viaFleet.certifiedP);
  EXPECT_NEAR(viaDense.shortfall, viaFleet.shortfall, kShortfallTol);
  ASSERT_EQ(viaDense.vi.z.size(), viaFleet.vi.z.size());
  EXPECT_LT((viaDense.vi.z - viaFleet.vi.z).norm(), kZTol);
}

// MF1: the factory built from a MATRIX-FREE lcp (dense M never assembled;
// Q taken from varQuad) solves the same Newton system as one built from the
// dense twin -- verified against the dense twin's explicit K.
TEST(NetworkFleetNewton, ParityFromLeanBuild) {
  const FleetInstance inst = makeMediumFleetInstance(kSeed);
  const FleetReducedProblem reduced = makeFleetReducedProblem(inst);
  const double epsilon = defaultFleetTieBreakEpsilon(inst);
  const FleetLcp dense = buildFleetLcp(inst, reduced, epsilon, true);
  const FleetLcp lean = buildFleetLcp(inst, reduced, epsilon, false);
  ASSERT_EQ(0, lean.M.size());

  const Index dim = lcpDim(dense);
  const VectorXd sOverY = makeSpreadDiagonal(dim, -2.0, 2.0);
  MatrixXd K = dense.M;
  K.diagonal() += sOverY;
  const VectorXd rhs = makeRhs(dim);

  const NewtonSolve solve = makeFleetNewtonFactory(lean)(sOverY, 0.0);
  const VectorXd d = solve(rhs);
  ASSERT_TRUE(d.allFinite());
  EXPECT_LT(backwardError(K, d, rhs), kBackwardTol);
}

// Guards: the factory validates its inputs and its lcp rather than proceed.
TEST(NetworkFleetNewton, RejectsBadInputs) {
  const FleetInstance inst = makeMediumFleetInstance(kSeed);
  const FleetLcp lcp = buildLcpFor(inst, 0);
  const Index dim = lcpDim(lcp);
  const NewtonSolverFactory factory = makeFleetNewtonFactory(lcp);
  const VectorXd good = VectorXd::Constant(dim, 1.0);

  // The fleet LCP has no free block: nonzero freeRegularization is misuse.
  EXPECT_THROW(factory(good, 1.0e-8), std::invalid_argument);

  // Wrong-sized or non-positive complementarity diagonal.
  EXPECT_THROW(factory(VectorXd::Constant(dim + 1, 1.0), 0.0),
               std::invalid_argument);
  VectorXd nonPositive = good;
  nonPositive(0) = 0.0;
  EXPECT_THROW(factory(nonPositive, 0.0), std::invalid_argument);

  // A wrong-sized rhs is refused by the returned solver.
  const NewtonSolve solve = factory(good, 0.0);
  EXPECT_THROW(solve(VectorXd::Constant(dim - 1, 1.0)),
               std::invalid_argument);

  // An inconsistent lcp is refused at factory construction.
  EXPECT_THROW(makeFleetNewtonFactory(FleetLcp{}), std::invalid_argument);

  // The ipmNewton param is validated by solveFleetPlan.
  FleetSolveParams bad;
  bad.ipmNewton = "sparse";
  EXPECT_THROW(solveFleetPlan(inst, bad), std::invalid_argument);
}
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------

// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
// Tests for the SAOE app class: the default chain reaches reference
// equilibrium E, Auto converges feasibly, unsupported engines throw, and the
// random generator round-trips through solve.
// ----------------------------------------------
#include "saoeproblem.hpp"

#include <gtest/gtest.h>

#include <cmath>
#include <stdexcept>

using namespace VINCP;
using namespace VINCP::App;

namespace {

  constexpr int    kRefActors  = 6;
  constexpr int    kRefOptions = 10;
  constexpr double kRmseTol    = 1.0e-2;   // saoe_test's bar on E
  constexpr double kFeasTol    = 1.0e-3;
  constexpr double kProbSumTol = 1.0e-6;

  // The reference instance (= alloceff01cm data) and its known equilibrium E.
  SaoeData
  referenceInstance()
  {
    SaoeData d;
    d.R.resize(kRefActors, kRefOptions);
    d.R << 0.00,  181.42,  -50.43,  -26.32,  256.02,  -21.27, -132.68,  -65.12,  131.40,   14.54,
           0.00,   31.24,  -46.53,  122.90,   39.47,  -12.94,   50.32,   70.03,    8.34, -109.78,
           0.00,  -30.11,  -48.64,  -56.84,  -51.50,  -80.42, -130.30,  -54.20,   -8.75,   51.97,
           0.00,   29.12,  160.04,  -27.68,   91.49,   80.93,  117.95,   27.88,   33.62,   72.34,
           0.00, -199.26, -234.61,   67.80, -319.57,  270.83,  234.85,  -14.91, -236.86, -103.50,
           0.00,   78.66,  -22.12,   14.25,  109.23,  -17.30,  -31.16,  -42.61,   31.46,  -46.13;
    d.S.resize(kRefActors);
    d.S << 68.0, 66.0, 125.0, 101.0, 127.0, 96.0;
    return d;
  }

  MatrixXd
  referenceEquilibrium()
  {
    MatrixXd e(kRefActors, kRefOptions);
    e << 0.00, 0.00,   0.00, 0.00,  68.00,   0.00, 0.00, 0.00, 0.00,   0.00,
         0.00, 0.00,   0.00, 66.00,  0.00,   0.00, 0.00, 0.00, 0.00,   0.00,
         0.00, 0.00,   0.00, 0.00,   0.00,   0.00, 0.00, 0.00, 0.00, 125.00,
         0.00, 0.00, 101.00, 0.00,   0.00,   0.00, 0.00, 0.00, 0.00,   0.00,
         0.00, 0.00,   0.00, 0.00,   0.00, 127.00, 0.00, 0.00, 0.00,   0.00,
         0.00, 0.00,   0.00, 0.00,  96.00,   0.00, 0.00, 0.00, 0.00,   0.00;
    return e;
  }

  double
  effortRmse(const MatrixXd& e, const MatrixXd& reference)
  {
    return std::sqrt((e - reference).array().square().sum()
                     / static_cast<double>(e.size()));
  }

  // Efforts non-negative and per-actor budgets respected.
  bool
  feasibleP(const MatrixXd& e, const VectorXd& S)
  {
    const double maxNeg  = -e.minCoeff();
    const double maxOver = (e.rowwise().sum() - S).maxCoeff();
    return maxNeg <= kFeasTol && maxOver <= kFeasTol;
  }

} // namespace

TEST(SaoeProblem, DefaultChainReachesReferenceEquilibrium)
{
  const SAOE problem(referenceInstance());
  const auto [vi, res] = problem.solve(SaoeParams{});   // Default -> Chain

  EXPECT_TRUE(vi.converged);
  EXPECT_TRUE(feasibleP(res.e, referenceInstance().S));
  EXPECT_LE(effortRmse(res.e, referenceEquilibrium()), kRmseTol);
  EXPECT_NEAR(res.probabilities.sum(), 1.0, kProbSumTol);
}

TEST(SaoeProblem, AutoEngineConvergesFeasibly)
{
  const SAOE problem(referenceInstance());
  SaoeParams params;
  params.engine = ProblemBase::Engine::Auto;
  const auto [vi, res] = problem.solve(params);

  EXPECT_TRUE(vi.converged);
  EXPECT_TRUE(feasibleP(res.e, referenceInstance().S));
  EXPECT_NEAR(res.probabilities.sum(), 1.0, kProbSumTol);
}

TEST(SaoeProblem, UnsupportedEngineThrows)
{
  const SAOE problem(referenceInstance());
  SaoeParams params;
  params.engine = ProblemBase::Engine::Ipm;
  EXPECT_THROW(problem.solve(params), std::invalid_argument);
}

TEST(SaoeProblem, GenerateRoundTripsThroughSolve)
{
  SaoeRandomSpec spec;
  spec.numActors  = 5;
  spec.numOptions = 7;
  spec.seed       = 11528563544ULL;
  const SaoeData data = SAOE::generate(spec);

  EXPECT_EQ(data.R.rows(), spec.numActors);
  EXPECT_EQ(data.R.cols(), spec.numOptions);
  EXPECT_EQ(data.S.size(), spec.numActors);

  const SAOE problem(data);
  const auto [vi, res] = problem.solve(SaoeParams{});
  (void)vi;
  EXPECT_EQ(res.e.rows(), spec.numActors);
  EXPECT_EQ(res.e.cols(), spec.numOptions);
  EXPECT_EQ(res.probabilities.size(), spec.numOptions);
  EXPECT_TRUE(feasibleP(res.e, data.S));
  EXPECT_NEAR(res.probabilities.sum(), 1.0, kProbSumTol);
}

TEST(SaoeProblem, GenerateRejectsBadSpec)
{
  SaoeRandomSpec bad;
  bad.numActors = 0;
  EXPECT_THROW(SAOE::generate(bad), std::invalid_argument);

  SaoeRandomSpec inverted;
  inverted.rewardLo = 10.0;
  inverted.rewardHi = -10.0;
  EXPECT_THROW(SAOE::generate(inverted), std::invalid_argument);
}
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------

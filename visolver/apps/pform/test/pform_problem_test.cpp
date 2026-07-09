// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
// Tests for the PForm parliament-formation class: the matching/count helpers,
// the SAOE solve, the eps-from-q derivation, and the deterministic guess.
// ----------------------------------------------
#include "pformproblem.hpp"

#include <gtest/gtest.h>

#include <cmath>
#include <cstdint>
#include <stdexcept>

using namespace VINCP;
using namespace VINCP::App;

namespace {

  constexpr std::uint64_t kSeed = 20260709ULL;
  constexpr double kProbSumTol = 1.0e-6;

  // A small instance (3 parties, 3 issues => K = 27) so the SAOE chain is quick.
  PformData
  smallInstance()
  {
    PformRandomSpec spec;
    spec.numParties = 3;
    spec.numIssues  = 3;
    spec.seed       = kSeed;
    return PForm::generate(spec);
  }

} // namespace

TEST(PformProblem, ParliamentCountAndMatching)
{
  EXPECT_EQ(pformParliamentCount(3, 4), 81);
  EXPECT_EQ(pformParliamentCount(2, 3), 8);

  // f_d = (k / M^d) mod M, issue 0 least significant.
  const std::vector<Index> f = pformMatching(3, 3, 4);
  ASSERT_EQ(f.size(), 4u);
  EXPECT_EQ(f[0], 0);   // 3 % 3
  EXPECT_EQ(f[1], 1);   // (3 / 3) % 3
  EXPECT_EQ(f[2], 0);
  EXPECT_EQ(f[3], 0);
}

TEST(PformProblem, RejectsDegenerateShapes)
{
  EXPECT_THROW(pformParliamentCount(1, 3), std::invalid_argument);   // < 2 parties
  EXPECT_THROW(pformParliamentCount(3, 0), std::invalid_argument);   // < 1 issue
  EXPECT_THROW(pformParliamentCount(20, 20), std::invalid_argument); // over the cap
  EXPECT_THROW(pformMatching(100, 3, 4), std::invalid_argument);     // k out of range
}

TEST(PformProblem, SolveConvergesAndIsPinned)
{
  const PForm problem(smallInstance());
  const auto [vi, res] = problem.solve(PformParams{});

  const Index K = pformParliamentCount(3, 3);
  EXPECT_TRUE(vi.converged);
  EXPECT_EQ(res.effort.rows(), 3);
  EXPECT_EQ(res.effort.cols(), K);
  EXPECT_EQ(res.probabilities.size(), K);
  EXPECT_NEAR(res.probabilities.sum(), 1.0, kProbSumTol);

  // The deterministic parliament is the argmax of (eta, phi, k), so its eta is
  // the maximum eta.
  EXPECT_GE(res.deterministic, 0);
  EXPECT_LT(res.deterministic, K);
  EXPECT_DOUBLE_EQ(res.eta(res.deterministic), res.eta.maxCoeff());
}

TEST(PformProblem, EpsilonMatchesUnselectedProb)
{
  const PformData data = smallInstance();
  const PForm problem(data);
  PformParams params;
  params.unselectedProb = 0.1;
  const auto [vi, res] = problem.solve(params);
  (void)vi;

  const double K = static_cast<double>(pformParliamentCount(3, 3));
  const double W = data.weight.sum();
  const double q = params.unselectedProb;
  const double expected = q * W / (K * (1.0 - q) - 1.0);
  EXPECT_NEAR(res.epsilon, expected, 1.0e-12);
  EXPECT_GT(res.epsilon, 0.0);
}

TEST(PformProblem, RejectsBadUnselectedProb)
{
  const PForm problem(smallInstance());
  PformParams tooLow;
  tooLow.unselectedProb = 0.0;
  EXPECT_THROW(problem.solve(tooLow), std::invalid_argument);

  PformParams tooHigh;
  tooHigh.unselectedProb = 0.999;   // >= (K-1)/K for K = 27
  EXPECT_THROW(problem.solve(tooHigh), std::invalid_argument);
}

TEST(PformProblem, RejectsUnsupportedEngine)
{
  const PForm problem(smallInstance());
  PformParams params;
  params.engine = ProblemBase::Engine::Ipm;   // SAOE honors only Chain / Auto
  EXPECT_THROW(problem.solve(params), std::invalid_argument);
}

TEST(PformProblem, RejectsLowSalienceColumn)
{
  PformData data = smallInstance();
  data.salience.col(0) *= 0.01;   // drive party 0's total salience below 1
  const PForm problem(data);
  EXPECT_THROW(problem.solve(PformParams{}), std::invalid_argument);
}
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------

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

TEST(PformProblem, RejectsAllZeroIssue)
{
  // Both party columns are valid (sum 5 >= 1), but issue 1 is salient to
  // nobody -- the row rule must reject that all-zero salience row.
  PformData data;
  data.weight = VectorXd::Constant(2, 1.0);
  data.position.resize(2, 2);
  data.position << 0.2, 0.8,
                   0.5, 0.5;
  data.salience.resize(2, 2);
  data.salience << 5.0, 5.0,
                   0.0, 0.0;
  EXPECT_THROW(validatePformData(data), std::invalid_argument);
}

TEST(PformProblem, GenerateCoversEveryRowAndColumn)
{
  for (const std::uint64_t seed : { 1ULL, 7ULL, 42ULL, 100ULL }) {
    PformRandomSpec spec;
    spec.numParties = 4;
    spec.numIssues  = 5;
    spec.seed       = seed;
    const PformData data = PForm::generate(spec);
    for (Index d = 0; d < data.position.rows(); ++d) {
      EXPECT_GT(data.salience.row(d).sum(), 0.0);   // no issue salient to nobody
    }
    for (Index m = 0; m < data.weight.size(); ++m) {
      EXPECT_GE(data.salience.col(m).sum(), 1.0);   // every party >= 1
    }
    EXPECT_NO_THROW(validatePformData(data));
  }
}

namespace {

  // Build a PformResult carrying just the effort and probabilities that the
  // coalition analysis reads; the other fields are irrelevant here.
  PformResult
  resultWith(const MatrixXd& effort, const VectorXd& probabilities)
  {
    PformResult r;
    r.effort        = effort;
    r.probabilities = probabilities;
    r.utilities     = VectorXd::Zero(effort.rows());
    r.eta           = VectorXd::Zero(effort.cols());
    r.phi           = VectorXd::Zero(effort.cols());
    r.deterministic = 0;
    r.epsilon       = 0.0;
    return r;
  }

} // namespace

TEST(PformProblem, CoalitionsGroupTwoClassesWithOneFreeIssue)
{
  // M = 2 parties, D = 2 issues => K = 4. Issue 0 is least significant, so
  // parliaments 0,1 share issue 1 = P0 (differ on issue 0) and 2,3 share
  // issue 1 = P1. Both parties fund each class; the classes differ in effort.
  MatrixXd e(2, 4);
  e << 0.30, 0.30, 0.10, 0.10,    // P0
       0.25, 0.25, 0.20, 0.20;    // P1
  VectorXd p(4);
  p << 0.30, 0.30, 0.20, 0.20;
  const auto coalitions = pformCoalitions(resultWith(e, p), 2, 2);

  ASSERT_EQ(coalitions.size(), 2u);

  // Sorted by probEach descending: the issue-1 = P0 class (prob 0.30) leads.
  const PformCoalition& x = coalitions[0];
  EXPECT_GT(x.probEach, coalitions[1].probEach);        // ordering
  EXPECT_EQ(x.members, (std::vector<Index>{ 0, 1 }));
  ASSERT_EQ(x.pattern.size(), 2u);
  EXPECT_EQ(x.pattern[0], kFreeIssue);                  // issue 0 varies
  EXPECT_EQ(x.pattern[1], 0);                           // issue 1 pinned to P0
  EXPECT_EQ(x.parliaments.size(), 2u);
  EXPECT_NEAR(x.probEach, 0.30, 1.0e-12);
  EXPECT_NEAR(x.probTotal, 0.60, 1.0e-12);
  EXPECT_TRUE(x.regularP);
  ASSERT_EQ(x.effortPer.size(), 2);
  EXPECT_NEAR(x.effortPer(0), 0.30, 1.0e-9);
  EXPECT_NEAR(x.effortPer(1), 0.25, 1.0e-9);

  const PformCoalition& y = coalitions[1];
  EXPECT_EQ(y.pattern[0], kFreeIssue);
  EXPECT_EQ(y.pattern[1], 1);                           // issue 1 pinned to P1
}

TEST(PformProblem, CoalitionSoloWithTwoFreeIssues)
{
  // M = 3, D = 3 => K = 27. Only P1 funds the 9 parliaments whose issue 2 is
  // controlled by P1 (issue 2 most significant: k in [9, 17]); issues 0 and 1
  // are then free, forming one solo coalition.
  const Index M = 3, D = 3, K = 27;
  MatrixXd e = MatrixXd::Zero(M, K);
  VectorXd p = VectorXd::Zero(K);
  for (Index k = 9; k < 18; ++k) {
    e(1, k) = 0.5;
    p(k)    = 1.0 / 9.0;
  }
  const auto coalitions = pformCoalitions(resultWith(e, p), M, D);

  ASSERT_EQ(coalitions.size(), 1u);
  const PformCoalition& c = coalitions[0];
  EXPECT_EQ(c.members, (std::vector<Index>{ 1 }));
  EXPECT_EQ(c.parliaments.size(), 9u);
  EXPECT_EQ(c.pattern[2], 1);                           // issue 2 pinned to P1
  int freeCount = 0;
  for (const Index q : c.pattern) {
    if (kFreeIssue == q) {
      ++freeCount;
    }
  }
  EXPECT_EQ(freeCount, 2);
  EXPECT_TRUE(c.regularP);                              // 9 = M^2, a full box
}

TEST(PformProblem, CoalitionFlagsIrregularClass)
{
  // M = 2, D = 2 => K = 4. P0 funds only k=0 ([P0 P0]) and k=3 ([P1 P1]) with
  // identical effort, so they group together; both issues vary, but a full
  // product on two free issues needs M^2 = 4 parliaments, not 2.
  MatrixXd e(2, 4);
  e << 0.5, 0.0, 0.0, 0.5,
       0.0, 0.0, 0.0, 0.0;
  VectorXd p(4);
  p << 0.5, 0.0, 0.0, 0.5;
  const auto coalitions = pformCoalitions(resultWith(e, p), 2, 2);

  ASSERT_EQ(coalitions.size(), 1u);
  const PformCoalition& c = coalitions[0];
  EXPECT_EQ(c.members, (std::vector<Index>{ 0 }));
  EXPECT_EQ(c.parliaments.size(), 2u);
  EXPECT_EQ(c.pattern[0], kFreeIssue);
  EXPECT_EQ(c.pattern[1], kFreeIssue);
  EXPECT_FALSE(c.regularP);
}

TEST(PformProblem, CoalitionsEmptyWhenNoSupport)
{
  MatrixXd e = MatrixXd::Zero(2, 4);
  VectorXd p = VectorXd::Constant(4, 0.25);
  const auto coalitions = pformCoalitions(resultWith(e, p), 2, 2);
  EXPECT_TRUE(coalitions.empty());
}

TEST(PformProblem, CoalitionsRejectDimensionMismatch)
{
  MatrixXd e = MatrixXd::Zero(2, 4);
  VectorXd p = VectorXd::Constant(4, 0.25);
  EXPECT_THROW(pformCoalitions(resultWith(e, p), 3, 2), std::invalid_argument);
  EXPECT_THROW(pformCoalitions(resultWith(e, p), 2, 3), std::invalid_argument);
}
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------

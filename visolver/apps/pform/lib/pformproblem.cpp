// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
// PForm implementation: build the parliament utility matrix, derive the SAOE
// effort floor from the unselected-probability knob, solve, and package the
// supports with the deterministic Central-Position guess.
// ----------------------------------------------
#include "pformproblem.hpp"

#include <cmath>
#include <random>
#include <stdexcept>
#include <tuple>
#include <utility>

namespace VINCP::App {

  namespace {
    // K = M^D must stay small enough that the SAOE problem (K*M + M variables)
    // is tractable; larger instances are a modeling error, not a solve to
    // attempt.
    constexpr Index kMaxParliaments = 8192;
    constexpr double kSalienceSumTol = 1.0e-9;
  } // namespace

  Index
  pformParliamentCount(Index numParties, Index numIssues)
  {
    if (numParties < 2) {
      throw std::invalid_argument("PForm: need at least 2 parties.");
    }
    if (numIssues < 1) {
      throw std::invalid_argument("PForm: need at least 1 issue.");
    }
    Index count = 1;
    for (Index d = 0; d < numIssues; ++d) {
      if (count > kMaxParliaments / numParties) {
        throw std::invalid_argument(
            "PForm: K = M^D exceeds the tractable cap; reduce parties or issues.");
      }
      count *= numParties;
    }
    return count;
  }

  vector<Index>
  pformMatching(Index k, Index numParties, Index numIssues)
  {
    const Index count = pformParliamentCount(numParties, numIssues);
    if (k < 0 || count <= k) {
      throw std::invalid_argument("PForm: parliament index out of range.");
    }
    vector<Index> f(static_cast<size_t>(numIssues));
    Index rem = k;
    for (Index d = 0; d < numIssues; ++d) {
      f[static_cast<size_t>(d)] = rem % numParties;
      rem /= numParties;
    }
    return f;
  }

  PForm::PForm(PformData instanceData)
    : data(std::move(instanceData))
  {
    return;
  }

  Index
  PForm::numParties() const
  {
    return data.weight.size();
  }

  Index
  PForm::numIssues() const
  {
    return data.position.rows();
  }

  PformData
  PForm::generate(const PformRandomSpec& spec)
  {
    if (spec.numParties < 2) {
      throw std::invalid_argument("PForm::generate: need at least 2 parties.");
    }
    if (spec.numIssues < 1) {
      throw std::invalid_argument("PForm::generate: need at least 1 issue.");
    }
    if (spec.weightHi < spec.weightLo) {
      throw std::invalid_argument("PForm::generate: inverted weight range.");
    }

    const Index M = spec.numParties;
    const Index D = spec.numIssues;
    std::mt19937_64 rng(spec.seed);
    std::uniform_real_distribution<double> unit(0.0, 1.0);
    std::uniform_real_distribution<double> weightDist(spec.weightLo, spec.weightHi);
    std::uniform_real_distribution<double> totalSalDist(1.0, 2.0);

    PformData out;
    out.weight.resize(M);
    for (Index m = 0; m < M; ++m) {
      out.weight(m) = weightDist(rng);
    }
    out.position.resize(D, M);
    for (Index d = 0; d < D; ++d) {
      for (Index m = 0; m < M; ++m) {
        out.position(d, m) = unit(rng);
      }
    }
    // Saliences: strictly positive draws, then each party's column scaled so its
    // total salience lands in [1, 2] (satisfies sum_d S_dm >= 1).
    out.salience.resize(D, M);
    for (Index m = 0; m < M; ++m) {
      double colSum = 0.0;
      for (Index d = 0; d < D; ++d) {
        const double s = 0.05 + 0.95 * unit(rng);   // in [0.05, 1.0]
        out.salience(d, m) = s;
        colSum += s;
      }
      const double scale = totalSalDist(rng) / colSum;
      for (Index d = 0; d < D; ++d) {
        out.salience(d, m) *= scale;
      }
    }
    return out;
  }

  PForm::Solution
  PForm::solve(const Params& params) const
  {
    const Index M = data.weight.size();
    const Index D = data.position.rows();
    if (M < 2) {
      throw std::invalid_argument("PForm: need at least 2 parties.");
    }
    if (D < 1) {
      throw std::invalid_argument("PForm: need at least 1 issue.");
    }
    if (data.position.cols() != M || data.salience.rows() != D
        || data.salience.cols() != M) {
      throw std::invalid_argument(
          "PForm: position and salience must be D x M, matching weight's M.");
    }

    for (Index m = 0; m < M; ++m) {
      if (!(0.0 < data.weight(m))) {
        throw std::invalid_argument("PForm: every weight must be positive.");
      }
    }
    for (Index d = 0; d < D; ++d) {
      for (Index m = 0; m < M; ++m) {
        const double p = data.position(d, m);
        if (p < 0.0 || 1.0 < p) {
          throw std::invalid_argument("PForm: positions must lie in [0, 1].");
        }
        if (data.salience(d, m) < 0.0) {
          throw std::invalid_argument("PForm: saliences must be non-negative.");
        }
      }
    }
    VectorXd sumS(M);
    for (Index m = 0; m < M; ++m) {
      const double s = data.salience.col(m).sum();
      if (s < 1.0 - kSalienceSumTol) {
        throw std::invalid_argument(
            "PForm: each party's total salience sum_d S_dm must be >= 1.");
      }
      sumS(m) = s;
    }

    const Index K = pformParliamentCount(M, D);

    // Reward R(m, k) = U_km, plus eta_k (quadratic) and phi_k (linear) weights.
    MatrixXd R(M, K);
    VectorXd eta = VectorXd::Zero(K);
    VectorXd phi = VectorXd::Zero(K);
    VectorXd composite(D);
    for (Index k = 0; k < K; ++k) {
      Index rem = k;
      for (Index d = 0; d < D; ++d) {
        const Index controller = rem % M;
        rem /= M;
        composite(d) = data.position(d, controller);
      }
      for (Index m = 0; m < M; ++m) {
        double quad = 0.0;
        double absVal = 0.0;
        for (Index d = 0; d < D; ++d) {
          const double diff = data.position(d, m) - composite(d);
          const double s = data.salience(d, m);
          quad += s * diff * diff;
          absVal += s * std::abs(diff);
        }
        const double utility = 1.0 - quad / sumS(m);
        const double value = 1.0 - absVal / sumS(m);
        R(m, k) = utility;
        eta(k) += data.weight(m) * utility;
        phi(k) += data.weight(m) * value;
      }
    }

    // Deterministic Central-Position guess: the parliament first in the
    // descending lexicographic order of (eta_k, phi_k, k) -- i.e. the maximum
    // triple, with k the final (largest-wins) tie-breaker.
    Index deterministic = 0;
    for (Index k = 1; k < K; ++k) {
      if (std::make_tuple(eta(k), phi(k), k)
          > std::make_tuple(eta(deterministic), phi(deterministic), deterministic)) {
        deterministic = k;
      }
    }

    // Effort floor from the unselected-probability knob q (eps is not an input).
    const double q = params.unselectedProb;
    const double kd = static_cast<double>(K);
    const double qMax = (kd - 1.0) / kd;
    if (!(0.0 < q && q < qMax)) {
      throw std::invalid_argument(
          "PForm: unselectedProb q must satisfy 0 < q < (K-1)/K.");
    }
    const double W = data.weight.sum();
    const double epsilon = q * W / (kd * (1.0 - q) - 1.0);

    // Solve the SAOE Nash equilibrium of the utility matrix (risk aversion is
    // already in the quadratic utility, so the inner SAOE is risk-neutral).
    SaoeParams saoeParams;
    saoeParams.engine       = params.engine;
    saoeParams.riskAversion = 0.0;
    saoeParams.epsilon      = epsilon;
    saoeParams.verbose      = params.verbose;
    const SAOE saoe(SaoeData{ R, data.weight });
    const auto [vi, saoeResult] = saoe.solve(saoeParams);

    PformResult result;
    result.effort        = saoeResult.e;
    result.probabilities = saoeResult.probabilities;
    result.utilities     = saoeResult.utilities;
    result.eta           = eta;
    result.phi           = phi;
    result.deterministic = deterministic;
    result.epsilon       = epsilon;

    return Solution{ vi, result };
  }

} // namespace VINCP::App
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------

// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
// Armijo backtracking line search implementation.
// ----------------------------------------------
#include "armijo.hpp"

#include <stdexcept>
#include <string>

namespace VIMCP {

  namespace {

    void
    validateSearchInputs(const char* who,
                         const function<double(double)>& meritAt,
                         const ArmijoParams& params)
    {
      if (!meritAt) {
        throw std::invalid_argument(std::string(who) + ": meritAt must be set.");
      }
      if (!(0.0 < params.shrink && params.shrink < 1.0)) {
        throw std::invalid_argument(std::string(who) + ": shrink must lie in (0, 1).");
      }
      if (!(0.0 < params.sufficientDecrease && params.sufficientDecrease < 1.0)) {
        throw std::invalid_argument(std::string(who) + ": sufficientDecrease must lie in (0, 1).");
      }
      if (!(0.0 < params.alpha0)) {
        throw std::invalid_argument(std::string(who) + ": alpha0 must be positive.");
      }
      return;
    }

  } // namespace

  ArmijoResult
  armijoLineSearch(const function<double(double)>& meritAt,
                   double merit0,
                   const ArmijoParams& params)
  {
    validateSearchInputs("armijoLineSearch", meritAt, params);

    double alpha = params.alpha0;
    double bestAlpha = alpha;
    double bestMerit = meritAt(alpha);
    int backtracks = 0;

    if (bestMerit <= (1.0 - params.sufficientDecrease * alpha) * merit0) {
      return ArmijoResult{ alpha, bestMerit, backtracks, true };
    }

    for (int i = 0; i < params.maxBacktracks; ++i) {
      alpha *= params.shrink;
      const double m = meritAt(alpha);
      ++backtracks;
      if (m < bestMerit) {
        bestMerit = m;
        bestAlpha = alpha;
      }
      if (m <= (1.0 - params.sufficientDecrease * alpha) * merit0) {
        return ArmijoResult{ alpha, m, backtracks, true };
      }
    }

    // No sufficient-decrease step found; report the best tried, not accepted.
    return ArmijoResult{ bestAlpha, bestMerit, backtracks, false };
  }

  ArmijoResult
  armijoLineSearchDirectional(const function<double(double)>& meritAt,
                              double merit0,
                              double slope0,
                              const ArmijoParams& params)
  {
    validateSearchInputs("armijoLineSearchDirectional", meritAt, params);
    if (!(slope0 < 0.0)) {
      throw std::invalid_argument(
          "armijoLineSearchDirectional: slope0 must be negative (descent direction).");
    }

    double alpha = params.alpha0;
    double bestAlpha = alpha;
    double bestMerit = meritAt(alpha);
    int backtracks = 0;

    if (bestMerit <= merit0 + params.sufficientDecrease * alpha * slope0) {
      return ArmijoResult{ alpha, bestMerit, backtracks, true };
    }

    for (int i = 0; i < params.maxBacktracks; ++i) {
      alpha *= params.shrink;
      const double m = meritAt(alpha);
      ++backtracks;
      if (m < bestMerit) {
        bestMerit = m;
        bestAlpha = alpha;
      }
      if (m <= merit0 + params.sufficientDecrease * alpha * slope0) {
        return ArmijoResult{ alpha, m, backtracks, true };
      }
    }

    // No sufficient-decrease step found; report the best tried, not accepted.
    return ArmijoResult{ bestAlpha, bestMerit, backtracks, false };
  }

} // namespace VIMCP
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------

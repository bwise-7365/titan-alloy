// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
// Armijo backtracking line search implementation.
// ----------------------------------------------
#include "armijo.hpp"

#include <stdexcept>

namespace VINCP {

  ArmijoResult
  armijoLineSearch(const function<double(double)>& meritAt,
                   double merit0,
                   const ArmijoParams& params)
  {
    if (!meritAt) {
      throw std::invalid_argument("armijoLineSearch: meritAt must be set.");
    }
    if (!(0.0 < params.shrink && params.shrink < 1.0)) {
      throw std::invalid_argument("armijoLineSearch: shrink must lie in (0, 1).");
    }
    if (!(0.0 < params.sufficientDecrease && params.sufficientDecrease < 1.0)) {
      throw std::invalid_argument("armijoLineSearch: sufficientDecrease must lie in (0, 1).");
    }
    if (!(0.0 < params.alpha0)) {
      throw std::invalid_argument("armijoLineSearch: alpha0 must be positive.");
    }

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

} // namespace VINCP
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------

// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
// Armijo backtracking line search: a reusable globalization building block.
// ----------------------------------------------
#ifndef VINCP_ARMIJO_HPP
#define VINCP_ARMIJO_HPP

// Given a base merit value and a way to evaluate the merit at step length alpha
// along a caller-fixed direction, it shrinks alpha until a sufficient decrease
// is met. It is decoupled from any particular solver, direction, or merit, so
// several algorithms can share it.

#include <functional>

using std::function;

namespace VINCP {

  struct ArmijoParams {
    double alpha0             = 1.0;      // initial step length (the full step)
    double shrink             = 0.5;     // backtracking factor in (0, 1)
    double sufficientDecrease = 1.0e-4;  // c in (0, 1) in the acceptance test
    int    maxBacktracks      = 40;      // cap on shrink steps
  };

  struct ArmijoResult {
    double alpha      = 0.0;    // accepted (or, on failure, best) step length
    double merit      = 0.0;    // merit at that step length
    int    backtracks = 0;      // number of shrink steps taken
    bool   accepted   = false;  // whether sufficient decrease was met
  };

  // Backtracking line search using the value-only sufficient-decrease test
  //     phi(alpha) <= (1 - c*alpha) * phi0,
  // which needs no gradient and is valid for a descent direction. 'meritAt(alpha)'
  // returns the merit phi at step length alpha along the (caller-fixed) direction;
  // 'merit0' is phi(0) at the base point. If no step satisfies the test within
  // maxBacktracks, returns the best (lowest-merit) alpha tried with accepted=false
  // -- the caller decides what to do, rather than a value being silently forced.
  //
  // Throws std::invalid_argument on an unset meritAt or out-of-range parameters.
  ArmijoResult armijoLineSearch(const function<double(double)>& meritAt,
                                double merit0,
                                const ArmijoParams& params = ArmijoParams{});

} // namespace VINCP

#endif // VINCP_ARMIJO_HPP
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------

// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
// App-layer Problem template: a uniform (params -> solve -> result) shell that
// wraps the VINCP solver library into one reusable class per problem kind.
// ----------------------------------------------
#ifndef VINCP_APPS_PROBLEM_HPP
#define VINCP_APPS_PROBLEM_HPP

// The application framework sits ABOVE the solver library (namespace VINCP) and
// changes nothing in it: each concrete problem (SAOE, and later Fleet) derives
// from Problem, wraps the existing library entry points, and exposes a single
// shape:
//     create a problem instance (holding its DATA),
//     call solve(params),
//     receive a tuple (VIResult, Result).
// The VIResult half carries the raw solver telemetry (converged / residual /
// iters); the Result half is the decoded, domain-specific answer.
//
// DATA is deliberately NOT part of this contract: its shape differs per problem
// (SAOE has a reward matrix and strength vector; Fleet has a network), so it
// enters through the derived class's own constructor. What every problem shares
// is Params, Result, the Engine menu, and solve -- and those are what this
// template fixes.

#include "vincp.hpp"

#include <tuple>

namespace VINCP::App {

  // The solver-engine menu shared by every problem. It is a single, non-template
  // type (living in this non-template base) so that SAOE::Engine, Fleet::Engine,
  // and Problem<A, B>::Engine all name the SAME enum -- a problem-independent
  // vocabulary. Not every engine suits every problem: each Problem documents the
  // subset it honors and throws std::invalid_argument on an unsupported choice.
  // Engine::Default means "let the problem pick its own robust default".
  class ProblemBase {
  public:
    enum class Engine {
      Default,   // the problem's own robust default
      Chain,     // alternating globalizer/finisher chain (SAOE)
      Auto,      // chooseEngine dispatcher (SAOE)
      Ipm,       // Mehrotra interior point (Fleet)
      Bshe94b,   // He 1994 projection-contraction (Fleet)
      Ssn        // semismooth Newton (Fleet)
    };

  protected:
    ProblemBase() = default;
    ~ProblemBase() = default;

  private:
  };

  // The reusable problem shell. ParamsT and ResultT are supplied by the derived
  // class (as its own nested structs) and re-exposed here as Params / Result, so
  // callers write SAOE::Params / SAOE::Result. They are template TYPE PARAMETERS
  // rather than CRTP-derived members to avoid the incomplete-type trap (the base
  // would otherwise need the derived class fully defined to name its nested
  // types).
  template <class ParamsT, class ResultT>
  class Problem : public ProblemBase {
  public:
    using Params   = ParamsT;
    using Result   = ResultT;
    using Solution = std::tuple<VIResult, Result>;   // (raw solver, decoded answer)

    virtual ~Problem() = default;

    // Solve the instance under 'params' and return (VIResult, Result). Concrete
    // problems throw std::invalid_argument on an unsupported engine or bad
    // params, and propagate the solver library's own std::runtime_error on a
    // detected divergence / non-finite value.
    virtual Solution solve(const Params& params) const = 0;

  protected:
    Problem() = default;

  private:
  };

} // namespace VINCP::App

#endif // VINCP_APPS_PROBLEM_HPP
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------

// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
// App-layer Problem template: a uniform (params -> solve -> result) shell that
// wraps the VIMCP solver library into one reusable class per problem kind.
// ----------------------------------------------
#ifndef VIMCP_APPS_PROBLEM_HPP
#define VIMCP_APPS_PROBLEM_HPP

// The application framework sits ABOVE the solver library (namespace VIMCP) and
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

#include "vimcp.hpp"

#include <algorithm>
#include <cstddef>
#include <stdexcept>
#include <string>
#include <tuple>
#include <vector>

namespace VIMCP::App {

  // The solver-engine menu shared by every problem. It is a single, non-template
  // type (living in this non-template base) so that SAOE::Engine, Fleet::Engine,
  // and Problem<A, B>::Engine all name the SAME enum -- a problem-independent
  // vocabulary. Not every engine suits every problem: each Problem documents the
  // subset it honors and throws std::invalid_argument on an unsupported choice.
  // Engine::Default means "let the problem pick its own robust default".
  //
  // The parenthetical after each member below names the problem class(es) that
  // currently HONOR it (dispatch on it in their solve) -- NOT the limit of where
  // the method could mathematically apply. E.g. Ssn is tagged (Fleet) because
  // Fleet exposes it as a bare engine, yet semismooth Newton also runs inside
  // SAOE's chain; it is just not offered as a standalone SAOE engine. Add a tag
  // when you wire an engine into a problem's dispatch.
  class ProblemBase {
  public:
    enum class Engine {
      Default,          // the problem's own robust default
      Chain,            // alternating globalizer/finisher chain (SAOE)
      Auto,             // chooseEngine dispatcher (SAOE)
      Ipm,              // Mehrotra interior point (Fleet)
      Bshe94b,          // He 1994 projection-contraction (Fleet)
      Ssn,              // semismooth Newton (Fleet)
      SmoothingNewton,  // non-interior smoothing, Zhang-Liu-Liu (SAOE/pform)
      Fbs               // forward-backward splitting, He-Yuan-Zhang 2004 (SAOE/pform)
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

    // Drive a result to a SPARSE representative -- a vertex of the set of
    // solutions equivalent to it -- WITHOUT changing its pinned/invariant
    // quantities (e.g. fleet: consolidate flow arcs, deliveries unchanged; SAOE:
    // concentrate the non-unique effort attribution, probabilities unchanged).
    // PURE VIRTUAL by design: every solver author must consciously choose. If the
    // solver already follows edges / returns a unique result, write the trivial
    // pass-through `return result;`. If it does not (an interior-point / spread
    // solution), write the real consolidation.
    virtual Result sparsify(const Result& result) const = 0;

  protected:
    Problem() = default;

  private:
  };

  // ---------------------------------------------------------------------------
  // Engine metadata -- the single source of truth for engine names/tokens.
  // Each Problem separately declares WHICH engines it honors (see e.g.
  // SAOE::honoredEngines); these free functions only describe and parse the
  // shared vocabulary, so no app re-hardcodes it. The switches deliberately have
  // no default: a new Engine value makes the compiler flag every switch here.
  // ---------------------------------------------------------------------------

  // A short, stable CLI/config token for each engine (lower-case).
  inline const char*
  engineToken(ProblemBase::Engine engine)
  {
    switch (engine) {
      case ProblemBase::Engine::Default:         return "default";
      case ProblemBase::Engine::Chain:           return "chain";
      case ProblemBase::Engine::Auto:            return "auto";
      case ProblemBase::Engine::Ipm:             return "ipm";
      case ProblemBase::Engine::Bshe94b:         return "bshe94b";
      case ProblemBase::Engine::Ssn:             return "ssn";
      case ProblemBase::Engine::SmoothingNewton: return "smoothing";
      case ProblemBase::Engine::Fbs:             return "fbs";
    }
    return "?";   // unreachable (switch is exhaustive); satisfies -Wreturn-type
  }

  // A human-readable name/description for each engine.
  inline const char*
  engineName(ProblemBase::Engine engine)
  {
    switch (engine) {
      case ProblemBase::Engine::Default:
        return "Default (the problem's own robust default)";
      case ProblemBase::Engine::Chain:
        return "Chain (alternating globalizer/finisher)";
      case ProblemBase::Engine::Auto:
        return "Auto (chooseEngine: semismooth Newton, chain fallback)";
      case ProblemBase::Engine::Ipm:
        return "Ipm (Mehrotra interior point)";
      case ProblemBase::Engine::Bshe94b:
        return "Bshe94b (He 1994 projection-contraction)";
      case ProblemBase::Engine::Ssn:
        return "Ssn (semismooth Newton)";
      case ProblemBase::Engine::SmoothingNewton:
        return "SmoothingNewton (non-interior smoothing, Zhang-Liu-Liu)";
      case ProblemBase::Engine::Fbs:
        return "Fbs (forward-backward splitting, He-Yuan-Zhang 2004)";
    }
    return "?";
  }

  // Parse an engine token (as engineToken produces, plus the "smoothingnewton"
  // synonym). Throws std::invalid_argument naming the offender on an unknown one.
  inline ProblemBase::Engine
  parseEngineToken(const std::string& token)
  {
    for (const ProblemBase::Engine e :
         { ProblemBase::Engine::Default, ProblemBase::Engine::Chain,
           ProblemBase::Engine::Auto, ProblemBase::Engine::Ipm,
           ProblemBase::Engine::Bshe94b, ProblemBase::Engine::Ssn,
           ProblemBase::Engine::SmoothingNewton, ProblemBase::Engine::Fbs }) {
      if (token == engineToken(e)) {
        return e;
      }
    }
//    if ("smoothingnewton" == token) {
//      return ProblemBase::Engine::SmoothingNewton;
//    }
    throw std::invalid_argument("unknown engine token '" + token + "'.");
  }

  // "chain|auto|smoothing|fbs" from a list of engines, for help / error text.
  inline std::string
  engineTokenList(const std::vector<ProblemBase::Engine>& engines)
  {
    std::string out;
    for (std::size_t i = 0; i < engines.size(); ++i) {
      if (0 < i) {
        out += "|";
      }
      out += engineToken(engines[i]);
    }
    return out;
  }

  // Whether 'engine' is in 'honored' (Engine::Default is universally accepted --
  // it resolves to each problem's own default).
  inline bool
  engineIsHonored(const std::vector<ProblemBase::Engine>& honored,
                  ProblemBase::Engine engine)
  {
    return ProblemBase::Engine::Default == engine
        || std::find(honored.begin(), honored.end(), engine) != honored.end();
  }

} // namespace VIMCP::App

#endif // VIMCP_APPS_PROBLEM_HPP
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------

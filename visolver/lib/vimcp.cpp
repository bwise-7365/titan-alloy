// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
// Core domain types: projections, model evaluation, and shared input validation.
// ----------------------------------------------
#include "vimcp.hpp"

#include <memory>
#include <stdexcept>
#include <string>
#include <utility>

using std::string;

namespace VIMCP {

  VectorXd
  projectNonnegative(const VectorXd& v)
  {
    return v.cwiseMax(0.0);
  }

  Projector
  makeMixedProjector(Index numFree)
  {
    if (0 > numFree) {
      throw std::invalid_argument("makeMixedProjector: numFree must be non-negative.");
    }
    return [numFree](const VectorXd& v) -> VectorXd {
      if (numFree > v.size()) {
        throw std::invalid_argument("mixed projector: numFree exceeds vector length.");
      }
      VectorXd out = v;
      const Index tail = v.size() - numFree;
      out.tail(tail) = v.tail(tail).cwiseMax(0.0);
      return out;
    };
  }

  VectorXd
  evaluateF(const VIModel& model, const VectorXd& z)
  {
    if (!model.H || !model.G) {
      throw std::invalid_argument("evaluateF: model.H and model.G must be set.");
    }
    if (z.size() != model.n + model.m) {
      throw std::invalid_argument("evaluateF: z length must equal n + m.");
    }

    const VectorXd x = z.head(model.n);
    const VectorXd y = z.tail(model.m);

    const VectorXd h = model.H(x, y);
    const VectorXd g = model.G(x, y);

    if (h.size() != model.n) {
      throw std::runtime_error("evaluateF: H returned the wrong length.");
    }
    if (g.size() != model.m) {
      throw std::runtime_error("evaluateF: G returned the wrong length.");
    }

    VectorXd f(model.n + model.m);
    f.head(model.n) = h;
    f.tail(model.m) = g;

    if (!f.allFinite()) {
      throw std::runtime_error("evaluateF: F(z) is non-finite.");
    }
    return f;
  }

  VIModel
  makeVIModel(Index n, Index m, function<VectorXd(const VectorXd&)> F)
  {
    VIModel model;
    model.n = n;
    model.m = m;
    // H and G share a one-deep memo of (z, F(z)). evaluateF calls H(x, y) then
    // G(x, y) with the SAME (x, y), so the caller's field F is evaluated ONCE per z
    // rather than twice (once for each block). Single-threaded use only (the solvers
    // run serially), which the whole library already assumes.
    auto cache = std::make_shared<std::pair<VectorXd, VectorXd>>();
    const auto evalF = [n, m, F, cache](const VectorXd& x, const VectorXd& y) -> const VectorXd& {
      VectorXd z(n + m);
      z << x, y;
      if (cache->first.size() != z.size() || !(cache->first.array() == z.array()).all()) {
        cache->first = z;
        cache->second = F(z);
      }
      return cache->second;
    };
    model.H = [n, evalF](const VectorXd& x, const VectorXd& y) -> VectorXd {
      return evalF(x, y).head(n);
    };
    model.G = [m, evalF](const VectorXd& x, const VectorXd& y) -> VectorXd {
      return evalF(x, y).tail(m);
    };
    return model;
  }

  void
  validateLviInputs(const char* who,
                    const VectorXd& x0, const MatrixXd& M,
                    const VectorXd& q, const Projector& Pr)
  {
    const Index nd = x0.size();
    if (0 >= nd) {
      throw std::invalid_argument(string(who) + ": x0 must be non-empty.");
    }
    if (M.rows() != nd || M.cols() != nd) {
      throw std::invalid_argument(string(who) + ": M must be square and conformant with x0.");
    }
    if (q.size() != nd) {
      throw std::invalid_argument(string(who) + ": q must be conformant with x0.");
    }
    if (!Pr) {
      throw std::invalid_argument(string(who) + ": projector Pr must be set.");
    }
    return;
  }

} // namespace VIMCP
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------

// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
// Smoothing solver: phi functions, the reformulation, and the continuation variant.
// ----------------------------------------------
// "A non-interior-point smoothing method for variational inequality problem"
// by Zhang, Liu, Liu in Journal of Computational and Applied Mathematics 234 (2010)
//
// Their goal was to minimize scalar f(x) s.t. G(x) >= 0 which can be solved as
//
//     grad f(x)  =  lambda grad g(x)
//     0 <= lambda  _|_  g(x) >= 0
//
// As grad f(x) is a row vector, they transpose the top line to use column vectors.
// The auxiliary vector 's' is used to decouple the two uses of 'G'.
// ------------------------
// I realized that this could probably be generalized using
// the same 's',
//
// H(x,y) = grad f(x) - y grad g(x) and
// G(x,y) = g(x)
//
// like this:
//
//         0 = H(x,y)
//     0 <= y _|_ G(x,y) >= 0
//
// Note that y is constrained explicitly and x is constrained implicitly by H and G.
// I would suggest z = [u, x, y, s] and psi(z) is the following:
//
//          u
//       H(x,y)
//     s - G(x,y)
//     FB(u, y, s)
//
// Notice that if x has N dim, H has K, and y and G have M,
// then the input vector is 1+N+2M (u, x, y, s), while the output is 1+K+2M.
// That is why it is important that my damped Newton method allows
// the input and output vectors to have different dimensionality.
#include "smoothingnewton.hpp"

#include <algorithm>
#include <stdexcept>

namespace VINCP {

  VectorXd
  smoothedFischerBurmeister(double u, const VectorXd& a, const VectorXd& b)
  {
    if (a.size() != b.size()) {
      throw std::invalid_argument("smoothedFischerBurmeister: a and b must have equal length.");
    }
    // phi_i = a_i + b_i - sqrt(a_i^2 + b_i^2 + u^2); at u = 0 this is exact FB.
    return (a.array() + b.array()
            - (a.array().square() + b.array().square() + u * u).sqrt())
        .matrix();
  }

  VectorXd
  smoothedFB_WZ(double u, const VectorXd& a, const VectorXd& b)
  {
    if (a.size() != b.size()) {
      throw std::invalid_argument("smoothedFB_WZ: a and b must have equal length.");
    }
    // Wu & Zhao (2013), eq. (2):
    //   phi_i = a_i + b_i
    //     - sqrt( (a_i - u^2(a_i - b_i))^2 + (b_i + u^2(a_i - b_i))^2 + u^2 ).
    // At u = 0 the u^2(a-b) terms drop and this becomes exact Fischer-Burmeister.
    const double u2 = u * u;
    const VectorXd d  = a - b;          // a - b
    const VectorXd t1 = a - u2 * d;     // a - u^2 (a - b)
    const VectorXd t2 = b + u2 * d;     // b + u^2 (a - b)
    return (a.array() + b.array()
            - (t1.array().square() + t2.array().square() + u2).sqrt())
        .matrix();
  }

  // Uses Damped Newton (Levenberg Marquardt) to solve
  //
  //         0 = H(x,y)
  //     0 <= y _|_ G(x,y) >= 0
  // as
  //       H(x,y)
  //     s - G(x,y)
  //     FB(u, y, s)
  VIResult
  smoothingNewtonSolve(const MixedField& H, const MixedField& G,
                       const VectorXd& x0, const VectorXd& y0,
                       const SmoothingNewtonParams& params)
  {
    if (!H || !G) {
      throw std::invalid_argument("smoothingNewtonSolve: H and G must be set.");
    }
    if (0 >= x0.size()) {
      throw std::invalid_argument("smoothingNewtonSolve: x0 must be non-empty.");
    }
    if (0 >= y0.size()) {
      throw std::invalid_argument("smoothingNewtonSolve: y0 must be non-empty.");
    }
    if (!(0.0 < params.u0)) {
      throw std::invalid_argument("smoothingNewtonSolve: u0 must be positive.");
    }
    if (!params.smoothing) {
      throw std::invalid_argument("smoothingNewtonSolve: smoothing function must be set.");
    }

    const Index N = x0.size();
    const Index M = y0.size();

    // Slack starts at s0 = G(x0, y0) (so the s - G(x,y) block starts at 0). G must
    // return one component per multiplier for the complementarity to be well posed.
    const VectorXd s0 = G(x0, y0);
    if (s0.size() != M) {
      throw std::invalid_argument("smoothingNewtonSolve: G(x0, y0) length must equal y0 length (M).");
    }

    // Assemble z0 = [u0, x0, y0, s0] in R^(1 + N + 2M).
    VectorXd z0(1 + N + 2 * M);
    z0(0) = params.u0;
    z0.segment(1, N) = x0;
    z0.segment(1 + N, M) = y0;
    z0.segment(1 + N + M, M) = s0;

    // Stacked residual F(z) = [ u ; H(x,y) ; s - G(x,y) ; phi(u, y, s) ]. Captured by
    // value so it stays valid for the duration of the solve. K = H's output length is
    // read per call (dampedNewton's FD Jacobian allows the output dim to differ from N).
    const SmoothingFunction phiFn = params.smoothing;
    const VectorField F = [H, G, phiFn, N, M](const VectorXd& z) -> VectorXd {
      const double   u = z(0);
      const VectorXd x = z.segment(1, N);
      const VectorXd y = z.segment(1 + N, M);
      const VectorXd s = z.segment(1 + N + M, M);

      const VectorXd Hxy = H(x, y);          // K
      const VectorXd Gxy = G(x, y);          // M
      const VectorXd phi = phiFn(u, y, s);   // M
      const Index K = Hxy.size();

      VectorXd out(1 + K + 2 * M);
      out(0) = u;
      out.segment(1, K) = Hxy;
      out.segment(1 + K, M) = s - Gxy;
      out.segment(1 + K + M, M) = phi;
      return out;
    };

    return dampedNewtonSolve(F, z0, params.damped);
  }

  SmoothingSolution
  smoothingDecode(const VIResult& r, Index N, Index M)
  {
    // Mirror the z = [u, x, y, s] packing assembled above.
    SmoothingSolution sol;
    sol.u = r.z(0);
    sol.x = r.z.segment(1, N);
    sol.y = r.z.segment(1 + N, M);
    sol.s = r.z.segment(1 + N + M, M);
    return sol;
  }

  VIResult
  smoothingContinuationSolve(const MixedField& H, const MixedField& G,
                             const VectorXd& x0, const VectorXd& y0,
                             const SmoothingContinuationParams& params)
  {
    if (!H || !G) {
      throw std::invalid_argument("smoothingContinuationSolve: H and G must be set.");
    }
    if (0 >= x0.size()) {
      throw std::invalid_argument("smoothingContinuationSolve: x0 must be non-empty.");
    }
    if (0 >= y0.size()) {
      throw std::invalid_argument("smoothingContinuationSolve: y0 must be non-empty.");
    }
    if (!params.smoothing) {
      throw std::invalid_argument("smoothingContinuationSolve: smoothing function must be set.");
    }
    if (!(0.0 < params.u0)) {
      throw std::invalid_argument("smoothingContinuationSolve: u0 must be positive.");
    }
    if (!(0.0 < params.sigma && params.sigma < 1.0)) {
      throw std::invalid_argument("smoothingContinuationSolve: sigma must lie in (0, 1).");
    }
    if (!(0.0 < params.muMin)) {
      throw std::invalid_argument("smoothingContinuationSolve: muMin must be positive.");
    }
    if (params.muMin > params.u0) {
      throw std::invalid_argument("smoothingContinuationSolve: muMin must not exceed u0.");
    }
    if (0 >= params.maxOuter) {
      throw std::invalid_argument("smoothingContinuationSolve: maxOuter must be positive.");
    }
    if (!(0.0 < params.outerTol)) {
      throw std::invalid_argument("smoothingContinuationSolve: outerTol must be positive.");
    }

    const Index N = x0.size();
    const Index M = y0.size();

    // Slack starts at s0 = G(x0, y0) (so the s - G(x,y) block starts at 0). G must
    // return one component per multiplier for the complementarity to be well posed.
    const VectorXd s0 = G(x0, y0);
    if (s0.size() != M) {
      throw std::invalid_argument("smoothingContinuationSolve: G(x0, y0) length must equal y0 length (M).");
    }

    // Working vector w = [x, y, s] in R^(N + 2M). u is NOT part of w -- it is the
    // outer parameter mu, held fixed within each inner solve.
    VectorXd w(N + 2 * M);
    w.segment(0, N) = x0;
    w.segment(N, M) = y0;
    w.segment(N + M, M) = s0;

    const SmoothingFunction phiFn = params.smoothing;

    // Build the rectangular residual F_mu(w) = [ H(x,y) ; s - G(x,y) ; phi(mu, y, s) ]
    // for a fixed mu. K = H's output length is read per call (dampedNewton's FD
    // Jacobian and normal-equations step allow the output dim K + 2M to differ from
    // the input dim N + 2M -- this is what keeps arbitrary K, M, N working).
    const auto makeResidual = [&](double mu) -> VectorField {
      return [H, G, phiFn, N, M, mu](const VectorXd& ww) -> VectorXd {
        const VectorXd x = ww.segment(0, N);
        const VectorXd y = ww.segment(N, M);
        const VectorXd s = ww.segment(N + M, M);

        const VectorXd Hxy = H(x, y);          // K
        const VectorXd Gxy = G(x, y);          // M
        const VectorXd phi = phiFn(mu, y, s);  // M
        const Index K = Hxy.size();

        VectorXd out(K + 2 * M);
        out.segment(0, K) = Hxy;
        out.segment(K, M) = s - Gxy;
        out.segment(K + M, M) = phi;
        return out;
      };
    };

    double mu = params.u0;
    int    levels = 0;
    int    totalInner = 0;
    double residual = 0.0;
    bool   converged = false;

    // Outer continuation: solve at the current mu (warm-started from the previous
    // level's w), then shrink mu geometrically to the floor. mu never enters a Newton
    // system, so it cannot collapse; only the final-level result sets 'converged'.
    for (int level = 0; level < params.maxOuter; ++level) {
      const VectorField Fmu = makeResidual(mu);
      const VIResult inner = dampedNewtonSolve(Fmu, w, params.damped);
      w           = inner.z;
      residual    = inner.residual;
      totalInner += inner.iter;
      ++levels;

      if (mu <= params.muMin) {
        converged = inner.converged && (residual < params.outerTol);
        break;
      }
      mu = std::max(params.sigma * mu, params.muMin);
    }

    // Return z = [mu_final, x, y, s] so smoothingDecode applies unchanged.
    VectorXd z(1 + N + 2 * M);
    z(0) = mu;
    z.segment(1, N + 2 * M) = w;
    return VIResult{ z, residual, levels, converged, totalInner };
  }

} // namespace VINCP
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------

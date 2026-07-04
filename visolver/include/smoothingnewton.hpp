// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
// Non-interior-point smoothing solver for the mixed NCP / KKT system.
// ----------------------------------------------
#ifndef VINCP_SMOOTHINGNEWTON_HPP
#define VINCP_SMOOTHINGNEWTON_HPP

//     0 = H(x, y),                 H : R^N x R^M -> R^K
//     0 <= y _|_ G(x, y) >= 0,     G : R^N x R^M -> R^M
//
// Following the reformulation of Zhang, Liu & Liu, "A non-interior-point smoothing
// method for variational inequality problem" (JCAM 234, 2010): introduce a slack
// s in R^M and a smoothing parameter u, and stack the residual
//
//     F(z) = [ u ; H(x,y) ; s - G(x,y) ; phi(u, y, s) ],   z = [u, x, y, s],
//
// where phi is a smoothed complementarity function (default: smoothed Fischer-
// Burmeister). We do NOT reproduce the paper's centering / neighborhood Newton;
// instead F(z) = 0 is solved by the least-squares damped-Newton dampedNewtonSolve.
// Carrying u as the first residual row makes the (1/2) u^2 term of that solver's
// merit drive u -> 0 (recovering exact complementarity), with no homotopy schedule.
// Because dampedNewton uses a finite-difference Jacobian, H and G may be arbitrary
// black boxes, and its rectangular support allows K != N.

#include "vincp.hpp"
#include "dampednewton.hpp"

#include <Eigen/Dense>
#include <functional>

namespace VINCP {

  // A smoothed complementarity function phi(u, a, b), applied elementwise to the
  // equal-length vectors a, b with scalar smoothing parameter u. Swap in alternatives
  // (CHKS, Kanzow, ...) via SmoothingNewtonParams::smoothing.
  using SmoothingFunction =
      function<VectorXd(double u, const VectorXd& a, const VectorXd& b)>;

  // Default smoothing: smoothed Fischer-Burmeister,
  //     phi(u, a, b)_i = a_i + b_i - sqrt(a_i^2 + b_i^2 + u^2).
  // At u = 0 this is the exact Fischer-Burmeister function, whose zero characterizes
  // 0 <= a_i _|_ b_i >= 0. Throws std::invalid_argument if a and b differ in length.
  VectorXd smoothedFischerBurmeister(double u, const VectorXd& a, const VectorXd& b);

  // Alternative smoothing: the Wu & Zhao (2013) function, eq. (2) of "One-step
  // smoothing inexact Newton method for NCP with a P0 function":
  //     phi(u, a, b)_i = a_i + b_i
  //       - sqrt( (a_i - u^2 (a_i - b_i))^2 + (b_i + u^2 (a_i - b_i))^2 + u^2 ).
  // Like smoothedFischerBurmeister it reduces to exact Fischer-Burmeister at u = 0
  // (the u^2(a-b) terms vanish), so its zero likewise characterizes 0 <= a_i _|_
  // b_i >= 0; but for u > 0 the two differ. The paper shows this form keeps the
  // smoothed system's Jacobian nonsingular for a P0 F across the whole space -- a
  // property the plain smoothed FB does not enjoy. Same signature, so it drops into
  // SmoothingNewtonParams::smoothing. Throws std::invalid_argument if a and b differ
  // in length.
  VectorXd smoothedFB_WZ(double u, const VectorXd& a, const VectorXd& b);

  // The problem maps H(x, y) and G(x, y); same shape as VIModel's H/G (vincp.hpp), but
  // with no dimension constraint linking their outputs to x (K may differ from N).
  using MixedField = function<VectorXd(const VectorXd& x, const VectorXd& y)>;

  struct SmoothingNewtonParams {
    double             u0        = 1.0;                        // initial smoothing (> 0)
    SmoothingFunction  smoothing = smoothedFischerBurmeister;  // swappable phi
    DampedNewtonParams damped    = DampedNewtonParams{};       // inner damped-Newton controls
  };

  // The smoothing solution decoded from a solver VIResult, whose packed z is
  // [u, x, y, s]. smoothingNewtonSolve returns the raw VIResult (uniform with every
  // other solver); call smoothingDecode to recover the domain vectors.
  struct SmoothingSolution {
    VectorXd x;            // primal solution
    VectorXd y;            // multipliers (the complementarity variable)
    VectorXd s;            // slacks (s - G(x,y) is driven to 0, so s ~ G(x,y) at the root)
    double   u = 0.0;      // final smoothing parameter (~ 0)
  };

  // Decode a smoothing VIResult (z = [u, x, y, s], x in R^N, y and s in R^M).
  SmoothingSolution smoothingDecode(const VIResult& r, Index N, Index M);

  // Solve the mixed NCP above by the smoothing reformulation + dampedNewton. x0, y0 are
  // the starting primal / multiplier vectors (N = x0.size(), M = y0.size()); the slack
  // starts at s0 = G(x0, y0) and the smoothing parameter at params.u0. G(x0, y0) must
  // have length M. Throws std::invalid_argument on empty starts, u0 <= 0, an unset
  // smoothing function, or a G whose length != M; propagates dampedNewton's throws.
  // Returns the raw VIResult (z = [u, x, y, s]); decode it with smoothingDecode.
  VIResult smoothingNewtonSolve(const MixedField& H, const MixedField& G,
                                const VectorXd& x0, const VectorXd& y0,
                                const SmoothingNewtonParams& params = SmoothingNewtonParams{});

  // -----------------------------------------------------------------------------
  // Continuation (homotopy) variant.
  //
  // The default smoothingNewtonSolve carries u as the first row of the residual and
  // lets the least-squares merit drive it to 0 -- which it does in ~one step (that
  // row is linear in u), so the smoothing evaporates before the iterates converge
  // and Newton stalls at the (near-singular) exact-FB kink. The continuation solver
  // instead demotes u from a solved unknown to an OUTER parameter: at a fixed mu it
  // solves only [ H(x,y); s - G(x,y); phi(mu, y, s) ] = 0 for w = (x, y, s) with the
  // SAME least-squares dampedNewton (so it keeps arbitrary K, M, N -- the residual is
  // rectangular, K + 2M out by N + 2M in), then shrinks mu geometrically toward a
  // floor, warm-starting each level from the previous solution. Because mu is never a
  // Newton unknown, it cannot collapse. See the design in the project notes.
  struct SmoothingContinuationParams {
    double u0       = 1.0;       // initial smoothing mu (> 0)
    double sigma    = 0.1;       // geometric reduction factor per level, in (0, 1)
    double muMin    = 1.0e-12;   // smoothing floor / final level (> 0, and <= u0)
    int    maxOuter = 60;        // backstop on the number of mu-levels
    double outerTol = 1.0e-14;   // squared-residual tolerance at the final level
    SmoothingFunction  smoothing = smoothedFischerBurmeister;  // reuse phi (FB or WZ)
    DampedNewtonParams damped    = DampedNewtonParams{};       // inner-solve controls
  };

  // Solve the mixed NCP by smoothing continuation (see above). x0, y0 start the primal
  // / multiplier blocks (N = x0.size(), M = y0.size()); the slack starts at s0 =
  // G(x0, y0), which must have length M. Throws std::invalid_argument on empty starts,
  // an unset smoothing function, a G whose length != M, or params outside their ranges
  // (u0 <= 0, sigma not in (0,1), muMin <= 0, muMin > u0, maxOuter <= 0, outerTol <= 0);
  // propagates dampedNewton's throws. Returns the raw VIResult with z = [mu_final, x,
  // y, s] (so smoothingDecode applies unchanged), iter = number of mu-levels, and
  // innerIters = total dampedNewton iterations across all levels.
  VIResult smoothingContinuationSolve(const MixedField& H, const MixedField& G,
                                      const VectorXd& x0, const VectorXd& y0,
                                      const SmoothingContinuationParams& params =
                                          SmoothingContinuationParams{});

} // namespace VINCP

#endif // VINCP_SMOOTHINGNEWTON_HPP
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------

// Copyright Ben Paul Wise. All Rights Reserved.
// "A non-interior-point smoothing method for variational inequality problem"
// by Zhang, Liu, Liu in Journal of Computational and Applied Mathematics 234 (2010)
//
// Their goal was to minimize scalar f(x) s.g. G(x) => 0 which can be solved as
//
//     ∇f(x)  =  λ ∇g(x)
//     0 ≤ λ  ⊥  g(x) ≥ 0
//
// As ∇f(x) is a row vector, they transposes the top line to use column vectors.
// The auxiliary vector 's' is used to decouple the two uses of 'G'.
// ------------------------
// I realized that this could probably be generalized using
// the same 's',
//
// H(x,y) = ∇f(x) - y ∇g(x) and
// G(x,y) = g(x)
//
// like this:
//
//         0 = H(x,y)
//     0 ≤ y ⊥ G(x,y) ≥ 0
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

#include <stdexcept>

namespace VINCP {

VectorXd smoothedFischerBurmeister(double u, const VectorXd& a, const VectorXd& b) {
    if (a.size() != b.size()) {
        throw std::invalid_argument("smoothedFischerBurmeister: a and b must have equal length.");
    }
    // phi_i = a_i + b_i - sqrt(a_i^2 + b_i^2 + u^2); at u = 0 this is exact FB.
    return (a.array() + b.array()
            - (a.array().square() + b.array().square() + u * u).sqrt())
        .matrix();
}

    // Uses Damped Newton (Levenberg Marquart) to solve
    //
    //         0 = H(x,y)
    //     0 ≤ y ⊥ G(x,y) ≥ 0
    // as
    //       H(x,y)
    //     s - G(x,y)
    //     FB(u, y, s)
VIResult smoothingNewtonSolve(const MixedField& H, const MixedField& G,
                              const VectorXd& x0, const VectorXd& y0,
                              const SmoothingNewtonParams& params) {
    if (!H || !G) {
        throw std::invalid_argument("smoothingNewtonSolve: H and G must be set.");
    }
    if (x0.size() <= 0) {
        throw std::invalid_argument("smoothingNewtonSolve: x0 must be non-empty.");
    }
    if (y0.size() <= 0) {
        throw std::invalid_argument("smoothingNewtonSolve: y0 must be non-empty.");
    }
    if (!(params.u0 > 0.0)) {
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

SmoothingSolution smoothingDecode(const VIResult& r, Index N, Index M) {
    // Mirror the z = [u, x, y, s] packing assembled above.
    SmoothingSolution sol;
    sol.u = r.z(0);
    sol.x = r.z.segment(1, N);
    sol.y = r.z.segment(1 + N, M);
    sol.s = r.z.segment(1 + N + M, M);
    return sol;
}

} // namespace VINCP
// Copyright Ben Paul Wise. All Rights Reserved.

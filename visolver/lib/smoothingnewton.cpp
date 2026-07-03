// Copyright Ben Paul Wise. All Rights Reserved.
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

SmoothingResult smoothingNewtonSolve(const MixedField& H, const MixedField& G,
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

    const Eigen::Index N = x0.size();
    const Eigen::Index M = y0.size();

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
        const Eigen::Index K = Hxy.size();

        VectorXd out(1 + K + 2 * M);
        out(0) = u;
        out.segment(1, K) = Hxy;
        out.segment(1 + K, M) = s - Gxy;
        out.segment(1 + K + M, M) = phi;
        return out;
    };

    const VIResult r = dampedNewtonSolve(F, z0, params.damped);

    // Unpack z = [u, x, y, s] back into the domain result.
    SmoothingResult out;
    out.u     = r.z(0);
    out.x     = r.z.segment(1, N);
    out.y     = r.z.segment(1 + N, M);
    out.s     = r.z.segment(1 + N + M, M);
    out.solve = r;
    return out;
}

} // namespace VINCP
// Copyright Ben Paul Wise. All Rights Reserved.

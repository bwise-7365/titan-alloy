// Copyright Ben Paul Wise. All Rights Reserved.
#include "ellipsoidprojector.hpp"

#include <cmath>
#include <stdexcept>
#include <string>

namespace VINCP {

namespace {

void validateRadii(const VectorXd& x, const VectorXd& radii, const char* who) {
    if (radii.size() == 0) {
        throw std::invalid_argument(std::string(who) + ": radii must be non-empty.");
    }
    if (x.size() != radii.size()) {
        throw std::invalid_argument(std::string(who) + ": point and radii must have equal length.");
    }
    for (Index i = 0; i < radii.size(); ++i) {
        if (!(radii(i) > 0.0)) {
            throw std::invalid_argument(std::string(who) + ": every radius must be positive.");
        }
    }
}

// The KKT stationarity point for a given multiplier lambda >= 0:
//     x_i = r_i^2 y_i / (r_i^2 + lambda).
// At lambda = 0 this is y; as lambda grows every coordinate shrinks toward 0.
VectorXd trialPoint(const VectorXd& y, const VectorXd& radii, double lambda) {
    VectorXd x(y.size());
    for (Index i = 0; i < y.size(); ++i) {
        const double r2 = radii(i) * radii(i);
        x(i) = (r2 * y(i)) / (r2 + lambda);
    }
    return x;
}

} // namespace

double ellipsoidNorm(const VectorXd& x, const VectorXd& radii) {
    validateRadii(x, radii, "ellipsoidNorm");
    double sum = 0.0;
    for (Index i = 0; i < x.size(); ++i) {
        const double ratio = x(i) / radii(i);
        sum += ratio * ratio;
    }
    return std::sqrt(sum);
}

VectorXd projectEllipsoid(const VectorXd& y, const VectorXd& radii, double tol, int iterMax) {
    validateRadii(y, radii, "projectEllipsoid");
    if (!(tol > 0.0)) {
        throw std::invalid_argument("projectEllipsoid: tol must be positive.");
    }
    if (iterMax <= 0) {
        throw std::invalid_argument("projectEllipsoid: iterMax must be positive.");
    }

    // Already inside (or on) the ellipsoid: the point is its own projection.
    if (ellipsoidNorm(y, radii) <= 1.0) {
        return y;
    }

    const Index n = y.size();
    const double sqrtN = std::sqrt(static_cast<double>(n));

    // Bracket the multiplier. lambda = 0 gives trialPoint = y with norm > 1. lambdaHi
    // is the least value making every |x_i / r_i| <= 1/sqrt(n), so the norm is <= 1:
    //     |r_i y_i| / (r_i^2 + lambda) <= 1/sqrt(n)  <=>  lambda >= sqrt(n)|r_i y_i| - r_i^2.
    // Since y is strictly outside, some |y_i / r_i| > 1/sqrt(n), hence lambdaHi > 0.
    double lambdaHi = 0.0;
    for (Index i = 0; i < n; ++i) {
        const double r2 = radii(i) * radii(i);
        const double candidate = sqrtN * std::abs(radii(i) * y(i)) - r2;
        if (candidate > lambdaHi) {
            lambdaHi = candidate;
        }
    }

    // Root-find g(lambda) = ellipsoidNorm(trialPoint(lambda)) = 1 on the bracket
    // [lo, hi], where g is strictly decreasing so g(lo) > 1 (v_lo > 0) and
    // g(hi) <= 1 (v_hi <= 0). Each pass does a regula-falsi step then a bisection
    // step; the latter guarantees the bracket at least halves, the former accelerates.
    double lo = 0.0;
    double hi = lambdaHi;
    double vLo = ellipsoidNorm(trialPoint(y, radii, lo), radii) - 1.0;   // > 0
    double vHi = ellipsoidNorm(trialPoint(y, radii, hi), radii) - 1.0;   // <= 0

    for (int iter = 0; iter < iterMax; ++iter) {
        // Regula falsi: the zero crossing of the secant through (lo, vLo), (hi, vHi).
        // vLo > 0 and vHi < 0, so this lies strictly inside (lo, hi).
        const double lRF = (hi * vLo - lo * vHi) / (vLo - vHi);
        const double vRF = ellipsoidNorm(trialPoint(y, radii, lRF), radii) - 1.0;
        if (std::abs(vRF) <= tol) {
            return trialPoint(y, radii, lRF);
        }
        if (vRF > 0.0) {
            lo = lRF; vLo = vRF;
        } else {
            hi = lRF; vHi = vRF;
        }

        // Bisection on the (possibly already tightened) bracket.
        const double lBis = 0.5 * (lo + hi);
        const double vBis = ellipsoidNorm(trialPoint(y, radii, lBis), radii) - 1.0;
        if (std::abs(vBis) <= tol) {
            return trialPoint(y, radii, lBis);
        }
        if (vBis > 0.0) {
            lo = lBis; vLo = vBis;
        } else {
            hi = lBis; vHi = vBis;
        }
    }

    return trialPoint(y, radii, 0.5 * (lo + hi));
}

Projector makeEllipsoidProjector(const VectorXd& radii, double tol, int iterMax) {
    // Validate up front so a bad ellipsoid fails at construction, not per-call.
    if (radii.size() == 0) {
        throw std::invalid_argument("makeEllipsoidProjector: radii must be non-empty.");
    }
    for (Index i = 0; i < radii.size(); ++i) {
        if (!(radii(i) > 0.0)) {
            throw std::invalid_argument("makeEllipsoidProjector: every radius must be positive.");
        }
    }
    if (!(tol > 0.0)) {
        throw std::invalid_argument("makeEllipsoidProjector: tol must be positive.");
    }
    if (iterMax <= 0) {
        throw std::invalid_argument("makeEllipsoidProjector: iterMax must be positive.");
    }
    const VectorXd r = radii;
    return [r, tol, iterMax](const VectorXd& y) -> VectorXd {
        return projectEllipsoid(y, r, tol, iterMax);
    };
}

} // namespace VINCP
// Copyright Ben Paul Wise. All Rights Reserved.

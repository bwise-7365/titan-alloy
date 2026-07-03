// Copyright Ben Paul Wise. All Rights Reserved.
#include "ellipsoidprojector.hpp"

#include <Eigen/Dense>
#include <cmath>
#include <cstdio>
#include <exception>

using namespace VINCP;
using std::printf;

namespace {

// Tolerances for the checks below (all comparisons are on well-scaled O(1)-O(10)
// quantities, so a common absolute tolerance is appropriate).
const double kBoundaryTol = 1.0e-8;   // |ellipsoidNorm(projection) - 1|
const double kMatchTol    = 1.0e-7;   // agreement with a closed-form / cross-check
const double kCosTol      = 1.0e-6;   // 1 - cos(angle) for the KKT parallelism check

int g_failures = 0;

void check(bool ok, const char* what) {
    printf("  [%s] %s\n", ok ? "PASS" : "FAIL", what);
    if (!ok) {
        ++g_failures;
    }
}

VectorXd vec3(double a, double b, double c) {
    VectorXd v(3);
    v << a, b, c;
    return v;
}

} // namespace

int main() {
    // ---- 1. A point already inside is its own projection. ------------------------
    {
        const VectorXd radii = vec3(2.0, 3.0, 4.0);
        const VectorXd y     = vec3(0.5, 0.5, 0.5);   // ellipsoidNorm << 1
        const VectorXd x     = VINCP::projectEllipsoid(y, radii);
        check(VINCP::ellipsoidNorm(y, radii) < 1.0, "interior test point is inside");
        check((x - y).norm() == 0.0, "interior point returned unchanged");
    }

    // ---- 2. Sphere (equal radii): projection has the closed form R * y / ||y||. ---
    {
        const double   R     = 2.0;
        const VectorXd radii = vec3(R, R, R);
        const VectorXd y     = vec3(3.0, 4.0, 0.0);      // ||y|| = 5 > R
        const VectorXd x     = VINCP::projectEllipsoid(y, radii);
        const VectorXd xRef  = (R / y.norm()) * y;       // = (1.2, 1.6, 0)
        check((x - xRef).norm() < kMatchTol, "sphere projection matches R*y/||y||");
        check(std::abs(VINCP::ellipsoidNorm(x, radii) - 1.0) < kBoundaryTol,
              "sphere projection lands on the boundary");
    }

    // ---- 3. General ellipsoid: on the boundary and KKT-optimal. -------------------
    // At the projection x, the residual (y - x) must be parallel to and point along
    // the outward normal, grad = x_i / r_i^2 (proportional to the true gradient of
    // sum (x_i/r_i)^2). Check via the cosine of the angle between them.
    {
        const VectorXd radii = vec3(1.0, 2.0, 5.0);
        const VectorXd y     = vec3(3.0, -4.0, 10.0);    // clearly outside
        const VectorXd x     = VINCP::projectEllipsoid(y, radii);
        check(std::abs(VINCP::ellipsoidNorm(x, radii) - 1.0) < kBoundaryTol,
              "ellipsoid projection lands on the boundary");

        VectorXd grad(3);
        for (int i = 0; i < 3; ++i) {
            grad(i) = x(i) / (radii(i) * radii(i));
        }
        const VectorXd d = y - x;
        const double cosAngle = d.dot(grad) / (d.norm() * grad.norm());
        check(d.dot(grad) > 0.0, "residual points outward (same side as the normal)");
        check(std::abs(1.0 - cosAngle) < kCosTol, "residual is parallel to the ellipsoid normal (KKT)");
    }

    // ---- 4. The Projector factory agrees, and projection is (near) idempotent. ----
    {
        const VectorXd radii = vec3(1.5, 3.0, 0.75);
        const VectorXd y     = vec3(-6.0, 2.0, 4.0);
        const VINCP::Projector Pr = VINCP::makeEllipsoidProjector(radii);
        const VectorXd x  = Pr(y);
        const VectorXd x2 = Pr(x);
        check((x - VINCP::projectEllipsoid(y, radii)).norm() == 0.0,
              "makeEllipsoidProjector matches projectEllipsoid");
        check((x2 - x).norm() < kMatchTol, "projection is (near) idempotent on its image");
    }

    // ---- 5. Invalid radii are rejected, not silently accepted. -------------------
    {
        bool threw = false;
        try {
            (void)VINCP::projectEllipsoid(vec3(1.0, 1.0, 1.0), vec3(1.0, -2.0, 3.0));
        } catch (const std::invalid_argument&) {
            threw = true;
        }
        check(threw, "a non-positive radius throws std::invalid_argument");
    }

    printf("\n%s\n", (g_failures == 0) ? "PASS (ellipsoid projection)"
                                       : "FAIL (ellipsoid projection)");
    return (g_failures == 0) ? 0 : 1;
}
// Copyright Ben Paul Wise. All Rights Reserved.

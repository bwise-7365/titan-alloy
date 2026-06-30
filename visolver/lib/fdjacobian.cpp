#include "fdjacobian.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <stdexcept>

namespace VINCP {

Eigen::MatrixXd centralDifferenceJacobian(const VectorField& F,
                                          const VectorXd& z,
                                          double stepRel) {
    if (!F) {
        throw std::invalid_argument("centralDifferenceJacobian: F must be set.");
    }
    if (z.size() <= 0) {
        throw std::invalid_argument("centralDifferenceJacobian: z must be non-empty.");
    }
    if (stepRel <= 0.0) {
        stepRel = std::cbrt(std::numeric_limits<double>::epsilon());
    }

    const Eigen::Index d = z.size();

    // Establishes the output dimension p and validates the base point.
    const VectorXd f0 = F(z);
    if (!f0.allFinite()) {
        throw std::runtime_error("centralDifferenceJacobian: F(z) is non-finite.");
    }
    const Eigen::Index p = f0.size();

    Eigen::MatrixXd jac(p, d);
    VectorXd zp = z;
    VectorXd zm = z;

    for (Eigen::Index j = 0; j < d; ++j) {
        const double zj = z(j);
        const double h = stepRel * std::max(std::abs(zj), 1.0);

        // Snap the perturbed points to representable values, then take the
        // actual width between them; this curbs the rounding error that an
        // assumed analytic 2*h would introduce.
        const double zjp = zj + h;
        const double zjm = zj - h;
        const double width = zjp - zjm;

        zp(j) = zjp;
        zm(j) = zjm;

        const VectorXd fp = F(zp);
        const VectorXd fm = F(zm);

        // Restore the column before moving on.
        zp(j) = zj;
        zm(j) = zj;

        if (fp.size() != p || fm.size() != p) {
            throw std::runtime_error("centralDifferenceJacobian: F changed output length.");
        }
        if (!fp.allFinite() || !fm.allFinite()) {
            throw std::runtime_error("centralDifferenceJacobian: F evaluation is non-finite.");
        }

        jac.col(j) = (fp - fm) / width;
    }

    return jac;
}

} // namespace VINCP

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
        // Optimal relative step for the 4th-order central stencil: eps^(1/5).
        stepRel = std::pow(std::numeric_limits<double>::epsilon(), 0.2);
    }

    const Eigen::Index d = z.size();

    // Establishes the output dimension p and validates the base point.
    const VectorXd f0 = F(z);
    if (!f0.allFinite()) {
        throw std::runtime_error("centralDifferenceJacobian: F(z) is non-finite.");
    }
    const Eigen::Index p = f0.size();

    Eigen::MatrixXd jac(p, d);
    VectorXd zw = z;   // work point; only component j is perturbed at a time

    for (Eigen::Index j = 0; j < d; ++j) {
        const double zj = z(j);
        double h = stepRel * std::max(std::abs(zj), 1.0);

        // Snap h to an exactly representable step to curb rounding error.
        h = (zj + h) - zj;

        // Evaluate F at zj + shift (component j only), with validation.
        const auto evalShift = [&](double shift) -> VectorXd {
            zw(j) = zj + shift;
            const VectorXd f = F(zw);
            if (f.size() != p) {
                throw std::runtime_error("centralDifferenceJacobian: F changed output length.");
            }
            if (!f.allFinite()) {
                throw std::runtime_error("centralDifferenceJacobian: F evaluation is non-finite.");
            }
            return f;
        };

        // 4th-order central stencil:
        //   f'(x) = [ f(x-2h) - 8 f(x-h) + 8 f(x+h) - f(x+2h) ] / (12 h)  + O(h^4).
        const VectorXd fm2 = evalShift(-2.0 * h);
        const VectorXd fm1 = evalShift(-1.0 * h);
        const VectorXd fp1 = evalShift(+1.0 * h);
        const VectorXd fp2 = evalShift(+2.0 * h);
        zw(j) = zj;   // restore the column before moving on

        jac.col(j) = (fm2 - 8.0 * fm1 + 8.0 * fp1 - fp2) / (12.0 * h);
    }

    return jac;
}

} // namespace VINCP

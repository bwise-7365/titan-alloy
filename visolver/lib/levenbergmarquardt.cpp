// Copyright Ben Paul Wise. All Rights Reserved.
#include "levenbergmarquardt.hpp"

#include <algorithm>
#include <stdexcept>

namespace VINCP {

Eigen::MatrixXd levenbergMarquardtDamp(const Eigen::MatrixXd& J, double lambda) {
    if (J.rows() != J.cols()) {
        throw std::invalid_argument("levenbergMarquardtDamp: J must be square.");
    }
    if (lambda < 0.0) {
        throw std::invalid_argument("levenbergMarquardtDamp: lambda must be non-negative.");
    }
    return J + lambda * Eigen::MatrixXd::Identity(J.rows(), J.cols());
}

double levenbergMarquardtUpdate(double lambda, bool stepAccepted,
                                const LevenbergMarquardtParams& params) {
    const double updated = stepAccepted ? (lambda * params.decrease)
                                        : (lambda * params.increase);
    return std::clamp(updated, params.lambdaMin, params.lambdaMax);
}

} // namespace VINCP
// Copyright Ben Paul Wise. All Rights Reserved.

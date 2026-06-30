#include "vincp.hpp"

#include <stdexcept>

namespace VINCP {

VectorXd projectNonnegative(const VectorXd& v) {
    return v.cwiseMax(0.0);
}

Projector makeMixedProjector(Eigen::Index numFree) {
    if (numFree < 0) {
        throw std::invalid_argument("makeMixedProjector: numFree must be non-negative.");
    }
    return [numFree](const VectorXd& v) -> VectorXd {
        if (numFree > v.size()) {
            throw std::invalid_argument("mixed projector: numFree exceeds vector length.");
        }
        VectorXd out = v;
        const Eigen::Index tail = v.size() - numFree;
        out.tail(tail) = v.tail(tail).cwiseMax(0.0);
        return out;
    };
}

VectorXd evaluateF(const VIModel& model, const VectorXd& z) {
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

} // namespace VINCP

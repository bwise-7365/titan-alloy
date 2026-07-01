#include "utils.hpp"

#include <cstdio>

namespace VINCP {

    // My extension of RC6's function.
    // It is 1-to-1
    uint64_t qTrans(uint64_t x) {
        const uint64_t a = 3;
        const uint64_t n = 4;
        const uint64_t c = 17;
        const uint64_t y = (x+a)*((n*x)+c);
        return y;
    }

void printVector(const char* label, const Eigen::VectorXd& v) {
    std::printf("%s = [", label);
    for (Eigen::Index i = 0; i < v.size(); ++i) {
        std::printf(" %9.4f", v(i));
    }
    std::printf(" ]\n");
}

void makeComplementaryPair(Eigen::Index n, std::mt19937& rng,
                           int intLo, int intHi,
                           Eigen::VectorXd& w, Eigen::VectorXd& z) {
    std::uniform_int_distribution<int> intDist(intLo, intHi);
    w = Eigen::VectorXd::Zero(n);
    z = Eigen::VectorXd::Zero(n);
    for (Eigen::Index i = 0; i < n; ++i) {
        if (i % 2 == 0) {
            w(i) = static_cast<double>(intDist(rng));   // even index: w active, z = 0
        } else {
            z(i) = static_cast<double>(intDist(rng));   // odd  index: z active, w = 0
        }
    }
}

void printConstructed(const Eigen::VectorXd& z, const Eigen::VectorXd& w) {
    std::printf("\nconstructed solution (0 <= z _|_ w >= 0, w = M z + q):\n");
    printVector("  z", z);
    printVector("  w", w);
}

int reportAndCheck(const VIResult& result,
                   const Eigen::MatrixXd& M, const Eigen::VectorXd& q,
                   const Eigen::VectorXd& zConstructed, double solTol) {
    const Eigen::VectorXd wSolved = M * result.z + q;   // implied w at the solver's z
    const double solErr = (result.z - zConstructed).norm();

    std::printf("\nsolver result:\n");
    printVector("            z", result.z);
    printVector("  w = M z + q", wSolved);
    std::printf("\niterations     = %d\n", result.iter);
    std::printf("residual       = %.3e (squared)\n", result.residual);
    std::printf("converged      = %s\n", result.converged ? "true" : "false");
    std::printf("solution error = %.3e (||z_solved - z_constructed||)\n", solErr);

    if (result.converged && solErr < solTol) {
        std::printf("PASS (within %.1e)\n", solTol);
        return 0;
    }
    std::printf("FAIL (exceeds %.1e)\n", solTol);
    return 1;
}

} // namespace VINCP

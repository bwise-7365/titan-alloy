// Copyright Ben Paul Wise. All Rights Reserved.
#include "utils.hpp"

#include <chrono>
#include <cstdio>
#include <stdexcept>

namespace VINCP {
    uint64_t microsecondSeed() {
        const auto sinceEpoch = std::chrono::system_clock::now().time_since_epoch();
        const auto micros = std::chrono::duration_cast<std::chrono::microseconds>(sinceEpoch);
        return static_cast<uint64_t>(micros.count());
    }

    // My extension of RC6's function.
    // It is 1-to-1
    uint64_t qTrans(uint64_t x) {
        const uint64_t a = 3; //  must be positive, odd
        const uint64_t n = 4; //  must be positive, even
        const uint64_t c = 17; //  must be positive, odd
        const uint64_t y = (x+a)*((n*x)+c);
        return y;
    }

    uint64_t makeSeed(const uint64_t s1, const bool verbose) {
        uint64_t s2 = s1;
        bool generated = false;
        while (0 == s2) {
            s2 = qTrans(microsecondSeed());
            generated = true;
        }
        if (verbose) {
            if (generated) {
                printf("Generated PRNG seed\n");
            }
            printf("Using PRNG seed  %020llu , 0x%0llX\n",
                    s2, s2);
        }
        return qTrans(s2);
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

    CubicProblem makeCubicProblem(Eigen::Index nIn, Eigen::Index nOut,
                                  std::mt19937& rng,
                                  const Eigen::VectorXd& xStar,
                                  const Eigen::VectorXd& fStar,
                                  bool forcePSD,
                                  double aLo, double aHi) {
        if (nIn <= 0 || nOut <= 0) {
            throw std::invalid_argument("makeCubicProblem: nIn and nOut must be positive.");
        }
        if (forcePSD && nIn != nOut) {
            throw std::invalid_argument("makeCubicProblem: forcePSD requires nIn == nOut.");
        }
        if (xStar.size() != nIn) {
            throw std::invalid_argument("makeCubicProblem: xStar length must equal nIn.");
        }
        if (fStar.size() != nOut) {
            throw std::invalid_argument("makeCubicProblem: fStar length must equal nOut.");
        }

        std::uniform_real_distribution<double> aDist(aLo, aHi);
        const Eigen::Index rows = forcePSD ? nIn : nOut;   // A is rows x nIn
        Eigen::MatrixXd A(rows, nIn);
        for (Eigen::Index r = 0; r < rows; ++r) {
            for (Eigen::Index c = 0; c < nIn; ++c) {
                A(r, c) = aDist(rng);
            }
        }

        // base(x) = g(A x), or A^T g(A x) in the PSD form, with g(u) = u.^3 + u.
        const bool psd = forcePSD;
        const auto base = [A, psd](const Eigen::VectorXd& x) -> Eigen::VectorXd {
            const Eigen::VectorXd u = A * x;
            const Eigen::VectorXd gu = (u.array().cube() + u.array()).matrix();
            if (psd) {
                return A.transpose() * gu;
            }
            return gu;
        };

        const Eigen::VectorXd k = fStar - base(xStar);
        const auto F = [base, k](const Eigen::VectorXd& x) -> Eigen::VectorXd {
            return base(x) + k;
        };

        return CubicProblem{ nIn, nOut, psd, A, k, xStar, fStar, F };
    }
} // namespace VINCP
// Copyright Ben Paul Wise. All Rights Reserved.

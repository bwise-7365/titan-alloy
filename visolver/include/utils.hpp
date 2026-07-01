// Copyright Ben Paul Wise. All Rights Reserved.
#ifndef VINCP_UTILS_HPP
#define VINCP_UTILS_HPP

// ============================================================================
// Small shared utilities used by demos and tests: vector printing plus the LCP
// test scaffolding (problem construction and result reporting) that would
// otherwise be duplicated across the test drivers. Not part of the numerical
// core, but kept in the library so callers do not copy-paste it.
// ============================================================================

#include "vincp.hpp"

#include <Eigen/Dense>
#include <cstdint>
#include <random>

namespace VINCP {
    // Microseconds since the Unix epoch, as a uint64_t. Intended ONLY as a quick
    // nondeterministic PRNG seed (e.g. std::mt19937 rng(microsecondSeed())) so a
    // test can be randomized run-to-run; it is not an accurate or monotonic clock.
    std::uint64_t microsecondSeed();

    // Generate the inner PRNG seed from a given one.
    // If the seed is zero, it will use microsecondSeed.
    uint64_t makeSeed(const uint64_t s1, const bool verbose);

    // Print a vector on one line as:  label = [ v0 v1 ... ].
    // Components are printed with a fixed field width for easy column alignment.
    void printVector(const char* label, const Eigen::VectorXd& v);

    // Build a complementary (w, z) pair for an LCP test: for each component i,
    // exactly one of w(i), z(i) is a random integer in [intLo, intHi] and the other
    // is zero, so 0 <= z _|_ w >= 0 holds by construction. Even indices (0-based,
    // i.e. the 1st, 3rd, ... entries) activate w; odd indices activate z.
    // The values are of type 'double' but their values are integers to make
    // output neater. Draws n integers from 'rng', so the caller's later draws
    // stay deterministic for a fixed seed. w and z are resized to length n.
    void makeComplementaryPair(Eigen::Index n, std::mt19937& rng,
                               int intLo, int intHi,
                               Eigen::VectorXd& w, Eigen::VectorXd& z);

    // Print the constructed (z, w) solution under a standard header.
    void printConstructed(const Eigen::VectorXd& z, const Eigen::VectorXd& w);

    // Print the solver's result (z and the implied w = M z + q), the iteration
    // count, squared residual, converged flag, and the error against the
    // constructed solution. Returns 0 if the solver converged and
    // ||z_solved - zConstructed|| < solTol, else 1 -- suitable as a test exit code.
    int reportAndCheck(const VIResult& result,
                       const Eigen::MatrixXd& M, const Eigen::VectorXd& q,
                       const Eigen::VectorXd& zConstructed, double solTol);

    // -----------------------------------------------------------------------
    // Cubic problem generator (shared by the VI demo and the LM test)
    // -----------------------------------------------------------------------
    //
    // A cubic map through linear forms u = A x with elementwise g(u) = u.^3 + u.
    // The constant k is chosen so F(xStar) == fStar for the caller's known point.
    //   - forcePSD = true : F(x) = A^T ( g(A x) ) + k, A is nIn x nIn (requires
    //     nIn == nOut). The Jacobian A^T diag(3 u^2 + 1) A is PSD, so F is a
    //     monotone gradient map -- the regime the VI/complementarity solver needs.
    //   - forcePSD = false: F(x) = g(A x) + k, A is nOut x nIn (rectangular ok).
    //     A general, NOT-necessarily-monotone cubic -- e.g. an overdetermined
    //     system with a known zero-residual root for a Levenberg-Marquardt test.
    struct CubicProblem {
        Eigen::Index    nIn  = 0;   // input dimension
        Eigen::Index    nOut = 0;   // output dimension
        bool            psd  = false;
        Eigen::MatrixXd A;          // forms matrix: nIn x nIn if psd, else nOut x nIn
        Eigen::VectorXd k;          // constant offset
        Eigen::VectorXd xStar;      // known point
        Eigen::VectorXd fStar;      // F(xStar), by construction
        std::function<Eigen::VectorXd(const Eigen::VectorXd&)> F;  // the cubic map
    };

    // Construct a cubic problem (see above). A is drawn from U[aLo, aHi] via rng;
    // k is set so F(xStar) == fStar. Throws std::invalid_argument on a dimension
    // mismatch (including forcePSD with nIn != nOut).
    CubicProblem makeCubicProblem(Eigen::Index nIn, Eigen::Index nOut,
                                  std::mt19937& rng,
                                  const Eigen::VectorXd& xStar,
                                  const Eigen::VectorXd& fStar,
                                  bool forcePSD,
                                  double aLo, double aHi);
} // namespace VINCP

#endif // VINCP_UTILS_HPP
// Copyright Ben Paul Wise. All Rights Reserved.

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
#include "fdjacobian.hpp"   // VectorField

#include <Eigen/Dense>
#include <chrono>
#include <cstdint>
#include <functional>
#include <random>
#include <string>

namespace VINCP {
    // Microseconds since the Unix epoch, as a uint64_t. Intended ONLY as a quick
    // nondeterministic PRNG seed (e.g. std::mt19937 rng(microsecondSeed())) so a
    // test can be randomized run-to-run; it is not an accurate or monotonic clock.
    std::uint64_t microsecondSeed();

    // Generate the inner PRNG seed from a given one.
    // If the seed is zero, it will use microsecondSeed.
    uint64_t makeSeed(const uint64_t s1, const bool verbose);

    // Print the current UTC time (millisecond precision), labeled "UTC", and
    // return it -- pass the returned value to utcElapsed to time an interval.
    std::chrono::system_clock::time_point utcNow();

    // Print the current UTC date-time and the elapsed time since 'start'
    // (millisecond precision).
    void utcElapsed(std::chrono::system_clock::time_point start);

    // RAII wall-clock timer: prints the start UTC on construction and, on
    // destruction (scope/function exit), the finish UTC and elapsed time.
    // Declare one at the top of main() to time an entire demo or test.
    class ScopedUtcTimer {
    public:
        explicit ScopedUtcTimer(std::string label = "");
        ~ScopedUtcTimer();
        ScopedUtcTimer(const ScopedUtcTimer&) = delete;
        ScopedUtcTimer& operator=(const ScopedUtcTimer&) = delete;
    private:
        std::string label_;
        std::chrono::system_clock::time_point start_;
    };

    // Print a solver's iteration counts, squared residual, and converged flag on
    // one line. A composite result (innerIters > 0, e.g. from solveVI) shows outer
    // and total inner iterations; a leaf result shows a single iteration count.
    void printSolveStats(const char* label, const VIResult& r);

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

    // Run one rectangular cubic least-squares case through 'solve' and check it:
    // builds a non-PSD cubic F: R^nIn -> R^nOut with a known root x* and a random
    // start (all from 'seed'), then reports and validates the result. For nOut >= nIn
    // the root is unique -- checks ||x - x*|| < 1e-6; for nOut < nIn the roots form the
    // manifold x* + null(A) -- checks convergence and ||A (x - x*)|| < 1e-6 (LM lands on
    // a possibly different root). 'solve' maps (F, x0) -> VIResult; bind a concrete
    // least-squares solver (levenbergMarquardtSolve, dampedNewtonSolve, ...) with its
    // own tolerances. 'solverName' labels the output. Returns 0 on pass, 1 on fail.
    int runCubicLsqCase(const char* solverName,
                        Eigen::Index nIn, Eigen::Index nOut,
                        std::uint_fast32_t seed,
                        const std::function<VIResult(const VectorField&,
                                                     const Eigen::VectorXd&)>& solve);

    // -----------------------------------------------------------------------
    // SAOE table display (shared by the SAOE demo and test)
    // -----------------------------------------------------------------------
    //
    // Each renders either a LaTeX tabular (latex = true, paste-ready) or plain
    // column-aligned ASCII (latex = false). Formatting: strengths %.1f, rewards
    // %7.1f, efforts %5.1f, utilities %.3f, probabilities %.3f.

    // A format-aware comment line: LaTeX "% text", ASCII "# text".
    void printComment(bool latex, const char* text);

    // Inputs table: actor index, strength S_i, then the N rewards r_{ij}.
    void saoePrintInputs(const Eigen::MatrixXd& R, const Eigen::VectorXd& S, bool latex);

    // Solution table: actor index, utility u_i, then efforts for the options that
    // carry non-zero effort (all-zero option columns omitted; "-" marks a zero),
    // with a final row of those options' probabilities. eps is the model's eps.
    void saoePrintSolution(const Eigen::MatrixXd& R, const Eigen::MatrixXd& e,
                           double eps, bool latex);
} // namespace VINCP

#endif // VINCP_UTILS_HPP
// Copyright Ben Paul Wise. All Rights Reserved.

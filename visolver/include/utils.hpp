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
#include <random>

namespace VINCP {

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

} // namespace VINCP

#endif // VINCP_UTILS_HPP

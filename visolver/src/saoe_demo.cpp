// Copyright Ben Paul Wise. All Rights Reserved.
#include "saoe.hpp"
#include "utils.hpp"

#include <Eigen/Dense>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <random>

using namespace VINCP;
using std::printf;

// SAOE demo: generate a random reward matrix R and strength vector S, solve the
// Nash equilibrium (VINCP::saoe), and print the problem and solution using the
// shared table display in utils. The 'latexOutput' switch selects the format:
//   true  -> LaTeX tabular, ready to paste into a paper;
//   false -> plain, column-aligned ASCII.
int main() {
    VINCP::ScopedUtcTimer timer("saoe_demo");
    const int numActors  = 5;                  // rows of R, entries of S
    const int numOptions = 7;                  // columns of R
    const std::uint64_t seed = 11528563544L;   // constant seed (random instance)
    const bool latexOutput = false;            // true = LaTeX tabular, false = aligned ASCII

    std::mt19937_64 rng(seed);
    std::uniform_real_distribution<double> rewardDist(-100.0, 200.0);
    std::uniform_real_distribution<double> strengthDist(10.0, 30.0);

    MatrixXd R(numActors, numOptions);
    for (int i = 0; i < numActors; ++i) {
        for (int j = 0; j < numOptions; ++j) {
            R(i, j) = rewardDist(rng);
        }
    }
    VectorXd S(numActors);
    for (int i = 0; i < numActors; ++i) {
        S(i) = std::round(strengthDist(rng) * 10.0) / 10.0;
    }

    const VINCP::VIResult r = VINCP::saoe(R, S);
    const VINCP::SaoeSolution sol = VINCP::saoeDecode(r, numActors, numOptions);

    char line[256];
    std::snprintf(line, sizeof line, "SAOE: %d actors, %d options, seed %llu, eps = %.3e",
                  numActors, numOptions, static_cast<unsigned long long>(seed),
                  VINCP::saoeEps(R));
    VINCP::printComment(latexOutput, line);
    VINCP::saoePrintInputs(R, S, latexOutput);

    printf("\n");
    std::snprintf(line, sizeof line,
                  "solver: converged = %s, outer iters = %d, total inner iters = %d, residual^2 = %.3e",
                  r.converged ? "true" : "false", r.iter,
                  r.innerIters, r.residual);
    VINCP::printComment(latexOutput, line);
    VINCP::saoePrintSolution(R, sol.e, VINCP::saoeEps(R), latexOutput);

    return 0;
}
// Copyright Ben Paul Wise. All Rights Reserved.

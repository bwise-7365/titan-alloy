// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
// Side-by-side printout demo of the two inner LVI solvers (Han vs He).
// ----------------------------------------------
#include "dhan06.hpp"
#include "bshe94b.hpp"
#include "josephynewton.hpp"
#include "utils.hpp"

#include <Eigen/Dense>
#include <cstdint>
#include <cstdio>
#include <exception>
#include <random>

using namespace VIMCP;
using std::printf;

// Side-by-side DEMO of the two inner LVI solvers -- Han (dHan06) and He
// (bsHe94b) -- run for their printout, not as a pass/fail gate (that is
// test/han_vs_he_test.cpp). It shows two things:
//   (1) a size sweep on a monotone linear VI (LCP, M = A^T A), so the two
//       solvers' iteration counts can be compared as the dimension grows; and
//   (2) a monotone nonlinear "PSD" VI driven through the SAME Josephy-Newton
//       outer loop with each solver plugged in via the InnerSolver seam, with
//       both solved vectors printed next to the known solution.

// One linear-VI (LCP) size: build M = A^T A with a known complementary root,
// solve with each inner solver, and print a one-line comparison.
static void
runLinearSize(Index N, std::mt19937& rng,
              int intLo, int intHi, double aLo, double aHi,
              double magTol, int iterMax)
{
  std::uniform_real_distribution<double> aDist(aLo, aHi);
  VectorXd w, zSol;
  makeComplementaryPair(N, rng, intLo, intHi, w, zSol);
  MatrixXd A(N, N);
  for (Index r = 0; r < N; ++r) {
    for (Index c = 0; c < N; ++c) {
      A(r, c) = aDist(rng);
    }
  }
  const MatrixXd M = A.transpose() * A;
  const VectorXd q  = w - M * zSol;
  const VectorXd x0 = VectorXd::Zero(N);

  const VIResult han =
      dHan06(x0, M, q, projectNonnegative, magTol, iterMax, 0);
  const VIResult he =
      bsHe94b(x0, M, q, projectNonnegative, magTol, iterMax, 0);

  printf("  N=%3td   dHan06: iters=%6d solErr=%.2e   |   bsHe94b: iters=%6d solErr=%.2e\n",
         static_cast<long long>(N),
         han.iter, (han.z - zSol).norm(),
         he.iter,  (he.z  - zSol).norm());
  return;
}

int
main()
{
  ScopedUtcTimer timer("han_vs_he_demo");
  const std::uint_fast32_t seed = 424242u;  // fixed for a reproducible demo

  const int    intLo = 1,    intHi = 10;    // complementary-pair magnitudes
  const double aLo   = -1.0, aHi   = 1.0;   // forms-matrix range
  const double magTol   = 1.0e-14;          // inner squared-residual tolerance
  const int    iterMax  = 100000;           // inner iteration cap

  std::mt19937 rng(seed);

  // ---- (1) Linear VI (LCP) size sweep ----
  printf("=== (1) Linear VI: monotone LCP, M = A^T A -- iteration-count sweep ===\n");
  const Index sizes[] = { 4, 8, 16, 32 };
  for (const Index N : sizes) {
    runLinearSize(N, rng, intLo, intHi, aLo, aHi, magTol, iterMax);
  }

  // ---- (2) PSD nonlinear VI through the JN outer loop ----
  const Index n = 4;                        // free block dimension
  const Index m = 4;                        // non-negative block dimension
  const Index d = n + m;
  const int    xLo = -4, xHi = 4;
  const double outerTol = 1.0e-10;
  const int    outerIterMax = 200;

  std::uniform_int_distribution<int> xDist(xLo, xHi);
  VectorXd xStar(n);
  for (Index i = 0; i < n; ++i) {
    xStar(i) = static_cast<double>(xDist(rng));
  }
  VectorXd wStar, yStar;
  makeComplementaryPair(m, rng, intLo, intHi, wStar, yStar);
  VectorXd zStar(d);
  zStar << xStar, yStar;

  VectorXd target(d);
  target << VectorXd::Zero(n), wStar;
  const CubicProblem prob =
      makeCubicProblem(d, d, rng, zStar, target, /*forcePSD=*/true, aLo, aHi);

  const VIModel model = makeVIModel(n, m, prob.F);

  const VectorXd z0 = VectorXd::Zero(d);
  JosephyNewtonParams params;
  params.outerTol     = outerTol;
  params.outerIterMax = outerIterMax;

  const InnerSolver innerHan = makeDHan06Solver(magTol, iterMax, 0);
  const InnerSolver innerHe  = makeBsHe94bSolver(magTol, iterMax, 0);

  printf("\n=== (2) PSD nonlinear VI: cubic via JN outer loop, n=%td free, m=%td nonneg ===\n",
         static_cast<long long>(n), static_cast<long long>(m));
  try {
    const VIResult han = solveVI(model, z0, innerHan, params);
    const VIResult he  = solveVI(model, z0, innerHe,  params);

    printVector("expected      ", zStar);
    printVector("dHan06 solved ", han.z);
    printVector("bsHe94b solved", he.z);
    printf("dHan06 : outer iters=%d  total inner iters=%d  converged=%s  solErr=%.2e\n",
           han.iter, han.innerIters, han.converged ? "true" : "false", (han.z - zStar).norm());
    printf("bsHe94b: outer iters=%d  total inner iters=%d  converged=%s  solErr=%.2e\n",
           he.iter,  he.innerIters, he.converged ? "true" : "false", (he.z - zStar).norm());
  }
  catch (const std::exception& ex) {
    printf("solveVI threw: %s\n", ex.what());
    return 1;
  }
  return 0;
}
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------

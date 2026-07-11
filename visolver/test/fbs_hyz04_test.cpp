// Copyright Ben Paul Wise. All Rights Reserved.
#include "ellipsoidprojector.hpp"
#include "fbshyz04.hpp"
#include "mcpengines.hpp"
#include "saoe.hpp"
#include "saoesupport.hpp"
#include "utils.hpp"

#include <Eigen/Dense>
#include <gtest/gtest.h>

#include <random>
#include <stdexcept>

using namespace VIMCP;

// Forward-backward splitting (He-Yuan-Zhang 2004): the general-VI engine
// reimplemented from the author's pmedemo code. Gated on the constructed
// problems it has theory for (monotone, Lipschitz), plus the HISTORICAL
// probe: pmedemo solved the reference SAOE instance with FBS and found a
// NON-VERTEX equilibrium (actor 1 splitting effort across options 4 and 6,
// supports {4, 6, 9} -- PME paper section 4.4.6). The probe here asks
// whether this reimplementation reproduces interior-equilibrium behavior;
// it is the "do actors really go all-or-nothing?" experiment.

namespace {
    constexpr std::uint_fast32_t kSeed = 20260706u;
    constexpr Index  kDim    = 8;
    constexpr double kMagTol = 1.0e-14;   // squared
    constexpr int    kIterMax = 200000;
    constexpr double kSolTol = 1.0e-5;
} // namespace

// Monotone LCP with a constructed solution, F bound as the affine field.
TEST(FbsHyz04, SolvesMonotoneLcp) {
    std::mt19937 rng(kSeed);
    VectorXd w, zStar;
    makeComplementaryPair(kDim, rng, 1, 10, w, zStar);
    std::uniform_real_distribution<double> aDist(-1.0, 1.0);
    MatrixXd A(kDim, kDim);
    for (Index r = 0; r < kDim; ++r) {
        for (Index c = 0; c < kDim; ++c) {
            A(r, c) = aDist(rng);
        }
    }
    const MatrixXd M = A.transpose() * A;
    const VectorXd q = w - M * zStar;
    const VectorField F = [&](const VectorXd& x) -> VectorXd { return M * x + q; };

    const VIResult r = fbsHyz04(VectorXd::Zero(kDim), F, projectNonnegative,
                                kMagTol, kIterMax, 0);
    EXPECT_TRUE(r.converged);
    EXPECT_LT((r.z - zStar).norm(), kSolTol);
}

// The point of the engine: a NONLINEAR monotone VI solved directly -- no
// Josephy-Newton wrapper, no Jacobian of any kind.
TEST(FbsHyz04, SolvesNonlinearMonotoneViDirectly) {
    std::mt19937 rng(kSeed);
    const Index n = 3, m = 3, d = n + m;
    VectorXd wStar, yStar;
    makeComplementaryPair(m, rng, 1, 10, wStar, yStar);
    VectorXd zStar(d);
    zStar << VectorXd::Constant(n, 1.0), yStar;
    VectorXd target(d);
    target << VectorXd::Zero(n), wStar;
    const CubicProblem prob = makeCubicProblem(d, d, rng, zStar, target,
                                               /*forcePSD=*/true, -1.0, 1.0);

    const VIResult r = fbsHyz04(VectorXd::Zero(d), prob.F, makeMixedProjector(n),
                                kMagTol, kIterMax, 0);
    EXPECT_TRUE(r.converged);
    EXPECT_LT((r.z - zStar).norm(), kSolTol);
}

// Generality in K: a constant field over an ellipsoid (minimize c^T x), whose
// solution is the boundary point x_i = -r_i^2 c_i / sqrt(sum (r_j c_j)^2).
TEST(FbsHyz04, SolvesOverEllipsoidK) {
    const Index d = 4;
    VectorXd radii(d), c(d);
    radii << 1.0, 2.0, 0.5, 3.0;
    c << 1.0, -2.0, 3.0, 0.5;
    const VectorField F = [&c](const VectorXd&) -> VectorXd { return c; };
    const double nu = (radii.cwiseProduct(c)).norm();
    const VectorXd xStar = -(radii.cwiseProduct(radii).cwiseProduct(c)) / nu;

    const VIResult r = fbsHyz04(VectorXd::Zero(d), F,
                                makeEllipsoidProjector(radii),
                                kMagTol, kIterMax, 0);
    EXPECT_TRUE(r.converged);
    EXPECT_LT((r.z - xStar).norm(), kSolTol);
}

TEST(FbsHyz04, RejectsBadInputs) {
    const VectorField F = [](const VectorXd& x) -> VectorXd { return x; };
    const VectorXd x0 = VectorXd::Zero(2);
    EXPECT_THROW(fbsHyz04(VectorXd(), F, projectNonnegative, 1e-10, 100, 0),
                 std::invalid_argument);
    EXPECT_THROW(fbsHyz04(x0, VectorField{}, projectNonnegative, 1e-10, 100, 0),
                 std::invalid_argument);
    EXPECT_THROW(fbsHyz04(x0, F, Projector{}, 1e-10, 100, 0),
                 std::invalid_argument);
    FbsHyz04Params badTheta;
    badTheta.theta = 1.5;
    EXPECT_THROW(fbsHyz04(x0, F, projectNonnegative, 1e-10, 100, 0, badTheta),
                 std::invalid_argument);
}

// The historical probe (PME paper 4.4.6): FBS on the reference SAOE
// instance. pmedemo's FBS found supports {4, 6, 9} with actor 1 SPLIT
// across options 4 and 6 -- an interior (non-vertex) equilibrium of the
// risk-NEUTRAL game. Gate: converged + feasible (which equilibrium it
// reaches is the experiment; the decoded allocation is printed for
// comparison with the paper's table). NOTE: SAOE is nonmonotone, so this
// is outside FBS's guarantee -- an honest failure here is itself a finding.
TEST(FbsHyz04, SaoeReferenceProbe) {
    MatrixXd R, E;
    VectorXd S;
    saoeReferenceInstance(R, S, E);
    const VIModel model = saoeModel(R, S);
    const VectorField F = [model](const VectorXd& z) -> VectorXd {
        return evaluateF(model, z);
    };

    const SolveFn solve = [&](const VectorXd& z0) -> VIResult {
        return fbsHyz04(z0, F, projectNonnegative, 1.0e-10, 60000, 5000,
                        FbsHyz04Params{},
                        [](int iter, int iterMax, double mag, double tol) {
                            std::printf("    fbs iter %d/%d: residual^2 %.3e (tol %.1e)\n",
                                        iter, iterMax, mag, tol);
                            std::fflush(stdout);
                        });
    };
    const int failures = runCase("fbs (SAOE reference probe)", solve,
                                 saoeDefaultStart(R, S),
                                 { saoePrintDecoded(R), saoeCheckFeasible(R, S),
                                   checkConvergedFlag() });
    EXPECT_EQ(0, failures)
        << "FBS did not converge to a feasible allocation on the reference "
           "SAOE instance (see the printed trajectory)";
}

// The eps-smoothing hypothesis probe: rerun the SAOE reference with
// pmedemo's strength floor eps = 0.1 (RMS(weights)/1000, vs this library's
// RMS(R)/1e4 ~ 0.0108). HYPOTHESIS: the larger floor smooths the game
// enough to admit the INTERIOR equilibrium pmedemo found (supports
// {4, 6, 9}, actor 1 split across options 4 and 6); at the sharp floor the
// vertex equilibrium E wins (the probe above). The gate is convergence +
// feasibility only; the split structure is printed and counted -- the
// vertex/interior outcome IS the experiment's reading.
TEST(FbsHyz04, SaoeEpsSmoothingProbe) {
    MatrixXd R, E;
    VectorXd S;
    saoeReferenceInstance(R, S, E);
    const double pmedemoEps = 0.1;
    const VIModel model = saoeModel(R, S, /*riskAversion=*/0.0, pmedemoEps);
    const VectorField F = [model](const VectorXd& z) -> VectorXd {
        return evaluateF(model, z);
    };

    // Count actors carrying meaningful effort on more than one option.
    const CheckFn countSplits = [R](const VIResult& r) -> CheckResult {
        const SaoeSolution sol = saoeDecode(r, R.rows(), R.cols());
        int splitActors = 0;
        for (Index i = 0; i < R.rows(); ++i) {
            int active = 0;
            for (Index j = 0; j < R.cols(); ++j) {
                active += (1.0 < sol.e(i, j)) ? 1 : 0;   // > 1 unit of effort
            }
            splitActors += (1 < active) ? 1 : 0;
        }
        char buf[96];
        std::snprintf(buf, sizeof buf,
                      "%d actor(s) split effort across options (interior "
                      "structure if > 0)", splitActors);
        return CheckResult{ true, string(buf) };
    };

    const SolveFn solve = [&](const VectorXd& z0) -> VIResult {
        return fbsHyz04(z0, F, projectNonnegative, 1.0e-10, 60000, 5000,
                        FbsHyz04Params{},
                        [](int iter, int iterMax, double mag, double tol) {
                            std::printf("    fbs(eps=0.1) iter %d/%d: residual^2 %.3e (tol %.1e)\n",
                                        iter, iterMax, mag, tol);
                            std::fflush(stdout);
                        });
    };
    const int failures = runCase("fbs (SAOE, pmedemo eps = 0.1)", solve,
                                 saoeDefaultStart(R, S),
                                 { saoePrintDecoded(R), countSplits,
                                   saoeCheckFeasible(R, S),
                                   checkConvergedFlag() });
    EXPECT_EQ(0, failures)
        << "FBS at eps = 0.1 did not converge to a feasible allocation";
}
// Copyright Ben Paul Wise. All Rights Reserved.

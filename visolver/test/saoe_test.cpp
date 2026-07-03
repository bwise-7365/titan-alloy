// Copyright Ben Paul Wise. All Rights Reserved.
#include "saoe.hpp"
#include "utils.hpp"

#include <Eigen/Dense>
#include <cmath>
#include <cstdio>
#include <exception>
#include <random>

using namespace VINCP;
using std::printf;

// SAOE test on a fixed 6-actor x 10-option instance. The reference equilibrium E
// (below) is the allocation the earlier C++ implementation and the well-regarded
// NPLEC and PATH solvers produced, and which bsHe94b reproduces here. The pass criterion
// is therefore CORRECTNESS: the returned allocation must be feasible (e >= 0 and, per
// actor, sum_j e_ij <= S_i) AND close to E in the root-mean-square-error sense
// (RMSE over all M*N efforts <= rmseTol). This game is non-monotone with multiple
// KKT points, so not every inner solver reaches E (dHan06 may settle in a different
// basin); we therefore pass iff at least ONE inner solver (dHan06 or bsHe94b) lands
// on E. Both inner solvers are run and the input and result tables are always
// printed so an off-target, infeasible, or divergent run can be examined.
int main() {
    VINCP::ScopedUtcTimer timer("saoe_test");
    const bool   latex      = false;   // ASCII output for the test log
    const bool   repeatable = true;    // true = fixed deterministic start, so the RMSE
                                       // grade against E below is reproducible;
                                       // false = random start via makeSeed(0), to explore
                                       // which equilibrium the solver reaches each run
    const double feasTol    = 1.0e-3;  // tolerance for the feasibility check
    const double rmseTol    = 1.0e-2;  // max RMSE (in effort units, over all M*N entries)
                                       // between the returned allocation and the reference
                                       // equilibrium E for a solver to count as correct
    const bool   std_alloceff = true;  // use the standard test data from alloceff01cm.gms

    const int M = 6;   // actors
    const int N = 10;  // options

    MatrixXd R(M, N);  // rewards to actors
    VectorXd S(M);           // strengths of actors
    MatrixXd E(M, N);  // risk-neutral distribution of effort


    if (std_alloceff) {  // solved as alloceff01cm.gms by NPLEC and by PATH
    R << 0.00,  181.42,  -50.43,  -26.32,  256.02,  -21.27, -132.68,  -65.12,  131.40,   14.54,
         0.00,   31.24,  -46.53,  122.90,   39.47,  -12.94,   50.32,   70.03,    8.34, -109.78,
         0.00,  -30.11,  -48.64,  -56.84,  -51.50,  -80.42, -130.30,  -54.20,   -8.75,   51.97,
         0.00,   29.12,  160.04,  -27.68,   91.49,   80.93,  117.95,   27.88,   33.62,   72.34,
         0.00, -199.26, -234.61,   67.80, -319.57,  270.83,  234.85,  -14.91, -236.86, -103.50,
         0.00,   78.66,  -22.12,   14.25,  109.23,  -17.30,  -31.16,  -42.61,   31.46,  -46.13;

    E << 0.00,    0.00,    0.00,    0.00,   68.00,    0.00,    0.00,    0.00,    0.00,    0.00,
         0.00,    0.00,    0.00,   66.00,    0.00,    0.00,    0.00,    0.00,    0.00,    0.00,
         0.00,    0.00,    0.00,    0.00,    0.00,    0.00,    0.00,    0.00,    0.00,  125.00,
         0.00,    0.00,  101.00,    0.00,    0.00,    0.00,    0.00,    0.00,    0.00,    0.00,
         0.00,    0.00,    0.00,    0.00,    0.00,  127.00,    0.00,    0.00,    0.00,    0.00,
         0.00,    0.00,    0.00,    0.00,    96.00,    0.00,   0.00,    0.00,    0.00,    0.00;

    S << 68.0, 66.0, 125.0, 101.0, 127.0, 96.0;
    }
    else { // apparently harder, but solved as alloceff01cmB.gms by NPLEC and by PATH

            R <<  0.0,   42.9, -196.3, -263.6,   -8.8,  -87.9,  399.5,  102.6, -199.6,  211.0,
                  0.0, -387.1,   65.9,  171.7,  102.6,  367.8, -205.2, -474.5,  470.3,   59.2,
                  0.0,  294.5,  129.7,  102.2,  -80.4, -235.0, -202.4,  313.3, -217.4, -252.1,
                  0.0,  169.4, -268.6, -385.4,  -41.6, -220.7,  567.7,  271.1, -392.9,  250.1,
                  0.0, -321.1,   70.1,  162.4,   84.8,  308.9, -201.0, -397.6,  402.1,   31.3,
                  0.0,  165.6,  157.0,  166.3,  -46.4, -111.2, -281.5,  153.9,  -56.6, -238.5;



             E << 0.0,    0.0,    0.0,    0.0,    0.0,    0.0,  195.0,    0.0,    0.0,    0.00,
                  0.0,    0.0,    0.0,    0.0,    0.0,    0.0,    0.0,    0.0,   69.0,    0.00,
                  0.0,    0.0,    0.0,    0.0,    0.0,    0.0,    0.0,   56.0,    0.0,    0.00,
                  0.0,    0.0,    0.0,    0.0,    0.0,    0.0,  120.0,    0.0,    0.0,    0.00,
                  0.0,    0.0,    0.0,    0.0,    0.0,    0.0,    0.0,    0.0,   77.0,    0.00,
                  0.0,    0.0,    0.0,  106.0,    0.0,    0.0,    0.0,    0.0,    0.0,    0.00;

        S << 195.0,  69.0,   56.0,  120.0,   77.0,  106.0;
    }

    VINCP::printComment(latex, "SAOE test: 6 actors x 10 options (pass = a feasible allocation)");
    VINCP::saoePrintInputs(R, S, latex);

    // Starting point: empty => saoe() uses its deterministic default (repeatable);
    // otherwise a random start from a fresh makeSeed(0) so runs explore different
    // equilibria. Both inner solvers below start from the same point.
    VectorXd z0;
    if (!repeatable) {
        std::mt19937_64 rng(VINCP::makeSeed(0, true));
        z0 = VINCP::saoeRandomStart(R, S, rng);
    }
    VINCP::printComment(latex, repeatable ? "start: deterministic (repeatable)"
                                          : "start: random (makeSeed(0))");

    struct Method { const char* name; VINCP::InnerMethod inner; };
    const Method methods[] = {
        { "dHan06",  VINCP::InnerMethod::Han },
        { "bsHe94b", VINCP::InnerMethod::He  },
    };

    bool anyMatched = false;   // some solver was both feasible and close to E
    // Check: decode the allocation, print the effort table, and require feasibility
    // (e >= 0, per-actor budget) AND a small RMSE against the reference equilibrium E.
    const CheckFn saoeCheck = [&](const VIResult& r) -> CheckResult {
        const SaoeSolution sol = saoeDecode(r, M, N);
        saoePrintSolution(R, sol.e, saoeEps(R), latex);
        const double maxNeg   = -sol.e.minCoeff();
        const double maxOver  = (sol.e.rowwise().sum() - S).maxCoeff();
        const bool   feasible = (maxNeg <= feasTol) && (maxOver <= feasTol);
        const double rmse     = std::sqrt((sol.e - E).array().square().sum()
                                          / static_cast<double>(M * N));
        const bool   onTarget = (rmse <= rmseTol);
        char buf[192];
        std::snprintf(buf, sizeof buf,
                      "feasible=%s (maxNeg %.2e, over %.2e), RMSE vs E = %.4e (tol %.2e) -> %s",
                      feasible ? "true" : "false", maxNeg, maxOver, rmse, rmseTol,
                      onTarget ? "on target" : "off target");
        return CheckResult{ feasible && onTarget, std::string(buf) };
    };

    // Any-of: pass if at least one inner solver reproduces E.
    for (const Method& m : methods) {
        SaoeParams params;
        params.innerMethod          = m.inner;
        params.logInnerDefiniteness = true;   // probe each outer Jacobian's monotonicity
        const SolveFn solve = [&](const VectorXd& start){ return saoe(R, S, params, start); };
        if (runCase(m.name, solve, z0, { saoeCheck }) == 0) {
            anyMatched = true;
        }
    }

    printf("\n%s\n", anyMatched
                     ? "PASS (at least one inner solver reproduced the reference equilibrium E)"
                     : "FAIL (no inner solver produced a feasible allocation matching E within RMSE tol)");
    return anyMatched ? 0 : 1;
}
// Copyright Ben Paul Wise. All Rights Reserved.

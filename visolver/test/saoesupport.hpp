// Copyright Ben Paul Wise. All Rights Reserved.
#ifndef VIMCP_SAOESUPPORT_HPP
#define VIMCP_SAOESUPPORT_HPP

// ============================================================================
// Test-only support shared by the SAOE solver tests (saoe_chain_test,
// saoe_risk_test): the reference 6-actor x 10-option instance with its known
// equilibrium E, the alternating-chain runner configured as the SAOE tests
// run it, and the shared feasibility / reference-match / printing checks.
// Extracted so the risk-averse variant tests drop in as data + assertions
// rather than copied harness code.
// ============================================================================

#include "alternatingchain.hpp"
#include "josephynewton.hpp"
#include "saoe.hpp"
#include "semismoothnewton.hpp"
#include "utils.hpp"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <numeric>
#include <random>
#include <stdexcept>
#include <vector>

namespace VIMCP {

    // Chain configuration shared by the SAOE tests (squared-norm tolerances).
    constexpr double kSaoeChainMagTol      = 1.0e-10;
    constexpr int    kSaoeSsnIterMax       = 300;
    constexpr int    kSaoeOuterIterMax     = 50;    // JN outer cap (globalizer)
    constexpr double kSaoeInnerMagTol      = 1.0e-12;
    constexpr int    kSaoeIpmIterMax       = 200;   // counts LU factorizations
    constexpr int    kSaoeJnStallIterMax   = 5;
    constexpr int    kSaoeChainRoundsMax   = 8;
    constexpr double kSaoeChainPerturb     = 0.1;

    // Feasibility / reference-match bars (the saoe_test values).
    constexpr double kSaoeFeasTol = 1.0e-3;
    constexpr double kSaoeRmseTol = 1.0e-2;

    // The alternating chain in the SAOE configuration: Josephy-Newton over
    // the interior-point inner solver as globalizer (under the no-progress
    // cutoff), the semismooth solver with nonmonotone memory as finisher.
    inline SolveFn
    makeSaoeChain(const VIModel& model)
    {
        return [model](const VectorXd& z0) -> VIResult {
            JosephyNewtonParams jnParams;
            jnParams.outerTol     = kSaoeChainMagTol;
            jnParams.outerIterMax = kSaoeOuterIterMax;
            jnParams.stallIterMax = kSaoeJnStallIterMax;
            const InnerSolver inner = makeMehrotraIpmSolver(
                model.n, kSaoeInnerMagTol, kSaoeIpmIterMax, 0);
            const StageSolver globalizer = [model, inner, jnParams](const VectorXd& start) {
                return solveVI(model, start, inner, jnParams);
            };
            SemismoothNewtonParams ssnParams;
            ssnParams.magTol            = kSaoeChainMagTol;
            ssnParams.iterMax           = kSaoeSsnIterMax;
            ssnParams.nonmonotoneMemory = 4;
            const StageSolver finisher = [model, ssnParams](const VectorXd& start) {
                return semismoothNewtonSolve(model, start, ssnParams);
            };

            AlternatingChainParams chainParams;
            chainParams.magTol       = kSaoeChainMagTol;
            chainParams.roundsMax    = kSaoeChainRoundsMax;
            chainParams.perturbScale = kSaoeChainPerturb;

            const ChainStageLogger stageLog =
                [](int round, const char* stage, double stageResidual,
                   double bestResidual, const string& note) {
                    if (note.empty()) {
                        std::printf("    saoe-chain round %d %s: residual^2 %.3e (best %.3e)\n",
                                    round, stage, stageResidual, bestResidual);
                    }
                    else {
                        std::printf("    saoe-chain round %d %s threw (stalled stage): %s\n",
                                    round, stage, note.c_str());
                    }
                    std::fflush(stdout);
                    return;
                };

            return alternatingChainSolve(model, z0, globalizer, finisher,
                                         chainParams, stageLog);
        };
    }

    // Feasibility check: efforts non-negative and per-actor budgets respected.
    inline CheckFn
    saoeCheckFeasible(const MatrixXd& R, const VectorXd& S)
    {
        const Index M = R.rows();
        const Index N = R.cols();
        return [M, N, S](const VIResult& r) -> CheckResult {
            const SaoeSolution sol = saoeDecode(r, M, N);
            const double maxNeg  = -sol.e.minCoeff();
            const double maxOver = (sol.e.rowwise().sum() - S).maxCoeff();
            char buf[128];
            std::snprintf(buf, sizeof buf,
                          "feasibility: max negativity %.2e, max budget overrun %.2e (tol %.1e)",
                          maxNeg, maxOver, kSaoeFeasTol);
            return CheckResult{ maxNeg <= kSaoeFeasTol && maxOver <= kSaoeFeasTol,
                                string(buf) };
        };
    }

    // Report-only: print the decoded allocation, probabilities, and utilities.
    inline CheckFn
    saoePrintDecoded(const MatrixXd& R)
    {
        const Index M = R.rows();
        const Index N = R.cols();
        return [R, M, N](const VIResult& r) -> CheckResult {
            const SaoeSolution sol = saoeDecode(r, M, N);
            saoePrintSolution(R, sol.e, saoeEps(R), /*latex=*/false);
            const VectorXd u = saoeUtilities(R, sol.e, saoeEps(R));
            std::printf("  utilities:");
            for (Index i = 0; i < M; ++i) {
                std::printf(" %8.3f", u(i));
            }
            std::printf("\n");
            std::fflush(stdout);
            return CheckResult{ true, "solution printed above" };
        };
    }

    // The reference instance (saoe_test's standard data = alloceff01cm.gms)
    // and its known equilibrium E (the PATH / NPLEC / earlier-solver answer).
    constexpr int kSaoeRefActors  = 6;
    constexpr int kSaoeRefOptions = 10;

    inline void
    saoeReferenceInstance(MatrixXd& R, VectorXd& S, MatrixXd& E)
    {
        R.resize(kSaoeRefActors, kSaoeRefOptions);
        E.resize(kSaoeRefActors, kSaoeRefOptions);
        S.resize(kSaoeRefActors);
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
             0.00,    0.00,    0.00,    0.00,   96.00,    0.00,    0.00,    0.00,    0.00,    0.00;

        S << 68.0, 66.0, 125.0, 101.0, 127.0, 96.0;
        return;
    }

    // Gating check for the reference instance: land on E (RMSE over all
    // M*N efforts, saoe_test's bar).
    inline CheckFn
    saoeCheckMatchesReference(const MatrixXd& E)
    {
        const Index M = E.rows();
        const Index N = E.cols();
        return [E, M, N](const VIResult& r) -> CheckResult {
            const SaoeSolution sol = saoeDecode(r, M, N);
            const double rmse = std::sqrt((sol.e - E).array().square().sum()
                                          / static_cast<double>(M * N));
            char buf[96];
            std::snprintf(buf, sizeof buf,
                          "RMSE vs reference equilibrium E = %.4e (tol %.1e)",
                          rmse, kSaoeRmseTol);
            return CheckResult{ rmse <= kSaoeRmseTol, string(buf) };
        };
    }

    // ------------------------------------------------------------------
    // The expanded 20 x 12 instance (shared by saoe_chain_test and
    // saoe_risk_test so both exercise the SAME two cases). Recipe (Ben,
    // 2026-07-06): rewards U[-50, 100] with EXACTLY one third of the
    // entries zero (80 of 240); every column at least one negative and one
    // positive; every row at least two positive and two negative;
    // strengths U[10, 100]. Row quotas hold by construction (two slots per
    // row forced to each sign), column quotas by targeted redraws of
    // UNFORCED slots (cannot break the row quotas), with a fresh zero mask
    // if repair is ever impossible. saoe_chain_test re-verifies every
    // postcondition on the finished matrix.
    // ------------------------------------------------------------------
    constexpr int           kSaoeExpActors  = 20;
    constexpr int           kSaoeExpOptions = 12;
    constexpr int           kSaoeExpZeros   = (kSaoeExpActors * kSaoeExpOptions) / 3;
    constexpr std::uint32_t kSaoeExpandedSeed = 20260706u;
    constexpr double kSaoeRewardLo   = -50.0;
    constexpr double kSaoeRewardHi   = 100.0;
    constexpr double kSaoeStrengthLo = 10.0;
    constexpr double kSaoeStrengthHi = 100.0;
    constexpr int    kSaoeRowPosQuota = 2;   // per row: >= 2 positive, >= 2 negative
    constexpr int    kSaoeColPosQuota = 1;   // per column: >= 1 positive, >= 1 negative
    constexpr int    kSaoeGenAttemptMax = 1000;

    // Draw from (lo, hi) rejecting an exact zero, so sign quotas are unambiguous.
    inline double
    saoeDrawNonzero(std::mt19937& rng, double lo, double hi)
    {
        std::uniform_real_distribution<double> dist(lo, hi);
        double v = 0.0;
        do {
            v = dist(rng);
        } while (0.0 == v);
        return v;
    }

    inline MatrixXd
    saoeMakeExpandedRewards(std::uint32_t seed)
    {
        std::mt19937 rng(seed);
        const int total = kSaoeExpActors * kSaoeExpOptions;

        for (int attempt = 0; attempt < kSaoeGenAttemptMax; ++attempt) {
            // Zero mask: exactly kSaoeExpZeros zeros, positioned so every row
            // keeps >= 4 nonzeros (its sign quotas) and every column >= 2.
            std::vector<int> order(total);
            std::iota(order.begin(), order.end(), 0);
            std::shuffle(order.begin(), order.end(), rng);
            std::vector<bool> zeroP(total, false);
            for (int k = 0; k < kSaoeExpZeros; ++k) {
                zeroP[static_cast<size_t>(order[static_cast<size_t>(k)])] = true;
            }
            bool maskOkP = true;
            for (int i = 0; maskOkP && i < kSaoeExpActors; ++i) {
                int nonzero = 0;
                for (int j = 0; j < kSaoeExpOptions; ++j) {
                    nonzero += zeroP[static_cast<size_t>(i * kSaoeExpOptions + j)] ? 0 : 1;
                }
                maskOkP = (2 * kSaoeRowPosQuota <= nonzero);
            }
            for (int j = 0; maskOkP && j < kSaoeExpOptions; ++j) {
                int nonzero = 0;
                for (int i = 0; i < kSaoeExpActors; ++i) {
                    nonzero += zeroP[static_cast<size_t>(i * kSaoeExpOptions + j)] ? 0 : 1;
                }
                maskOkP = (2 * kSaoeColPosQuota <= nonzero);
            }
            if (!maskOkP) {
                continue;   // fresh mask
            }

            // Row quotas by construction: per row, shuffle the nonzero slots
            // and force the first two positive, the next two negative; the
            // rest draw from the full range.
            MatrixXd R = MatrixXd::Zero(kSaoeExpActors, kSaoeExpOptions);
            std::vector<bool> forcedP(total, false);
            for (int i = 0; i < kSaoeExpActors; ++i) {
                std::vector<int> slots;
                for (int j = 0; j < kSaoeExpOptions; ++j) {
                    if (!zeroP[static_cast<size_t>(i * kSaoeExpOptions + j)]) {
                        slots.push_back(j);
                    }
                }
                std::shuffle(slots.begin(), slots.end(), rng);
                for (size_t s = 0; s < slots.size(); ++s) {
                    const int j = slots[s];
                    if (s < static_cast<size_t>(kSaoeRowPosQuota)) {
                        R(i, j) = saoeDrawNonzero(rng, 0.0, kSaoeRewardHi);
                        forcedP[static_cast<size_t>(i * kSaoeExpOptions + j)] = true;
                    }
                    else if (s < static_cast<size_t>(2 * kSaoeRowPosQuota)) {
                        R(i, j) = saoeDrawNonzero(rng, kSaoeRewardLo, 0.0);
                        forcedP[static_cast<size_t>(i * kSaoeExpOptions + j)] = true;
                    }
                    else {
                        R(i, j) = saoeDrawNonzero(rng, kSaoeRewardLo, kSaoeRewardHi);
                    }
                }
            }

            // Column quotas by repair: redraw an unforced slot of the right
            // column into the missing sign. Repairs touch only unforced
            // slots, so the row quotas cannot be broken.
            bool repairedOkP = true;
            for (int j = 0; repairedOkP && j < kSaoeExpOptions; ++j) {
                for (int sign = 0; repairedOkP && sign < 2; ++sign) {
                    const bool wantPositiveP = (0 == sign);
                    int have = 0;
                    for (int i = 0; i < kSaoeExpActors; ++i) {
                        const double v = R(i, j);
                        have += (wantPositiveP ? (0.0 < v) : (v < 0.0)) ? 1 : 0;
                    }
                    while (have < kSaoeColPosQuota) {
                        std::vector<int> candidates;
                        for (int i = 0; i < kSaoeExpActors; ++i) {
                            const size_t idx =
                                static_cast<size_t>(i * kSaoeExpOptions + j);
                            const bool wrongSignP =
                                wantPositiveP ? (R(i, j) < 0.0) : (0.0 < R(i, j));
                            if (!zeroP[idx] && !forcedP[idx] && wrongSignP) {
                                candidates.push_back(i);
                            }
                        }
                        if (candidates.empty()) {
                            repairedOkP = false;   // restart with a fresh mask
                            break;
                        }
                        std::uniform_int_distribution<size_t> pick(0, candidates.size() - 1);
                        const int i = candidates[pick(rng)];
                        R(i, j) = wantPositiveP
                                      ? saoeDrawNonzero(rng, 0.0, kSaoeRewardHi)
                                      : saoeDrawNonzero(rng, kSaoeRewardLo, 0.0);
                        forcedP[static_cast<size_t>(i * kSaoeExpOptions + j)] = true;
                        ++have;
                    }
                }
            }
            if (repairedOkP) {
                return R;
            }
        }
        throw std::runtime_error(
            "saoeMakeExpandedRewards: could not satisfy the sign quotas (seed pathology).");
    }

    inline VectorXd
    saoeMakeExpandedStrengths(std::uint32_t seed)
    {
        // Separate stream from the rewards so a change to the reward recipe
        // cannot silently reshuffle the strengths.
        std::mt19937 rng(seed + 1u);
        std::uniform_real_distribution<double> dist(kSaoeStrengthLo, kSaoeStrengthHi);
        VectorXd S(kSaoeExpActors);
        for (Index i = 0; i < kSaoeExpActors; ++i) {
            S(i) = dist(rng);
        }
        return S;
    }

    // ------------------------------------------------------------------
    // THE shared case list: every SAOE test iterates this one source of
    // truth, so a change to either setup -- or a future third case -- reaches
    // all of them automatically. The reference case additionally has the
    // known equilibrium E (saoeReferenceInstance); the expanded case has
    // none, so gates on it are convergence + feasibility only.
    // ------------------------------------------------------------------
    struct SaoeCase {
        string name;
        MatrixXd R;
        VectorXd S;
        bool hasReferenceP = false;   // E is known (the reference case only)
    };

    inline vector<SaoeCase>
    saoeSharedCases()
    {
        vector<SaoeCase> cases;

        SaoeCase reference;
        reference.name = "reference 6x10";
        MatrixXd E;
        saoeReferenceInstance(reference.R, reference.S, E);
        reference.hasReferenceP = true;
        cases.push_back(reference);

        SaoeCase expanded;
        expanded.name = "expanded 20x12";
        expanded.R = saoeMakeExpandedRewards(kSaoeExpandedSeed);
        expanded.S = saoeMakeExpandedStrengths(kSaoeExpandedSeed);
        cases.push_back(expanded);

        return cases;
    }

} // namespace VIMCP

#endif // VIMCP_SAOESUPPORT_HPP
// Copyright Ben Paul Wise. All Rights Reserved.

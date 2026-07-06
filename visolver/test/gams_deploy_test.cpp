// Copyright Ben Paul Wise. All Rights Reserved.
#include "mcpengines.hpp"
#include "vincp.hpp"

#include <Eigen/Dense>
#include <gtest/gtest.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>
#include <exception>
#include <limits>
#include <numeric>
#include <utility>

using namespace VINCP;

// ============================================================================
// Faithful translation of deploy_v07.gms (sheridan/deployment): a two-sided
// pre-position / deploy / interdict game -- movers, attackers, and defenders
// with smooth ratio combat en route and at the conflict locations -- as a
// mixed nonlinear complementarity problem over K = R^n x R_+^m.
//
// GAMS -> VINCP mapping (Model interdict, pairings from its / ... / list):
//   free block x (n = 4) = [ alphaR | alphaB | lambdaR | lambdaB ], the
//     multipliers of the four =e= constraints; H rows are those constraints
//     (probabilities sum to one, pre-positioning exhausts the totals).
//   nonneg block y (m = 446) = every Positive variable, packed in the model
//     statement's pairing order (offsets below); G rows are the =g= equations
//     paired with them -- stationarity (cost >= exact marginal benefit) for
//     the primal variables, primal feasibility for the multipliers.
//
// The survivor macros and their chain-rule gradients (BA2x, fRsurv, dfR_df,
// dfR_dE, dfR_dA and the Blue mirrors -- Maxima-verified in the author's
// v07_check.mac) are transcribed literally in computeStage() below; every
// denominator is guarded by myEps = 0.01 exactly as in the .gms.
//
// PASS CRITERION (upgraded 2026-07-06 after the alternating chain first
// solved the problem and the author confirmed the answer matches the GAMS
// solution): the MCP is NONMONOTONE (mixed curvature: concave from strength,
// convex from weakness -- "mass or abstain") with multiple Nash equilibria,
// so the test passes iff AT LEAST ONE engine row converges AND lands on the
// GAMS-verified equilibrium (kRefPr / kRefPb / kRefPayoff* below):
//     pr = (0, 0, 0.505, 0.495, 0)   -- support {RS3, RS4}
//     pb = (0, 0.600, 0, 0, 0.400)   -- support {BS2, BS5}; pb(BS2) = rho,
//                                       Blue's probability cap is ACTIVE
//     PayoffR = 58.069, PayoffB = 60.717
// Known-good: ONLY the alternating chain converges (round 4, residual^2
// 6.0e-15, ~18 s); every standalone engine fails, which is the point of the
// chain. Strategy probabilities, payoffs, and a natural-residual breakdown
// are printed for every returned point either way.
//
// NOTES
//  - The .gms warns that interior starts are ESSENTIAL: the survivor function
//    h(x) = x^2/(x + c) has h'(0) = 0, so a route at zero shows no first-order
//    reason to open. initialPoint() reproduces the GAMS .L levels exactly.
//  - Dimension is 450, so the finite-difference Jacobian costs 4 * 450 = 1800
//    F-evaluations per Newton step: expect seconds per engine row in Release
//    and noticeably longer in Debug.
//  - dHan06 is not in the engine list: it refactors its metric every inner
//    iteration, which is minutes-scale at this dimension. Add McpEngine::JnHan
//    to the list to try it anyway.
// ============================================================================

namespace {
    // Sets (deploy_v07.gms lines 54-59).
    constexpr int kNumRedLoc    = 3;   // m: RL1..RL3
    constexpr int kNumBlueLoc   = 3;   // n: BL1..BL3
    constexpr int kNumConLoc    = 4;   // k: CL1..CL4
    constexpr int kNumRedStrat  = 5;   // r: RS1..RS5
    constexpr int kNumBlueStrat = 5;   // b: BS1..BS5

    // Scalars (lines 66-95).
    constexpr double kTotalR  = 35.0;   // Red movers available
    constexpr double kTotalB  = 37.0;   // Blue movers available
    constexpr double kLogR    = 421.0;  // Red ton-mile budget
    constexpr double kLogB    = 410.0;  // Blue ton-mile budget
    constexpr double kEps     = 0.01;   // myEps, guards every denominator
    constexpr double kRho     = 0.6;    // max probability of any one strategy
    constexpr double kTotREsc = 10.0;   // Red escort pool (per strategy)
    constexpr double kTotRAtt = 12.0;   // Red attacker pool
    constexpr double kTotBEsc = 12.0;   // Blue escort pool
    constexpr double kTotBAtt = 11.0;   // Blue attacker pool

    // Tables (lines 100-131).
    const double kVr[kNumRedStrat][kNumConLoc] = {
        { 31.8, 24.2, 15.4, 28.4 },
        { 22.7, 15.3, 26.6, 35.3 },
        { 10.8, 30.1, 22.5, 36.6 },
        { 15.5, 12.6, 33.2, 38.7 },
        { 32.9, 34.7, 24.1,  8.3 },
    };
    const double kDistR[kNumRedLoc][kNumConLoc] = {
        { 11.0, 13.0, 17.0, 20.0 },
        { 17.0, 13.0, 14.0, 16.0 },
        { 18.0, 16.0, 13.0, 10.0 },
    };
    const double kVb[kNumBlueStrat][kNumConLoc] = {
        { 25.2, 20.1, 13.0, 41.7 },
        { 33.4, 33.1,  8.3, 25.2 },
        { 28.1, 16.8, 25.9, 29.2 },
        { 10.1, 43.6, 14.5, 31.8 },
        { 39.8, 14.4, 28.0, 17.8 },
    };
    const double kDistB[kNumBlueLoc][kNumConLoc] = {
        { 10.0, 13.0, 15.0, 17.0 },
        { 14.0, 12.0, 13.0, 15.0 },
        { 20.0, 18.0, 15.0, 11.0 },
    };

    // Free-block packing (n = 4).
    constexpr Index kAlphaR  = 0;   // _|_ L_M_Red_Prob
    constexpr Index kAlphaB  = 1;   // _|_ L_M_Blue_Prob
    constexpr Index kLambdaR = 2;   // _|_ L_M_Red_Dep
    constexpr Index kLambdaB = 3;   // _|_ L_M_Blue_Dep
    constexpr Index kNumFree = 4;

    // Nonneg-block packing (m = 446), in the model statement's pairing order.
    constexpr Index kRedRouteCount  = kNumRedStrat * kNumRedLoc * kNumConLoc;    // 60
    constexpr Index kBlueRouteCount = kNumBlueStrat * kNumBlueLoc * kNumConLoc;  // 60

    constexpr Index kPrOff     = 0;                                             // pr(r)
    constexpr Index kPbOff     = kPrOff + kNumRedStrat;                         // pb(b)
    constexpr Index kFlowROff  = kPbOff + kNumBlueStrat;                        // flowR(r,m,k)
    constexpr Index kFlowBOff  = kFlowROff + kRedRouteCount;                    // flowB(b,n,k)
    constexpr Index kREsOff    = kFlowBOff + kBlueRouteCount;                   // REs(r,m,k)
    constexpr Index kBEsOff    = kREsOff + kRedRouteCount;                      // BEs(b,n,k)
    constexpr Index kRAtOff    = kBEsOff + kBlueRouteCount;                     // RAt(r,n,k)
    constexpr Index kBAtOff    = kRAtOff + kNumRedStrat * kNumBlueLoc * kNumConLoc;  // BAt(b,m,k)
    constexpr Index kRdOff     = kBAtOff + kNumBlueStrat * kNumRedLoc * kNumConLoc;  // RD(m)
    constexpr Index kBdOff     = kRdOff + kNumRedLoc;                           // BD(n)
    constexpr Index kBetaROff  = kBdOff + kNumBlueLoc;                          // betaR(r)
    constexpr Index kBetaBOff  = kBetaROff + kNumRedStrat;                      // betaB(b)
    constexpr Index kEtaROff   = kBetaBOff + kNumBlueStrat;                     // etaR(r)
    constexpr Index kEtaBOff   = kEtaROff + kNumRedStrat;                       // etaB(b)
    constexpr Index kGammaROff = kEtaBOff + kNumBlueStrat;                      // gammaR(r,m)
    constexpr Index kGammaBOff = kGammaROff + kNumRedStrat * kNumRedLoc;        // gammaB(b,n)
    constexpr Index kMuEROff   = kGammaBOff + kNumBlueStrat * kNumBlueLoc;      // muER(r)
    constexpr Index kMuEBOff   = kMuEROff + kNumRedStrat;                       // muEB(b)
    constexpr Index kMuAROff   = kMuEBOff + kNumBlueStrat;                      // muAR(r)
    constexpr Index kMuABOff   = kMuAROff + kNumRedStrat;                       // muAB(b)
    constexpr Index kNumComp   = kMuABOff + kNumBlueStrat;                      // 446

    constexpr Index prAt(int r)                 { return kPrOff + r; }
    constexpr Index pbAt(int b)                 { return kPbOff + b; }
    constexpr Index flowRAt(int r, int m, int k){ return kFlowROff + (r * kNumRedLoc + m) * kNumConLoc + k; }
    constexpr Index flowBAt(int b, int n, int k){ return kFlowBOff + (b * kNumBlueLoc + n) * kNumConLoc + k; }
    constexpr Index resAt(int r, int m, int k)  { return kREsOff + (r * kNumRedLoc + m) * kNumConLoc + k; }
    constexpr Index besAt(int b, int n, int k)  { return kBEsOff + (b * kNumBlueLoc + n) * kNumConLoc + k; }
    constexpr Index ratAt(int r, int n, int k)  { return kRAtOff + (r * kNumBlueLoc + n) * kNumConLoc + k; }
    constexpr Index batAt(int b, int m, int k)  { return kBAtOff + (b * kNumRedLoc + m) * kNumConLoc + k; }
    constexpr Index rdAt(int m)                 { return kRdOff + m; }
    constexpr Index bdAt(int n)                 { return kBdOff + n; }
    constexpr Index betaRAt(int r)              { return kBetaROff + r; }
    constexpr Index betaBAt(int b)              { return kBetaBOff + b; }
    constexpr Index etaRAt(int r)               { return kEtaROff + r; }
    constexpr Index etaBAt(int b)               { return kEtaBOff + b; }
    constexpr Index gammaRAt(int r, int m)      { return kGammaROff + r * kNumRedLoc + m; }
    constexpr Index gammaBAt(int b, int n)      { return kGammaBOff + b * kNumBlueLoc + n; }
    constexpr Index muERAt(int r)               { return kMuEROff + r; }
    constexpr Index muEBAt(int b)               { return kMuEBOff + b; }
    constexpr Index muARAt(int r)               { return kMuAROff + r; }
    constexpr Index muABAt(int b)               { return kMuABOff + b; }

    // Everything the payoff gradients need for ONE strategy pair (r, b): node
    // survivor totals and the per-route survivor derivatives. Transcribes the
    // .gms macros BA2x/fRsurv/FRnode/denx, dfR_df/dfR_dE/dfR_dA, and the Blue
    // mirrors (lines 160-178).
    using PerCon    = std::array<double, kNumConLoc>;
    using RedRoute  = std::array<std::array<double, kNumConLoc>, kNumRedLoc>;
    using BlueRoute = std::array<std::array<double, kNumConLoc>, kNumBlueLoc>;

    struct StagePair {
        PerCon    frNode;     // FRnode(r,b,k): Red survivors arriving at k
        PerCon    fbNode;     // FBnode(r,b,k)
        PerCon    den;        // denx(r,b,k) = FRnode + FBnode + eps
        RedRoute  dfRdFlow;   // dfR_df:  d fRsurv / d flowR      (own flow)
        RedRoute  dfRdEsc;    // dfR_dE:  d fRsurv / d REs        (own escorts)
        RedRoute  dfRdAtt;    // dfR_dA: -d fRsurv / d BAt        (enemy attackers)
        BlueRoute dfBdFlow;   // dfB_dg and mirrors
        BlueRoute dfBdEsc;
        BlueRoute dfBdAtt;
    };

    StagePair
    computeStage(int r, int b, const VectorXd& y)
    {
        StagePair s{};
        for (int k = 0; k < kNumConLoc; ++k) {
            s.frNode[k] = 0.0;
            s.fbNode[k] = 0.0;
        }
        // Red routes m -> k under (r, b): Blue attackers vs Red escorts, then
        // Red movers vs the surviving attackers.
        for (int m = 0; m < kNumRedLoc; ++m) {
            for (int k = 0; k < kNumConLoc; ++k) {
                const double bat      = y(batAt(b, m, k));
                const double res      = y(resAt(r, m, k));
                const double fr       = y(flowRAt(r, m, k));
                const double escDen   = bat + res + kEps;
                const double ba2      = bat * bat / escDen;            // BA2x
                const double routeDen = fr + ba2 + kEps;
                s.frNode[k] += fr * fr / routeDen;                     // fRsurv
                s.dfRdFlow[m][k] = fr * (fr + 2.0 * ba2 + 2.0 * kEps)
                                   / (routeDen * routeDen);
                s.dfRdEsc[m][k]  = (fr * fr) / (routeDen * routeDen)
                                   * (bat * bat) / (escDen * escDen);
                s.dfRdAtt[m][k]  = (fr * fr) / (routeDen * routeDen)
                                   * bat * (bat + 2.0 * res + 2.0 * kEps)
                                   / (escDen * escDen);
            }
        }
        // Blue routes n -> k under (r, b): the exact mirror.
        for (int n = 0; n < kNumBlueLoc; ++n) {
            for (int k = 0; k < kNumConLoc; ++k) {
                const double rat      = y(ratAt(r, n, k));
                const double bes      = y(besAt(b, n, k));
                const double fb       = y(flowBAt(b, n, k));
                const double escDen   = rat + bes + kEps;
                const double ra2      = rat * rat / escDen;            // RA2x
                const double routeDen = fb + ra2 + kEps;
                s.fbNode[k] += fb * fb / routeDen;                     // fBsurv
                s.dfBdFlow[n][k] = fb * (fb + 2.0 * ra2 + 2.0 * kEps)
                                   / (routeDen * routeDen);
                s.dfBdEsc[n][k]  = (fb * fb) / (routeDen * routeDen)
                                   * (rat * rat) / (escDen * escDen);
                s.dfBdAtt[n][k]  = (fb * fb) / (routeDen * routeDen)
                                   * rat * (rat + 2.0 * bes + 2.0 * kEps)
                                   / (escDen * escDen);
            }
        }
        for (int k = 0; k < kNumConLoc; ++k) {
            s.den[k] = s.frNode[k] + s.fbNode[k] + kEps;
        }
        return s;
    }

    using StageTable =
        std::array<std::array<StagePair, kNumBlueStrat>, kNumRedStrat>;

    StageTable
    computeAllStages(const VectorXd& y)
    {
        StageTable table;
        for (int r = 0; r < kNumRedStrat; ++r) {
            for (int b = 0; b < kNumBlueStrat; ++b) {
                table[r][b] = computeStage(r, b, y);
            }
        }
        return table;
    }

    // H rows: the four =e= constraints, paired with the free block.
    VectorXd
    deployH(const VectorXd& /*x*/, const VectorXd& y)
    {
        VectorXd h(kNumFree);
        double sumPr = 0.0, sumPb = 0.0, sumRd = 0.0, sumBd = 0.0;
        for (int r = 0; r < kNumRedStrat; ++r)  { sumPr += y(prAt(r)); }
        for (int b = 0; b < kNumBlueStrat; ++b) { sumPb += y(pbAt(b)); }
        for (int m = 0; m < kNumRedLoc; ++m)    { sumRd += y(rdAt(m)); }
        for (int n = 0; n < kNumBlueLoc; ++n)   { sumBd += y(bdAt(n)); }
        h(kAlphaR)  = 1.0 - sumPr;        // L_M_Red_Prob
        h(kAlphaB)  = 1.0 - sumPb;        // L_M_Blue_Prob
        h(kLambdaR) = kTotalR - sumRd;    // L_M_Red_Dep
        h(kLambdaB) = kTotalB - sumBd;    // L_M_Blue_Dep
        return h;
    }

    // G rows: stationarity for the primal variables, feasibility for the
    // multipliers -- one block per pairing, in packing order.
    VectorXd
    deployG(const VectorXd& x, const VectorXd& y)
    {
        const StageTable stage = computeAllStages(y);
        VectorXd g(kNumComp);

        // C_B_Red_Prob(r) _|_ pr(r).
        for (int r = 0; r < kNumRedStrat; ++r) {
            double benefit = 0.0;
            for (int b = 0; b < kNumBlueStrat; ++b) {
                const StagePair& s = stage[r][b];
                for (int k = 0; k < kNumConLoc; ++k) {
                    benefit += y(pbAt(b)) * kVr[r][k] * s.frNode[k] / s.den[k];
                }
            }
            g(prAt(r)) = x(kAlphaR) + y(betaRAt(r)) - benefit;
        }

        // C_B_Blue_Prob(b) _|_ pb(b).
        for (int b = 0; b < kNumBlueStrat; ++b) {
            double benefit = 0.0;
            for (int r = 0; r < kNumRedStrat; ++r) {
                const StagePair& s = stage[r][b];
                for (int k = 0; k < kNumConLoc; ++k) {
                    benefit += y(prAt(r)) * kVb[b][k] * s.fbNode[k] / s.den[k];
                }
            }
            g(pbAt(b)) = x(kAlphaB) + y(betaBAt(b)) - benefit;
        }

        // C_B_Red_Flow(r,m,k) _|_ flowR(r,m,k).
        for (int r = 0; r < kNumRedStrat; ++r) {
            for (int m = 0; m < kNumRedLoc; ++m) {
                for (int k = 0; k < kNumConLoc; ++k) {
                    double benefit = 0.0;
                    for (int b = 0; b < kNumBlueStrat; ++b) {
                        const StagePair& s = stage[r][b];
                        benefit += y(pbAt(b)) * kVr[r][k]
                                   * (s.fbNode[k] + kEps) / (s.den[k] * s.den[k])
                                   * s.dfRdFlow[m][k];
                    }
                    g(flowRAt(r, m, k)) = y(etaRAt(r)) * kDistR[m][k]
                                          + y(gammaRAt(r, m))
                                          - y(prAt(r)) * benefit;
                }
            }
        }

        // C_B_Blue_Flow(b,n,k) _|_ flowB(b,n,k).
        for (int b = 0; b < kNumBlueStrat; ++b) {
            for (int n = 0; n < kNumBlueLoc; ++n) {
                for (int k = 0; k < kNumConLoc; ++k) {
                    double benefit = 0.0;
                    for (int r = 0; r < kNumRedStrat; ++r) {
                        const StagePair& s = stage[r][b];
                        benefit += y(prAt(r)) * kVb[b][k]
                                   * (s.frNode[k] + kEps) / (s.den[k] * s.den[k])
                                   * s.dfBdFlow[n][k];
                    }
                    g(flowBAt(b, n, k)) = y(etaBAt(b)) * kDistB[n][k]
                                          + y(gammaBAt(b, n))
                                          - y(pbAt(b)) * benefit;
                }
            }
        }

        // C_B_Red_Esc(r,m,k) _|_ REs(r,m,k).
        for (int r = 0; r < kNumRedStrat; ++r) {
            for (int m = 0; m < kNumRedLoc; ++m) {
                for (int k = 0; k < kNumConLoc; ++k) {
                    double benefit = 0.0;
                    for (int b = 0; b < kNumBlueStrat; ++b) {
                        const StagePair& s = stage[r][b];
                        benefit += y(pbAt(b)) * kVr[r][k]
                                   * (s.fbNode[k] + kEps) / (s.den[k] * s.den[k])
                                   * s.dfRdEsc[m][k];
                    }
                    g(resAt(r, m, k)) = y(muERAt(r)) - y(prAt(r)) * benefit;
                }
            }
        }

        // C_B_Blue_Esc(b,n,k) _|_ BEs(b,n,k).
        for (int b = 0; b < kNumBlueStrat; ++b) {
            for (int n = 0; n < kNumBlueLoc; ++n) {
                for (int k = 0; k < kNumConLoc; ++k) {
                    double benefit = 0.0;
                    for (int r = 0; r < kNumRedStrat; ++r) {
                        const StagePair& s = stage[r][b];
                        benefit += y(prAt(r)) * kVb[b][k]
                                   * (s.frNode[k] + kEps) / (s.den[k] * s.den[k])
                                   * s.dfBdEsc[n][k];
                    }
                    g(besAt(b, n, k)) = y(muEBAt(b)) - y(pbAt(b)) * benefit;
                }
            }
        }

        // C_B_Red_Att(r,n,k) _|_ RAt(r,n,k): Red attackers thin Blue's
        // survivors, raising Red's node share FRnode/den.
        for (int r = 0; r < kNumRedStrat; ++r) {
            for (int n = 0; n < kNumBlueLoc; ++n) {
                for (int k = 0; k < kNumConLoc; ++k) {
                    double benefit = 0.0;
                    for (int b = 0; b < kNumBlueStrat; ++b) {
                        const StagePair& s = stage[r][b];
                        benefit += y(pbAt(b)) * kVr[r][k]
                                   * s.frNode[k] / (s.den[k] * s.den[k])
                                   * s.dfBdAtt[n][k];
                    }
                    g(ratAt(r, n, k)) = y(muARAt(r)) - y(prAt(r)) * benefit;
                }
            }
        }

        // C_B_Blue_Att(b,m,k) _|_ BAt(b,m,k).
        for (int b = 0; b < kNumBlueStrat; ++b) {
            for (int m = 0; m < kNumRedLoc; ++m) {
                for (int k = 0; k < kNumConLoc; ++k) {
                    double benefit = 0.0;
                    for (int r = 0; r < kNumRedStrat; ++r) {
                        const StagePair& s = stage[r][b];
                        benefit += y(prAt(r)) * kVb[b][k]
                                   * s.fbNode[k] / (s.den[k] * s.den[k])
                                   * s.dfRdAtt[m][k];
                    }
                    g(batAt(b, m, k)) = y(muABAt(b)) - y(pbAt(b)) * benefit;
                }
            }
        }

        // C_B_Red_Dep(m) _|_ RD(m) and C_B_Blue_Dep(n) _|_ BD(n).
        for (int m = 0; m < kNumRedLoc; ++m) {
            double sumGamma = 0.0;
            for (int r = 0; r < kNumRedStrat; ++r) {
                sumGamma += y(gammaRAt(r, m));
            }
            g(rdAt(m)) = x(kLambdaR) - sumGamma;
        }
        for (int n = 0; n < kNumBlueLoc; ++n) {
            double sumGamma = 0.0;
            for (int b = 0; b < kNumBlueStrat; ++b) {
                sumGamma += y(gammaBAt(b, n));
            }
            g(bdAt(n)) = x(kLambdaB) - sumGamma;
        }

        // L_M_Red_Rho(r) _|_ betaR(r) and the Blue mirror.
        for (int r = 0; r < kNumRedStrat; ++r) {
            g(betaRAt(r)) = kRho - y(prAt(r));
        }
        for (int b = 0; b < kNumBlueStrat; ++b) {
            g(betaBAt(b)) = kRho - y(pbAt(b));
        }

        // L_M_Red_Log(r) _|_ etaR(r) and the Blue mirror.
        for (int r = 0; r < kNumRedStrat; ++r) {
            double tonMiles = 0.0;
            for (int m = 0; m < kNumRedLoc; ++m) {
                for (int k = 0; k < kNumConLoc; ++k) {
                    tonMiles += kDistR[m][k] * y(flowRAt(r, m, k));
                }
            }
            g(etaRAt(r)) = kLogR - tonMiles;
        }
        for (int b = 0; b < kNumBlueStrat; ++b) {
            double tonMiles = 0.0;
            for (int n = 0; n < kNumBlueLoc; ++n) {
                for (int k = 0; k < kNumConLoc; ++k) {
                    tonMiles += kDistB[n][k] * y(flowBAt(b, n, k));
                }
            }
            g(etaBAt(b)) = kLogB - tonMiles;
        }

        // L_M_Red_Flow(r,m) _|_ gammaR(r,m) and the Blue mirror.
        for (int r = 0; r < kNumRedStrat; ++r) {
            for (int m = 0; m < kNumRedLoc; ++m) {
                double sent = 0.0;
                for (int k = 0; k < kNumConLoc; ++k) {
                    sent += y(flowRAt(r, m, k));
                }
                g(gammaRAt(r, m)) = y(rdAt(m)) - sent;
            }
        }
        for (int b = 0; b < kNumBlueStrat; ++b) {
            for (int n = 0; n < kNumBlueLoc; ++n) {
                double sent = 0.0;
                for (int k = 0; k < kNumConLoc; ++k) {
                    sent += y(flowBAt(b, n, k));
                }
                g(gammaBAt(b, n)) = y(bdAt(n)) - sent;
            }
        }

        // Escort / attacker pool limits _|_ their shadow prices.
        for (int r = 0; r < kNumRedStrat; ++r) {
            double used = 0.0;
            for (int m = 0; m < kNumRedLoc; ++m) {
                for (int k = 0; k < kNumConLoc; ++k) {
                    used += y(resAt(r, m, k));
                }
            }
            g(muERAt(r)) = kTotREsc - used;
        }
        for (int b = 0; b < kNumBlueStrat; ++b) {
            double used = 0.0;
            for (int n = 0; n < kNumBlueLoc; ++n) {
                for (int k = 0; k < kNumConLoc; ++k) {
                    used += y(besAt(b, n, k));
                }
            }
            g(muEBAt(b)) = kTotBEsc - used;
        }
        for (int r = 0; r < kNumRedStrat; ++r) {
            double used = 0.0;
            for (int n = 0; n < kNumBlueLoc; ++n) {
                for (int k = 0; k < kNumConLoc; ++k) {
                    used += y(ratAt(r, n, k));
                }
            }
            g(muARAt(r)) = kTotRAtt - used;
        }
        for (int b = 0; b < kNumBlueStrat; ++b) {
            double used = 0.0;
            for (int m = 0; m < kNumRedLoc; ++m) {
                for (int k = 0; k < kNumConLoc; ++k) {
                    used += y(batAt(b, m, k));
                }
            }
            g(muABAt(b)) = kTotBAtt - used;
        }

        return g;
    }

    VIModel
    buildModel()
    {
        VIModel model;
        model.n = kNumFree;
        model.m = kNumComp;
        model.H = deployH;
        model.G = deployG;
        return model;
    }

    // The GAMS .L levels (lines 185-196): flows and allocations start INTERIOR
    // (essential -- see the header note); multipliers and the free block start
    // at the GAMS default level 0.
    VectorXd
    initialPoint()
    {
        VectorXd y0 = VectorXd::Zero(kNumComp);
        for (int m = 0; m < kNumRedLoc; ++m) {
            y0(rdAt(m)) = kTotalR / (kNumRedLoc + 1.0);
        }
        for (int n = 0; n < kNumBlueLoc; ++n) {
            y0(bdAt(n)) = kTotalB / (kNumBlueLoc + 1.0);
        }
        for (int r = 0; r < kNumRedStrat; ++r) {
            y0(prAt(r)) = 1.0 / (kNumRedStrat + 1.0);
        }
        for (int b = 0; b < kNumBlueStrat; ++b) {
            y0(pbAt(b)) = 1.0 / (kNumBlueStrat + 1.0);
        }
        const double routes = static_cast<double>(kNumRedLoc * kNumConLoc);  // card(m)*card(k)
        for (int r = 0; r < kNumRedStrat; ++r) {
            for (int m = 0; m < kNumRedLoc; ++m) {
                for (int k = 0; k < kNumConLoc; ++k) {
                    y0(flowRAt(r, m, k)) = kTotalR / routes;
                    y0(resAt(r, m, k))   = kTotREsc / (routes + 1.0);
                }
            }
            for (int n = 0; n < kNumBlueLoc; ++n) {
                for (int k = 0; k < kNumConLoc; ++k) {
                    y0(ratAt(r, n, k)) = kTotRAtt / (routes + 1.0);
                }
            }
        }
        for (int b = 0; b < kNumBlueStrat; ++b) {
            for (int n = 0; n < kNumBlueLoc; ++n) {
                for (int k = 0; k < kNumConLoc; ++k) {
                    y0(flowBAt(b, n, k)) = kTotalB / routes;
                    y0(besAt(b, n, k))   = kTotBEsc / (routes + 1.0);
                }
            }
            for (int m = 0; m < kNumRedLoc; ++m) {
                for (int k = 0; k < kNumConLoc; ++k) {
                    y0(batAt(b, m, k)) = kTotBAtt / (routes + 1.0);
                }
            }
        }

        VectorXd z0(kNumFree + kNumComp);
        z0 << VectorXd::Zero(kNumFree), y0;
        return z0;
    }

    // Report-only check: print the mixed strategies and the expected payoffs
    //   PayoffR = sum_{r,b} pr(r) pb(b) sum_k Vr(r,k) FRnode(r,b,k)/den(r,b,k)
    // (and the Blue mirror) for eyeball comparison with the GAMS listing.
    CheckFn
    printGamsOutputs()
    {
        return [](const VIResult& res) -> CheckResult {
            const VectorXd y = res.z.tail(kNumComp);
            std::printf("  pr =");
            for (int r = 0; r < kNumRedStrat; ++r) {
                std::printf(" %7.3f", y(prAt(r)));
            }
            std::printf("\n  pb =");
            for (int b = 0; b < kNumBlueStrat; ++b) {
                std::printf(" %7.3f", y(pbAt(b)));
            }
            const StageTable stage = computeAllStages(y);
            double payoffR = 0.0, payoffB = 0.0;
            for (int r = 0; r < kNumRedStrat; ++r) {
                for (int b = 0; b < kNumBlueStrat; ++b) {
                    const StagePair& s = stage[r][b];
                    const double pp = y(prAt(r)) * y(pbAt(b));
                    for (int k = 0; k < kNumConLoc; ++k) {
                        payoffR += pp * kVr[r][k] * s.frNode[k] / s.den[k];
                        payoffB += pp * kVb[b][k] * s.fbNode[k] / s.den[k];
                    }
                }
            }
            std::printf("\n  PayoffR = %.3f   PayoffB = %.3f\n", payoffR, payoffB);
            return CheckResult{ true, "GAMS output parameters printed above" };
        };
    }

    // GAMS-style name of row/variable 'full' in the FULL z packing (free block
    // first, then y in pairing order) -- so the residual breakdown below reads
    // directly against the .gms equation listing.
    string
    rowName(Index full)
    {
        char buf[48];
        if (kNumFree > full) {
            const char* freeNames[kNumFree] = { "alphaR", "alphaB", "lambdaR", "lambdaB" };
            return string(freeNames[full]);
        }
        const Index i = full - kNumFree;
        // (strategy, location, conflict) triple blocks share one formatter.
        const auto triple = [&buf](const char* base, Index local,
                                   const char* stratTag, const char* locTag,
                                   int locCount) -> string {
            const int strat = static_cast<int>(local) / (locCount * kNumConLoc);
            const int rem   = static_cast<int>(local) % (locCount * kNumConLoc);
            std::snprintf(buf, sizeof buf, "%s(%s%d,%s%d,CL%d)", base,
                          stratTag, strat + 1, locTag, rem / kNumConLoc + 1,
                          rem % kNumConLoc + 1);
            return string(buf);
        };
        const auto single = [&buf](const char* base, const char* tag,
                                   Index local) -> string {
            std::snprintf(buf, sizeof buf, "%s(%s%d)", base, tag,
                          static_cast<int>(local) + 1);
            return string(buf);
        };
        if (i < kPbOff)     { return single("pr", "RS", i - kPrOff); }
        if (i < kFlowROff)  { return single("pb", "BS", i - kPbOff); }
        if (i < kFlowBOff)  { return triple("flowR", i - kFlowROff, "RS", "RL", kNumRedLoc); }
        if (i < kREsOff)    { return triple("flowB", i - kFlowBOff, "BS", "BL", kNumBlueLoc); }
        if (i < kBEsOff)    { return triple("REs", i - kREsOff, "RS", "RL", kNumRedLoc); }
        if (i < kRAtOff)    { return triple("BEs", i - kBEsOff, "BS", "BL", kNumBlueLoc); }
        if (i < kBAtOff)    { return triple("RAt", i - kRAtOff, "RS", "BL", kNumBlueLoc); }
        if (i < kRdOff)     { return triple("BAt", i - kBAtOff, "BS", "RL", kNumRedLoc); }
        if (i < kBdOff)     { return single("RD", "RL", i - kRdOff); }
        if (i < kBetaROff)  { return single("BD", "BL", i - kBdOff); }
        if (i < kBetaBOff)  { return single("betaR", "RS", i - kBetaROff); }
        if (i < kEtaROff)   { return single("betaB", "BS", i - kBetaBOff); }
        if (i < kEtaBOff)   { return single("etaR", "RS", i - kEtaROff); }
        if (i < kGammaROff) { return single("etaB", "BS", i - kEtaBOff); }
        if (i < kGammaBOff) {
            const Index local = i - kGammaROff;
            std::snprintf(buf, sizeof buf, "gammaR(RS%d,RL%d)",
                          static_cast<int>(local) / kNumRedLoc + 1,
                          static_cast<int>(local) % kNumRedLoc + 1);
            return string(buf);
        }
        if (i < kMuEROff) {
            const Index local = i - kGammaBOff;
            std::snprintf(buf, sizeof buf, "gammaB(BS%d,BL%d)",
                          static_cast<int>(local) / kNumBlueLoc + 1,
                          static_cast<int>(local) % kNumBlueLoc + 1);
            return string(buf);
        }
        if (i < kMuEBOff)   { return single("muER", "RS", i - kMuEROff); }
        if (i < kMuAROff)   { return single("muEB", "BS", i - kMuEBOff); }
        if (i < kMuABOff)   { return single("muAR", "RS", i - kMuAROff); }
        return single("muAB", "BS", i - kMuABOff);
    }

    // Reference equilibrium (author-confirmed 2026-07-06 to match the GAMS
    // solution; found by the alternating chain at residual^2 6.0e-15). Values
    // quoted to the confirming run's 3 decimals; tolerances sit above that
    // rounding while rejecting any other equilibrium (basins differ by whole
    // units in these outputs).
    const double kRefPr[kNumRedStrat]  = { 0.000, 0.000, 0.505, 0.495, 0.000 };
    const double kRefPb[kNumBlueStrat] = { 0.000, 0.600, 0.000, 0.000, 0.400 };
    constexpr double kRefPayoffR = 58.069;
    constexpr double kRefPayoffB = 60.717;
    constexpr double kProbTol    = 2.0e-3;
    constexpr double kPayoffTol  = 5.0e-2;

    // Gating check: the decoded mixed strategies and payoffs must match the
    // GAMS-verified equilibrium.
    CheckFn
    checkGamsEquilibrium()
    {
        return [](const VIResult& res) -> CheckResult {
            const VectorXd y = res.z.tail(kNumComp);
            double maxProbErr = 0.0;
            for (int r = 0; r < kNumRedStrat; ++r) {
                const double err = std::abs(y(prAt(r)) - kRefPr[r]);
                if (maxProbErr < err) {
                    maxProbErr = err;
                }
            }
            for (int b = 0; b < kNumBlueStrat; ++b) {
                const double err = std::abs(y(pbAt(b)) - kRefPb[b]);
                if (maxProbErr < err) {
                    maxProbErr = err;
                }
            }
            const StageTable stage = computeAllStages(y);
            double payoffR = 0.0, payoffB = 0.0;
            for (int r = 0; r < kNumRedStrat; ++r) {
                for (int b = 0; b < kNumBlueStrat; ++b) {
                    const StagePair& s = stage[r][b];
                    const double pp = y(prAt(r)) * y(pbAt(b));
                    for (int k = 0; k < kNumConLoc; ++k) {
                        payoffR += pp * kVr[r][k] * s.frNode[k] / s.den[k];
                        payoffB += pp * kVb[b][k] * s.fbNode[k] / s.den[k];
                    }
                }
            }
            const double payoffErr = std::max(std::abs(payoffR - kRefPayoffR),
                                              std::abs(payoffB - kRefPayoffB));
            char buf[160];
            std::snprintf(buf, sizeof buf,
                          "vs GAMS equilibrium: max |pr/pb err| = %.2e (tol %.1e), "
                          "max |payoff err| = %.2e (tol %.1e)",
                          maxProbErr, kProbTol, payoffErr, kPayoffTol);
            const bool matchP = (maxProbErr <= kProbTol) && (payoffErr <= kPayoffTol);
            return CheckResult{ matchP, string(buf) };
        };
    }

    constexpr int kBreakdownRows = 12;   // worst rows to print

    // Report-only check: rank the natural-residual components (free rows
    // r = H_i; orthant rows r_i = min(y_i, G_i)) and print the worst
    // kBreakdownRows by name with their y/G (or H) values, so a stalled point
    // shows WHICH equations obstruct it -- a rho cap, a stationarity row near
    // an eps-pole, an infeasibility -- rather than just how badly.
    CheckFn
    printResidualBreakdown()
    {
        return [](const VIResult& res) -> CheckResult {
            const VectorXd x = res.z.head(kNumFree);
            const VectorXd y = res.z.tail(kNumComp);
            const VectorXd h = deployH(x, y);
            const VectorXd g = deployG(x, y);
            VectorXd r(kNumFree + kNumComp);
            r.head(kNumFree) = h;
            for (Index i = 0; i < kNumComp; ++i) {
                r(kNumFree + i) = std::min(y(i), g(i));
            }
            vector<Index> order(static_cast<size_t>(kNumFree + kNumComp));
            std::iota(order.begin(), order.end(), Index{ 0 });
            std::partial_sort(order.begin(), order.begin() + kBreakdownRows,
                              order.end(), [&r](Index a, Index b) {
                                  return std::abs(r(a)) > std::abs(r(b));
                              });
            std::printf("  worst natural-residual rows (of %d):\n",
                        static_cast<int>(kNumFree + kNumComp));
            for (int t = 0; t < kBreakdownRows; ++t) {
                const Index full = order[static_cast<size_t>(t)];
                if (kNumFree > full) {
                    std::printf("    %-22s r = %+.3e   (free row: r = H)\n",
                                rowName(full).c_str(), r(full));
                }
                else {
                    const Index i = full - kNumFree;
                    std::printf("    %-22s r = %+.3e   y = %+.3e   G = %+.3e\n",
                                rowName(full).c_str(), r(full), y(i), g(i));
                }
            }
            return CheckResult{ true, "residual breakdown printed above" };
        };
    }

    // Engine controls (squared-norm tolerances). These gate CONVERGENCE only;
    // tighten when the GAMS reference solution is wired in.
    constexpr double kMagTol       = 1.0e-8;   // ssn + JN outer stop
    constexpr int    kSsnIterMax   = 300;
    constexpr int    kOuterIterMax = 50;
    constexpr double kInnerMagTol  = 1.0e-10;  // JN inner (affine-VI) stop
    constexpr int    kInnerIterMax = 20000;
    constexpr int    kHeartbeatFreq  = 1;      // flushed progress line per iter
    constexpr int    kJnStallIterMax = 5;      // JN no-progress cutoff: the first
                                               //   run froze at residual^2 19.62
                                               //   for ~22 outer iters (~1 s each)
    constexpr int    kIpmIterMax     = 200;    // inner IPM cap (counts dense LUs;
                                               //   matches McpEngineParams default)

    // ssn variant controls (the SEMI recipe for the line-search failure the
    // 2026-07-06 run showed: default penalized-FB ssn quit at iteration 20
    // while still descending).
    constexpr int    kNonmonotoneMemory = 4;    // SEMI production value
    constexpr double kRestartLambda     = 0.95; // penalized-FB restart lambda

    // Shared ssn-m4 parameters: the variant rows' base and the chain's phase 2.
    SemismoothNewtonParams
    ssnM4Params()
    {
        SemismoothNewtonParams p;
        p.magTol            = kMagTol;
        p.iterMax           = kSsnIterMax;
        p.iterFreq          = kHeartbeatFreq;
        p.nonmonotoneMemory = kNonmonotoneMemory;
        return p;
    }

    // The SEMI-style variant rows: nonmonotone memory 4 with the default
    // penalized FB (lambda 0.8), then the restart ladder -- lambda 0.95 and
    // plain FB -- as independent rows so the run shows which (if any) escapes.
    vector<McpEngineRow>
    makeSsnVariantRows(const VIModel& model)
    {
        const SemismoothNewtonParams base = ssnM4Params();

        SemismoothNewtonParams lam95 = base;
        lam95.ncp = penalizedFischerBurmeisterPair(kRestartLambda);

        SemismoothNewtonParams plainFb = base;
        plainFb.ncp = fischerBurmeisterPair();

        return {
            makeSsnRow(model, "ssn-m4",       base,    kHeartbeatFreq),
            makeSsnRow(model, "ssn-m4-lam95", lam95,   kHeartbeatFreq),
            makeSsnRow(model, "ssn-m4-fb",    plainFb, kHeartbeatFreq),
        };
    }

    // ------------------------------------------------------------------------
    // Alternating chain (built 2026-07-06 after three diagnostic runs; the
    // rationale below is the seed of the final report's description).
    //
    // The two Newton-class engines fail in COMPLEMENTARY ways on this
    // nonmonotone game:
    //   - jn+ipm (Josephy-Newton outer, interior-point inner) makes huge gains
    //     from far away -- its degeneracy-insensitive central-path steps took
    //     the GAMS start from residual^2 1.4e5 to 19.6 in ONE linearization --
    //     but stalls where the linearized LCP goes indefinite (run 2).
    //   - ssn (semismooth FB Newton) descends sharply from near-feasible
    //     points (19.6 -> 0.68 in 52 iterations, run 3) but dies far from K:
    //     its iterates are not confined to K, and the run-3 residual breakdown
    //     showed its plateau point holding NEGATIVE escort allocations
    //     (BEs = -0.34 against myEps = 0.01), i.e. camped beside the
    //     ratio-combat poles, where the Armijo search dies against the +inf
    //     walls of the merit.
    //
    // Hence the alternation, from the GAMS start, up to kChainRoundsMax rounds:
    //   1. PROJECT the iterate onto K = R^4 x R_+^446 (makeMixedProjector).
    //      This clears the pole adjacency exactly where the finisher stalled.
    //      Escorts projected TO zero keep a first-order re-entry signal
    //      (dfR_dE > 0 at REs = 0 when the route carries flow), unlike flows
    //      at zero, so the projection does not create h'(0) = 0 dead zones.
    //   2. GLOBALIZE: jn+ipm from the projected point, under the JN stall
    //      cutoff -- take the big linearization gains while they last.
    //   3. FINISH: ssn-m4 from phase 2's iterate -- local semismooth descent.
    // Each round re-linearizes each engine at the other's best point; continue
    // while a round improves the best squared natural residual by at least
    // 10% (kChainImproveFactor -- run 4 showed the endgame rounds grind SMALL
    // gains, and the round cap already bounds the cost), stop on convergence,
    // no-improvement, or the round cap. The row returns the BEST point
    // visited, not the last: nonmonotone ssn can wander above its best
    // (run 2's lam95 row returned 1.7e4 after visiting 8.2e3). Best-tracking
    // is at phase-endpoint granularity; returning ssn's internal best iterate
    // is a separate proposed library change. Precedents: PATH's crash/restart
    // cycles (Ferris-Munson) and this library's chainedSolodovHe (E3a).
    //
    // A phase that THROWS is a STALLED PHASE, not a failed row (run 4: round
    // 2's re-linearized jn+ipm was still improving, 1.19 -> 1.07, when its
    // inner IPM died with "step length collapsed" -- and the escaping
    // exception discarded a 0.68 best point). The catch logs the throw, keeps
    // the best point, and hands the round's start to the other engine; if
    // both phases of a round throw, the improvement test ends the loop and
    // the row still returns its best honestly (converged = false).
    // ------------------------------------------------------------------------
    constexpr int    kChainRoundsMax     = 5;
    constexpr double kChainImproveFactor = 0.9;   // a round must improve >= 10%

    McpEngineRow
    makeAlternatingChainRow(const VIModel& model)
    {
        const SolveFn solve = [model](const VectorXd& z0) -> VIResult {
            const Projector project = makeMixedProjector(model.n);
            JosephyNewtonParams jnParams;
            jnParams.outerTol      = kMagTol;
            jnParams.outerIterMax  = kOuterIterMax;
            jnParams.outerIterFreq = kHeartbeatFreq;
            jnParams.stallIterMax  = kJnStallIterMax;
            const InnerSolver inner =
                makeMehrotraIpmSolver(model.n, kInnerMagTol, kIpmIterMax, 0);

            VIResult best;
            best.z        = z0;
            best.residual = std::numeric_limits<double>::infinity();
            int outerTotal = 0;
            int innerTotal = 0;
            VectorXd z = z0;
            for (int round = 1; round <= kChainRoundsMax; ++round) {
                const double bestBefore = best.residual;
                char label[48];

                z = project(z);

                // Globalize. On a throw the finisher starts from the
                // projected point instead of the (lost) phase-1 iterate.
                VectorXd finishStart = z;
                std::snprintf(label, sizeof label, "altchain r%d jn+ipm", round);
                try {
                    const VIResult phase1 = solveVI(model, z, inner, jnParams,
                                                    heartbeatLogger(label));
                    outerTotal += phase1.iter;
                    innerTotal += phase1.innerIters;
                    if (phase1.residual < best.residual) {
                        best = phase1;
                    }
                    if (phase1.converged) {
                        break;   // best.converged now carries the success
                    }
                    finishStart = phase1.z;
                }
                catch (const std::exception& ex) {
                    std::printf("    %s threw (stalled phase): %s\n", label, ex.what());
                    std::fflush(stdout);
                }

                // Finish.
                std::snprintf(label, sizeof label, "altchain r%d ssn-m4", round);
                try {
                    const VIResult phase2 = semismoothNewtonSolve(
                        model, finishStart, ssnM4Params(), heartbeatLogger(label));
                    outerTotal += phase2.iter;
                    if (phase2.residual < best.residual) {
                        best = phase2;
                    }
                    if (phase2.converged) {
                        break;   // best.converged now carries the success
                    }
                }
                catch (const std::exception& ex) {
                    std::printf("    %s threw (stalled phase): %s\n", label, ex.what());
                    std::fflush(stdout);
                }

                z = best.z;   // next round: re-project + re-linearize at the best
                if (best.residual > kChainImproveFactor * bestBefore) {
                    break;    // round gained less than the required 10%
                }
            }
            VIResult out = best;
            out.iter       = outerTotal;
            out.innerIters = innerTotal;
            return out;
        };
        return McpEngineRow{ "altchain ipm->ssn", solve };
    }
} // namespace

TEST(GamsDeploy, AtLeastOneEngineReachesGamsEquilibrium) {
    const VIModel  model = buildModel();
    const VectorXd z0    = initialPoint();

    McpEngineParams params;
    params.magTol       = kMagTol;
    params.ssnIterMax   = kSsnIterMax;
    params.outerIterMax = kOuterIterMax;
    params.innerMagTol  = kInnerMagTol;
    params.innerIterMax = kInnerIterMax;
    params.iterFreq     = kHeartbeatFreq;   // flushed heartbeat: at this
                               //   dimension a row can honestly grind for
                               //   minutes, and a silent killed run loses all
                               //   buffered output (2026-07-06 first attempt)
    params.jnStallIterMax = kJnStallIterMax;

    // No JnHan row: see the header note on dHan06's per-iteration refactoring.
    const vector<McpEngine> engines = {
        McpEngine::Ssn, McpEngine::JnHe, McpEngine::JnIpm,
    };
    vector<McpEngineRow> rows = makeMcpEngineRows(model, engines, params);
    // The SEMI-style ssn variants (2026-07-06 menu item #1).
    for (McpEngineRow& row : makeSsnVariantRows(model)) {
        rows.push_back(std::move(row));
    }
    // The alternating globalizer -> finisher chain (round 1 reproduces the
    // single-shot chain of run 3 exactly, since the GAMS start is already in K).
    rows.push_back(makeAlternatingChainRow(model));

    // A row passes iff it converges AND matches the GAMS equilibrium. The
    // alternating chain is the only known-good row; the standalone engines
    // are kept as (failing) comparison rows -- their inability to solve this
    // problem alone is the documented reason the chain exists.
    const int passed = countConvergedRows(
        rows, z0,
        { printGamsOutputs(), printResidualBreakdown(), checkGamsEquilibrium() });
    EXPECT_GE(passed, 1)
        << "no engine converged to the GAMS equilibrium on the deploy_v07.gms "
           "mixed NCP (known-good: the alternating chain)";
}
// Copyright Ben Paul Wise. All Rights Reserved.

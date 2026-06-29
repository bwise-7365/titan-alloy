// Copyright Ben Paul Wise. All Rights Reserved.
//
// Created by bwise on 6/24/2026.
//

#include <assert.h>
#include <random>
#include "utils.h"
#include "flowplanner.h"


// Uses 'gravity model' to set up a feasible flow plan
// which swapping should improve.
void FlowPlanner::applyGravityModel() {
    double flowT = 0.0;
    for (int i = 0; i < nSrc; i++) {
        flowT += src[i];
    }

    flow.assign(nSrc, vector<double>(nDst));
    for (int i = 0; i < nSrc; i++) {
        for (int j = 0; j < nDst; j++) {
            flow[i][j] = (src[i] * dst[j]) / flowT;
        }
    }
}




void FlowPlanner::initGM() {

    prng.seed(seed);
    std::uniform_real_distribution<double> distribution(0.0, 1.0);

    double flowT = 5000.0;
    double costMin = 100.0;
    double costMax = 500.0;

    double sTotal = 0.0;
    src = vector<double>(nSrc);
    src.resize(nSrc);
    for (int i = 0; i < nSrc; i++) {
        const double s = distribution(prng);
        src[i] = s;
        sTotal += s;
    }
    for (int i = 0; i < nSrc; i++) {
        src[i] = (flowT / sTotal)* src[i];
    }

    double dTotal = 0.0;
    double dt2 = 0.0;
    dst = vector<double>(nDst);
    dst.resize(nDst);
    for (int j = 0; j < nDst; j++) {
        const double d = distribution(prng);
        dst[j] = d;
        dTotal += d;
    }

    for (int j = 0; j < nDst; j++) {
        dst[j] = (flowT / dTotal)*dst[j];
    }


    // for convenience, we truncate to integer values.
    // for LP feasibility, we will have to ensure that sum
    // of D <= sum of S, despite round-off errors.
    auto [iSrc, iDst, iSum] = Utils::balancedSD(src, dst);
    for (int i=0; i<nSrc; i++) {
        src[i] = iSrc[i];
    }
    for (int i=0; i<nDst; i++) {
        dst[i] = iDst[i];
    }
    flowT = iSum;

    cost = vector<vector<double>>(nSrc);
    cost.resize(nSrc);
    for (int i = 0; i < nSrc; i++) {
        cost[i] = vector<double>(nDst);
        cost[i].resize(nDst);
        for (int j = 0; j < nDst; j++) {
            const double d = distribution(prng);
            cost[i][j] = trunc( costMin + d*(costMax - costMin));
        }
    }

    applyGravityModel();
}


double FlowPlanner::oneStep() {

    //checkPlan(); // this was for verification during development. Use it if you change things.

    const double fc0 = flowCost();
    double bestDecline = - minDecline * fc0;
    bool realDecline = false;

    for (int i = 0; i < nSrc; i++) {
        for (int j = 0; j < nDst; j++) {
            for (int m = 0; m < nSrc; m++) {
                for (int n = 0; n < nDst; n++) {
                    const double disc = (cost[i][j] + cost[m][n]) - (cost[i][n]+cost[m][j]);
                    const double x = std::min(flow[i][n], flow[m][j]);
                    const double decline = disc*x;
                    if (decline < bestDecline) {
                        bestDecline = decline;
                        realDecline = true;
                        flow[i][j] = flow[i][j] + x;
                        flow[i][n] = flow[i][n] - x;
                        flow[m][n] = flow[m][n] + x;
                        flow[m][j] = flow[m][j] - x;
                    }
                }
            }
        }
    }

    checkPlan(); // this was for verification during development. Use it if you change things.


    double actualDecline = 0.0;
    if (realDecline) {
        actualDecline = bestDecline;
    }
    return actualDecline;

}

void FlowPlanner::runSwap(const bool verbose) {

    const string outputNameLog = outputBaseName + ".log.txt";

    FILE* outputLog = fopen(outputNameLog.c_str(), "w");

    if (verbose) {
        fprintf(outputLog, "Starting swap solver\n");
    }

    const double fc0 = flowCost();

    if (verbose) {
        showProblem(outputLog);
        fprintf(outputLog, "Initial cost from gravity model: %.4f\n", fc0);
    }

    int iter = 0;
    constexpr int maxIter = 500;
    double decline = -1.0;
    while ((decline < 0.0)  && (iter < maxIter)) {
        decline = oneStep();
        iter++;
        if (verbose) {
            fprintf(outputLog,"%3d/%3d decline: %14.4f\n",
                iter, maxIter, decline);
        }
    }

    if (verbose) {
        fprintf(outputLog,"Initial cost: %14.4f\n", fc0);
        const double fc1 = flowCost();
        fprintf(outputLog,"Final cost:   %14.4f\n", fc1);
        const double pct = 100.0 * (fc0 - fc1)/fc0;
        fprintf(outputLog,"Percent reduction: %5.2f\n", pct);
        fprintf(outputLog,"Factor reduction:  %5.2f\n", fc0/fc1);
        showPlan(outputLog);
    }

    fclose(outputLog);
}



// Copyright Ben Paul Wise. All Rights Reserved.

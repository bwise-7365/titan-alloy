// Copyright Ben Paul Wise. All Rights Reserved.

#include <assert.h>
#include "bdflowplanner.h"

#include <numeric>
#include <random>

void BDFP::runSwap(bool verbose) {
    if (verbose) {
        fprintf(outputLog, "Starting swap solver\n");
    }
    const double fc0 = flowCost();

    if (verbose) {
        showProblem();
        fprintf(outputLog, "Initial cost from prior plan: %.4f\n", fc0);
    }

    int iter = 0;
    constexpr int maxIter = 500;

    double decline = -1.0;
    while ((decline < 0.0)  && (iter < maxIter)) {
        decline = oneSwap();
        iter++;
        if (verbose) {
            fprintf(outputLog,"%3d/%3d swap  decline: %14.4f\n",
                iter, maxIter, decline);
            fflush(outputLog);
        }
    }

    // The shifts produce one or two small improvements
/*
    decline = -1.0;
    while ((decline < 0.0)  && (iter < maxIter)) {
        decline = oneShift();
        iter++;
        if (verbose) {
            fprintf(outputLog,"%3d/%3d shift decline: %14.4f\n",
                iter, maxIter, decline);
            fflush(outputLog);
        }
    }
*/

    if (verbose) {
        fprintf(outputLog,"Initial cost: %14.4f\n", fc0);
        const double fc1 = flowCost();
        fprintf(outputLog,"Final cost:   %14.4f\n", fc1);
        const double pct = 100.0 * (fc0 - fc1)/fc0;
        fprintf(outputLog,"Percent reduction: %5.2f\n", pct);
        fprintf(outputLog,"Factor reduction:  %5.2f\n", fc0/fc1);
        showPlan();
    }

}

double BDFP::oneSwap() {

    //checkPlan(); // this was for verification during development. Use it if you change things.

    const double fc0 = flowCost();
    double bestDecline = - minDecline * fc0;
    bool realDecline = false;

    for (int i = 0; i < nNodes; i++) {
        for (int j = 0; j < nNodes; j++) {
            for (int m = 0; m < nNodes; m++) {
                for (int n = 0; n < nNodes; n++) {
                    const double x = std::min(flow[i][n], flow[m][j]);
                    if (0.0 < x) { // there is something to be swapped
                        const double disc = (cost[i][j] + cost[m][n]) - (cost[i][n]+cost[m][j]);
                        if (disc < 0.0) {
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
        }
    }

    checkPlan(); // this was for verification during development. Use it if you change things.


    double actualDecline = 0.0;
    if (realDecline) {
        actualDecline = bestDecline;
    }
    return actualDecline;
}

double BDFP::oneShift() {

    const double fc0 = flowCost();
    double bestDecline = - minDecline * fc0;
    bool realDecline = false;


    // sTotal[i] is total flow out of #i
    vector<double> sTotal = vector<double>(nNodes);
    for (int i=0; i<nNodes; i++) {
        sTotal[i] = 0.0;
        for (int j=0; j<nNodes; j++) {
            sTotal[i] += flow[i][j];
        }
    }

    //checkPlan(); // this was for verification during development. Use it if you change things.


    for (int i = 0; i < nNodes; i++) {
        for (int j = 0; j < nNodes; j++) {
            for (int m = 0; m < nNodes; m++) {
                const double disc = (cost[i][j]- cost[m][j]);
                if (disc < 0.0) {  // potential marginal savings
                    if (0.0 < flow[m][j]) { // there is something to be shifted
                        const double x = std::min(cap[i] - sTotal[i], flow[m][j]);
                        const double decline = disc*x;
                        if (decline < bestDecline) {
                            bestDecline = decline;
                            realDecline = true;

                            flow[i][j] = flow[i][j] + x;
                            sTotal[i] = sTotal[i] + x;

                            flow[m][j] = flow[m][j] - x;
                            sTotal[m] = sTotal[m] - x;
                        }
                    }
                }
            }
        }
    }

    double actualDecline = 0.0;
    if (realDecline) {
        actualDecline = bestDecline;
    }
    return actualDecline;
}


// Copyright Ben Paul Wise. All Rights Reserved.

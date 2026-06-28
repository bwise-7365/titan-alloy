// Copyright Ben Paul Wise. All Rights Reserved.
//
// Created by bwise on 6/24/2026.
//

#include <assert.h>
#include "flowplanner.h"
#include <random>


// generates random flow problem of specified size with specified seed.
FlowPlanner::FlowPlanner(int ns, int nd, uint64_t s) {
    nSrc = ns;
    nDst = nd;
    seed = s;
    initGM();
}


// Just allocates memory
FlowPlanner::FlowPlanner(int ns, int nd) {
    nSrc = ns;
    nDst = nd;
    seed = dSeed;
    src.resize(nSrc);
    dst.resize(nDst);
    flow.assign(nSrc, vector<double>(nDst));
    cost.assign(nSrc, vector<double>(nDst));
}



FlowPlanner::~FlowPlanner() {
    // nothing yet
}

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


void FlowPlanner::setProblem(const vector<double> &s,
                             const vector<double> &d,
                             const vector<vector<double>> &c) {
    assert(static_cast<int>(s.size()) == nSrc);
    assert(static_cast<int>(d.size()) == nDst);
    assert(static_cast<int>(c.size()) == nSrc);

    src = s;
    dst = d;
    cost = c;

    applyGravityModel();
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
    auto [iSrc, iDst, iSum] = balancedSD(src, dst);
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

double FlowPlanner::flowCost() {
    double fc = 0.0;

    for (int i = 0; i < nSrc; i++) {
        for (int j = 0; j < nDst; j++) {
            fc += flow[i][j] *cost[i][j] ;
        }
    }
    return fc;
}

double FlowPlanner::oneStep() {

    checkPlan();

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

    checkPlan();


    double actualDecline = 0.0;
    if (realDecline) {
        actualDecline = bestDecline;
    }
    return actualDecline;

}

void FlowPlanner::runSwap(bool verbose) {

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


void FlowPlanner::showProblem(FILE* file) {

    fprintf(file, "Flow planner problem: %d sources, %d destinations\n", nSrc, nDst);

    fprintf(file, "Sources\n");
    for (int i = 0; i < nSrc; i++) {
        fprintf(file, "Source %3d capacity %8.2f \n", 1+i, src[i]);
    }
    fprintf(file, "\n");

    fprintf(file, "Destinations\n");
    for (int i = 0; i < nDst; i++) {
        fprintf(file, "Destination %3d requirement %8.2f \n", 1+i, dst[i]);
    }
    fprintf(file, "\n");


    fprintf(file, "Per-unit costs, row = src, col = dst\n");
    showMatrix(file, cost);
    return;
}

void FlowPlanner::showMatrix(FILE* file, const vector<vector<double>> &m) {
    fprintf(file, "   ");
    for (int j = 0; j < nDst; j++) {
        fprintf(file,"         %3d", 1+j);
    }
    fprintf(file, "\n");
    for (int i = 0; i < nSrc; i++) {
        fprintf(file, "%3d ", 1+i);
        for (int j = 0; j < nDst; j++) {
            fprintf(file, "  %9.2f ", m[i][j]);
        }
        fprintf(file, "\n");
    }
    return;
}

void FlowPlanner::showPlan(FILE* file) {
    fprintf(file, "Src->Dst flows, row = src, col = dst\n");
    showMatrix(file, flow);
    double fc = flowCost();
    fprintf(file, "Flow cost: %.4f\n", fc);
}

void FlowPlanner::checkPlan() {
    double fTotal = 0.0;
    for (int i = 0; i < nSrc; i++) {
        for (int j = 0; j < nDst; j++) {
            fTotal += flow[i][j];
        }
    }

    assert (0.0 < fTotal); // must not be zero
    double tol = fTotal * 1.0E-6;

    double sSum = 0.0;
    for (int i = 0; i < nSrc; i++) {
        sSum += src[i];
    }
    //printf("%.2f  %.2f \n", sSum, fTotal);

    double dSum = 0.0;
    for (int j = 0; j < nDst; j++) {
        dSum += dst[j];
    }
    //printf("%.2f  %.2f \n", dSum, fTotal);

    //cout << flush;

    assert(fabs(sSum - fTotal) < tol);
    assert(fabs(dSum - fTotal) < tol);

    //vector<double>  s2; // src[i] is the amount available from source i
    //vector<double>  d2;

    return;
}


// Copyright Ben Paul Wise. All Rights Reserved.

// Copyright Ben Paul Wise. All Rights Reserved.
//
// Created by bwise on 6/24/2026.
//

#include <assert.h>
#include <random>
#include "utils.h"
#include "flowplanner.h"


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
    seed = Utils::dSeed;
    src.resize(nSrc);
    dst.resize(nDst);
    flow.assign(nSrc, vector<double>(nDst));
    cost.assign(nSrc, vector<double>(nDst));
}



FlowPlanner::~FlowPlanner() {
    // nothing yet
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
double FlowPlanner::flowCost() {
    double fc = 0.0;

    for (int i = 0; i < nSrc; i++) {
        for (int j = 0; j < nDst; j++) {
            fc += flow[i][j] *cost[i][j] ;
        }
    }
    return fc;
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
    Utils::showMatrix(file, nSrc, nDst, m);
    return;
}

void FlowPlanner::showPlan(FILE* file) {
    fprintf(file, "Src->Dst flows, row = src, col = dst\n");
    showMatrix(file, flow);
    double fc = flowCost();
    for (int i=0; i<nSrc; i++) {
        double stotal = 0.0;
        for (int j=0; j<nDst; j++) {
            stotal += flow[i][j];
        }
        fprintf(file, "Src %3d sent %8.2f out of %8.2f\n",
            i+1, stotal, src[i]);
    }
    for (int j=0; j<nDst; j++) {
        double dtotal = 0.0;
        for (int i=0; i<nSrc; i++) {
            dtotal += flow[i][j];
        }
        fprintf(file, "Dst %3d received %8.2f out of %6.1f\n",
            j+1, dtotal, dst[j]);
    }
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

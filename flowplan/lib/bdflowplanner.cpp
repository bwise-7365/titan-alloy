// Copyright Ben Paul Wise. All Rights Reserved.
//
// Created by bwise on 6/24/2026.
//

#include <assert.h>
#include "bdflowplanner.h"
#include <random>


// Copyright Ben Paul Wise. All Rights Reserved.


BDFP::BDFP(const int ns, const int nbd, const int nd, const uint64_t s) {
    nNodes = ns + nbd + nd;
    seed = s;

    auto pureS = [ns, nbd, nd](int i) {
        return (i < ns);
    };

    auto pureC = [ns, nbd, nd](int i) {
        return (ns +nbd <= i);
    };

    auto mixed = [ns, nbd, nd](int i) {
        return (ns <= i && i < ns+nbd);
    };

    prng.seed(seed);
    std::uniform_real_distribution<double> distribution(0.0, 1.0);

    // For now, we say there is more available than can
    // be shipped, so the requests add to less than capacity.
    const double capT= 4000.0;
    const double rqtT = 4000.0;
    const double dTol = 1.0E-6;
    const double minDist = 0.1; // even in the same node, movement between buildings has a cost

    cap = vector<double>(nNodes);
    cap.resize(nNodes);
    rqt = vector<double>(nNodes);
    rqt.resize(nNodes);

    double cTotal = 0.0;
    double rTotal = 0.0;

    for (int i = 0; i < nNodes; i++) {
        double c = 0.0;
        double r = 0.0;
        if (i < ns+nbd) {
            c = distribution(prng);
        }
        if (ns <= i) {
            r = distribution(prng);
        }
        cap[i] = c;
        cTotal += c;
        rqt[i] = r;
        rTotal += r;
    }

    const double cRatio = capT / cTotal ;
    const double rRatio = rqtT / rTotal ;

    cTotal = 0.0;
    rTotal = 0.0;
    for (int i = 0; i < nNodes; i++) {
        cap[i] *= cRatio;
        rqt[i] *= rRatio;
        cap[i] = trunc(cap[i]);
        rqt[i] = trunc(rqt[i]);
        cTotal += cap[i];
        rTotal += rqt[i];
    }
    cap[0] = cap[0] + capT - cTotal; // fix truncate errors
    rqt[nNodes-1] = rqt[nNodes-1] + rqtT - rTotal; // same


    // We calculate cost from distance.
    // vertical is always 200 to 400.
    //
    // Initially, I put
    // pure sources on left, 100 to 200
    // mixed in the middle, 400 to 500
    // pure consumers on right, 700 to 800
    //
    // If there are few nodes and/or they
    // are largely in a left to right direction,
    // then there is little optimization to be done.
    // Hence, I mixed them up a bit.
    double sLeft = 0.0; // 100.0;
    double sRight = 800.0; // 200.0;

    double mLeft = 200.0; //400.0;
    double mRight = 10000.0; // 500.0;

    double cLeft = 400.0; // 700.0;
    double cRight = 1200.0; //  800.0;
    auto x = vector<double>(nNodes);
    x.resize(nNodes);
    auto y = vector<double>(nNodes);
    y.resize(nNodes);

    for (int i = 0; i < nNodes; i++) {
        y[i] = 200.0 + 200.0*distribution(prng);
        if (pureS(i)) {
            x[i] = sLeft+(sRight-sLeft)*distribution(prng);
        }
        if (mixed(i)) {
            x[i] = mLeft+(mRight-mLeft)*distribution(prng);
        }
        if (pureC(i)) {
            x[i] = cLeft + (cRight-cLeft)*distribution(prng);
        }
    }


    // build a cost matrix from distances.
    // 1: diagonals are not quite zero.
    // 2: c(i,j) != c(j,i)
    // 3: travel directly from far left to far right is more costly
    cost = vector<vector<double>>(nNodes);
    cost.resize(nNodes);
    for (int i = 0; i < nNodes; i++) {
        cost[i] = vector<double>(nNodes);
        cost[i].resize(nNodes);
        for (int j = 0; j < nNodes; j++) {
            const double dy = y[i] - y[j];
            const double dx = x[i] - x[j];
            double d;
            if (i == j) {
                d = minDist;
            }
            else {
                double f = distribution(prng)/20.0;
                d = (1.0 + f) * sqrt(dx*dx + dy*dy);
                bool tooFar = (pureC(i) && pureS(j)) || (pureC(j) && pureS(i));
                if (tooFar) {
                    d = 1.1 * d;
                }
            }
            d = trunc(10.0 *d)/10.0;
            cost[i][j] = d;
        }
    }

    // create the classic gravity model plan
    assert(fabs(capT-rqtT) < dTol);
    flow = vector<vector<double>>(nNodes);
    flow.resize(nNodes);
    for (int i = 0; i < nNodes; i++) {
        flow[i] = vector<double>(nNodes);
        flow[i].resize(nNodes);
        double ci = cap[i];
        for (int j = 0; j < nNodes; j++) {
            double rj = rqt[j];
            flow[i][j] = ci*rj/capT;
        }
    }

    for (int i = 0; i < nNodes; i++) {
        double si = 0.0;
        for (int j = 0; j < nNodes; j++) {
            si += flow[i][j];
        }
        double ci = cap[i];
        assert(fabs(si-ci) < dTol);
    }
    for (int i = 0; i < nNodes; i++) {
        double di = 0.0;
        for (int j = 0; j < nNodes; j++) {
            di += flow[j][i];
        }
        double ri = rqt[i];
        assert(fabs(di-ri) < dTol);
    }

    // We actually cannot eliminate to-and-fro shipments.
    // That would preserve flow-balance and reduce total shipments,
    // but then the supply & consumer nodes would receive too little and supply too little.
    // It would violate the constraint because we failed to take into account
    // shipment from self to self inside one node.
    /*
        for (int i = 0; i < nNodes; i++) {
            for (int j = 0; j < nNodes; j++) {
                if (i != j) {
                    double fij = flow[i][j];
                    double fji = flow[j][i];
                    double f = std::min(fij, fji);
                    flow[i][j] = std::max(0.0, fij - f); // avoid round-off to negative
                    flow[j][i] = std::max(0.0, fji - f);
                }
            }
        }
    */
}

BDFP::~BDFP() {
    // nothing yet
}

double BDFP::flowCost() {
    double fc = 0.0;
    for (int i = 0; i < nNodes; i++) {
        for (int j = 0; j < nNodes; j++){
            fc += cost[i][j] * flow[i][j];
        }
    }
    return fc;
}

// verify every flow is positive and every node is balanced.
void BDFP::checkPlan() {
    const double dTol = 1.0E-6;
    // the main thing is to check physical flow-balance constraints
    for (int j=0; j<nNodes; j++) {
        double lhs = cap[j];
        double rhs = rqt[j];
        for (int i=0; i<nNodes; i++) {
            assert(0.0 <= flow[i][j]);
            assert(0.0 <= flow[j][i]);
            lhs = lhs + flow[i][j];
            rhs = rhs + flow[j][i];
        }
        assert(fabs(lhs-rhs) < dTol);
    }

    for (int i=0; i<nNodes; i++) {
            double inFlow = 0.0;
        double outFlow = 0.0;
        for (int j=0; j<nNodes; j++) {
            inFlow = inFlow + flow[j][i];
            outFlow = outFlow + flow[i][j];
        }
        const double ri = rqt[i];
        const double ci = cap[i];
        assert(fabs(inFlow-ri) < dTol);
        assert(fabs(outFlow-ci) < dTol);
    }
}

void BDFP::setProblem(const vector<double> &cp, const vector<double> &rq, const vector<vector<double> > &cst) {

}

void BDFP::showProblem(FILE* file) {
    for (int i = 0; i < nNodes; i++) {
        fprintf(file, "Node %3d  capacity %6.1f  request  %6.1f\n", i+1, cap[i], rqt[i]);
    }
    fprintf(file, "Unit cost of row->col flow\n");
    showMatrix(file, cost);

}

void BDFP::showPlan(FILE* file) {
    fprintf(file, "Flow row->col\n");
    showMatrix(file, flow);
    double fc = flowCost();
    fprintf(file, "Flow cost: %.4f\n", fc);
}


void BDFP::showMatrix(FILE *file, const vector<vector<double> > &m) {
    Utils::showMatrix(file, nNodes, nNodes, m);
}

void BDFP::runSwap(bool verbose) {
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
            fflush(outputLog);
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

double BDFP::oneStep() {

    //checkPlan(); // this was for verification during development. Use it if you change things.

    const double fc0 = flowCost();
    double bestDecline = - minDecline * fc0;
    bool realDecline = false;

    for (int i = 0; i < nNodes; i++) {
        for (int j = 0; j < nNodes; j++) {
            for (int m = 0; m < nNodes; m++) {
                for (int n = 0; n < nNodes; n++) {
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
// Copyright Ben Paul Wise. All Rights Reserved.

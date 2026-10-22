// Copyright Ben Paul Wise. All Rights Reserved.

#include <assert.h>
#include "bdflowplanner.h"

#include <numeric>
#include <random>


BDFP::BDFP(int ns, int nbd, int nd, int capT, int rqtT, uint64_t s) {

    const string outputNameLog = outputBaseName + ".log.txt";

    outputLog = fopen(outputNameLog.c_str(), "w");

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



    // We calculate costs from distances.
    // vertical is always 200 to 400.
    //
    // Initially, I put
    // pure sources on the left, 100 to 200,
    // mixed in the middle, 400 to 500, and
    // pure consumers on the right, 700 to 800.
    //
    // If there are few nodes and/or they
    // are largely in a left-to-right direction,
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

    // Because sum of capacities is at least sum
    // of requirements (possibly =, possibly 10x or more)
    // we cannot create the classic gravity model plan.
    // But we can make my modification.

    assert(rqtT <= capT);
    flow = vector<vector<double>>(nNodes);
    flow.resize(nNodes);
    for (int i = 0; i < nNodes; i++) {
        flow[i] = vector<double>(nNodes);
        flow[i].resize(nNodes);
        double ci = cap[i];
        for (int j = 0; j < nNodes; j++) {
            double rj = rqt[j];
            flow[i][j] = ci*rj/capT;  // divide by the larger of the two
        }
    }

    for (int i = 0; i < nNodes; i++) {
        double si = 0.0;
        for (int j = 0; j < nNodes; j++) {
            si += flow[i][j];
        }
        double ci = (rqtT * cap[i])/capT;
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

    fclose(outputLog);
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
        assert(rhs <= lhs); // watch out for round-off
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
        assert(outFlow <= ci); // watch out for round-off
    }
}

void BDFP::setProblem(const vector<double> &cp, const vector<double> &rq, const vector<vector<double> > &cst) {

}

void BDFP::showProblem() {
    for (int i = 0; i < nNodes; i++) {
        fprintf(outputLog, "Node %3d  capacity %6.1f  request  %6.1f\n", i+1, cap[i], rqt[i]);
    }
    fprintf(outputLog, "Unit cost of row->col flow\n");
    showMatrix(cost);

}

void BDFP::showPlan() {
    fprintf(outputLog, "Flow row->col\n");
    showMatrix(flow);
    double fc = flowCost();
    fprintf(outputLog, "Flow cost: %.4f\n", fc);
}

void BDFP::showMatrix(const vector<vector<double> > &m) {
    Utils::showMatrix(outputLog, nNodes, nNodes, m);
}

// Copyright Ben Paul Wise. All Rights Reserved.

// Copyright Ben Paul Wise. All Rights Reserved.
//
// Created by bwise on 6/24/2026.
//

#include "flowplanner.h"
#include <ctime>
#include <iostream>
#include <random>

FlowPlanner::FlowPlanner(int ns, int nd) {
    nSrc = ns;
    nDst = nd;

    initGM();
}



FlowPlanner::~FlowPlanner() {
}

void FlowPlanner::initGM() {

    prng.seed(1173);
    std::uniform_real_distribution<double> distribution(0.0, 1.0);

    double flowT = 5000.0;
    double costMin = 100.0;
    double costMax = 500.0;

    double sTotal = 0.0;
    src = vector<double>(nSrc);
    src.resize(nSrc);
    for (int i = 0; i < nSrc; i++) {
        double s = distribution(prng);
        src[i] = s;
        sTotal += s;
    }
    for (int i = 0; i < nSrc; i++) {
        src[i] = (flowT / sTotal)* src[i] ;
    }

    double dTotal = 0.0;
    dst = vector<double>(nDst);
    dst.resize(nDst);
    for (int j = 0; j < nDst; j++) {
        double d = distribution(prng);
        dst[j] = d;
        dTotal += d;
    }
    for (int j = 0; j < nDst; j++) {
        dst[j] = (flowT / dTotal)*dst[j];
    }

    flow = vector<vector<double>>(nSrc,vector<double>(nDst));
    flow.resize(nDst);
    cost = vector<vector<double>>(nSrc,vector<double>(nDst));
    cost.resize(nDst);
    for (int i = 0; i < nSrc; i++) {
        flow[i].resize(nDst);
        cost[i].resize(nDst);
        for (int j = 0; j < nDst; j++) {
            flow[i][j] = (src[i]*dst[j])/flowT;
            double d = distribution(prng);
            cost[i][j] = costMin + d*(costMax - costMin);
      }
    }


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
    double fc0 = flowCost();
    double bestDecline = - minDecline * fc0;
    bool realDecline = false;

    for (int i = 0; i < nSrc; i++) {
        for (int j = 0; j < nDst; j++) {
            for (int m = 0; m < nSrc; m++) {
                for (int n = 0; n < nDst; n++) {
                    double disc = (cost[i][j] + cost[m][n])
                    - (cost[i][n]+cost[m][j]);
                    double x = std::min(flow[i][n], flow[m][j]);
                    double decline = disc*x;
                    if (decline < bestDecline) {
                        bestDecline = decline;
                        realDecline = true;
                        flow[i][j] = flow[i][j] + x;
                        flow[i][n] = flow[i][n] - x;
                        flow[m][n] = flow[m][j] + x;
                        flow[m][j] = flow[m][j] - x;
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

void FlowPlanner::run() {
    double fc0 = flowCost();
    printf("Initial cost: %12.3f\n", fc0);

    int iter = 0;
    int maxIter = 500;
    double decline = -1.0;
    while ((decline < 0.0)  && (iter < maxIter)) {
        decline = oneStep();
        iter++;
        printf("%3d/%3d decline: %12.3f\n",
            iter, maxIter, decline);
        cout << flush;
    }

    printf("Initial cost: %12.3f\n", fc0);
    double fc1 = flowCost();
    printf("Final cost:   %12.3f\n", fc1);
    double pct = 100.0 * (fc0 - fc1)/fc0;
    printf("Percent reduction: %5.2f\n", pct);
    printf("Factor reduction:  %5.2f\n", fc0/fc1);
}

string dateTimeString(time_point<system_clock> ft) {
    time_t fTime = system_clock::to_time_t(ft);
    //char* buff = newChars(30); // 25 should suffice
    //asctime_r(gmtime(&fTime), buff);  // EOL included
    char* buff = asctime(gmtime(&fTime));  // allocates own memory
    string s1 = string(buff);
    int nNdx = (int)(s1.find('\n'));
    string s2 = s1.substr(0, nNdx); // EOL removed
    s1.clear();
    return s2;
}

time_point<system_clock>  displayProgramStart(string appName, string appVersion) {
    time_point<system_clock> st;
    st = system_clock::now();
    string dts = dateTimeString(st);
    if (0 < appName.size()) {
        if (0 < appVersion.size()) {
            cout << "Software version: " << appName.c_str() << " " << " " << appVersion.c_str() << endl;
        }
        else {
            cout << "Software version: " << appName.c_str() << endl;
        }
    }
    cout << "Start time (UTC): " << dts.c_str() << endl;
    dts.clear();
    cout << flush;
    return st;
}

void displayProgramEnd(time_point<system_clock> st) {
    time_point<system_clock> ft;
    ft = system_clock::now();
    duration<double> eTime = ft - st;
    string dts = dateTimeString(ft);
    cout << "Finish time (UTC): " << dts.c_str() << endl;
    dts.clear();
    double duration = eTime.count();
    if (duration < 0.01) {
        printf("Elapsed time: %.6f seconds \n", eTime.count());
    }
    else if (duration < 10.0) {
        printf("Elapsed time: %.4f seconds \n", eTime.count());
    }
    else {
        printf("Elapsed time: %.2f seconds \n", eTime.count());
    }
    cout << flush;
    return;
}

// Copyright Ben Paul Wise. All Rights Reserved.
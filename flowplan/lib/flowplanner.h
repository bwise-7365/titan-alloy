// Copyright Ben Paul Wise. All Rights Reserved.
//
// Created by bwise on 6/24/2026.
//
#ifndef FLOWPLANNER_H
#define FLOWPLANNER_H


#include <chrono>
//#include <cmath>
#include <cstdio>
#include <iostream>
#include <limits>
#include <ostream>
#include <random>
#include <vector>

#include <assert.h>
#include <chrono>
#include <iostream>
#include <memory>  // unique_ptr
#include <random>
#include <stdio.h>
#include <sstream> // istringstream
#include <tuple>
#include <vector>

using std::cout;
using std::endl;
using std::flush;
using std::vector;
using std::string;
using std::chrono::duration;
using std::chrono::time_point;
using std::chrono::system_clock;

time_point<system_clock>  displayProgramStart(string appName = "", string appVersion = "");
void displayProgramEnd(time_point<system_clock> st);

class FlowPlanner {
    public:
    explicit FlowPlanner(int ns, int nd);
    ~FlowPlanner();
    void run();
    double flowCost();


    void initGM();

    double oneStep();

    int nSrc;
    int nDst;
    std::mt19937 prng;

    vector<double>  src; // src[i] is the amount available from source i
    vector<double>  dst; // dst[j] is the amount required by destination j
    vector<vector<double>>  flow; // flow[i][j] is the flow from source i to destination j
    vector<vector<double>>  cost; // cost[i][j] is the per-unit cost from source i to destination j

    const double minDecline = 1.0e-6; // min fractional drop (more than round-off error)

    void showProblem();
    void showPlan();

    protected:

private:

};

#endif //FLOWPLANNER_H
// Copyright Ben Paul Wise. All Rights Reserved.

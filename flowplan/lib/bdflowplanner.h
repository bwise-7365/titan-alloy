// Copyright Ben Paul Wise. All Rights Reserved.
//
//
#ifndef BD_FLOWPLANNER_H
#define BD_FLOWPLANNER_H



#include <chrono>
#include <iostream>
#include <ostream>
#include <random>
#include <vector>
#include <tuple>
#include "utils.h"

using std::cout;
using std::endl;
using std::flush;
using std::vector;
using std::pair;
using std::string;
using std::tuple;
using std::chrono::duration;
using std::chrono::time_point;
using std::chrono::system_clock;



class BDFP {
public:

    // generates random flow problem of specified size with specified seed.
    // NOTE: given the same seed, it will produce a different problem than Java 'main'
    explicit BDFP(int ns, int nbd, int nd, int capT, int rqtT, uint64_t s);
    ~BDFP();

    // given the current plan, compute total cost
    double flowCost();

    // Accept a caller-supplied problem (no truncation, no rebalancing)
    // Total cap must be at least total rqrt, possibly much more.
    void setProblem(const vector<double> &cp,
                    const vector<double> &rq,
                    const vector<vector<double>> &cst);


    int nNodes;
    uint64_t seed;
    std::mt19937 prng;
    string outputBaseName = "bd_flow_min_v00";

    FILE* outputLog;

    vector<double>  cap; // cap[i] is the amount available from node i
    vector<double>  rqt; // rqt[i] is the amount requested by node i
    vector<vector<double>>  flow; // flow[i][j] is the flow from node i to node j
    vector<vector<double>>  cost; // cost[i][j] is the per-unit cost from node i to node j

    const double minDecline = 1.0e-6; // min fractional drop (more than round-off error)

    void showProblem();
    void showPlan();
    void checkPlan();

    void showMatrix(const vector<vector<double>> &m);

    //void initMatch(int nsd, double sdRatio, uint64_t s);

    void matchClosest(bool verbose = true);

    // Iteratively improve a flow plan
    // by swapping flows to reduce cost.
    // perhaps surprisingly, swaps work on the bidirectional problem
    void runSwap(bool verbose = true);

    // If possible, swap flow between edges to reduce total cost
    // while maintaining feasibility: neither total
    // received nor total sent changes at any node.
    // If possible, returns change (negative).
    // If not, returns zero.
    double oneSwap();


    // If possible, shift flow between sources to reduce total cost
    // while maintaining feasibility:  total
    // received does not change, but totals sent will.
    // If possible, returns change (negative).
    // If not, returns zero.
    double oneShift();



protected:

private:
};

#endif // BD_FLOWPLANNER_H
// Copyright Ben Paul Wise. All Rights Reserved.

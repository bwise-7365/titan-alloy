// Copyright Ben Paul Wise. All Rights Reserved.
//
// The initial version of FlowPlanner used
// an iterative swapping scheme: slow but obviously correct.
// That was used as the baseline to verify that the GNU Linear Programming Kit
// (GLPK) was setup and used correctly.
// GLPK is not the best or fastest open-source.
// HiGHS (github.com/ERGO-Code/HiGHS)  has the MIT license and is native C++.
// HiGHS claims to have a GAMS interface at https://github.com/coin-or/GAMSlinks/
// so perhaps GAMS Studio, free version, could be used with a local HiGHS solver.
//
#ifndef FLOWPLANNER_H
#define FLOWPLANNER_H



#include <chrono>
#include <iostream>
#include <ostream>
#include <random>
#include <vector>

#include <tuple>

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

time_point<system_clock>  displayProgramStart(string appName = "", string appVersion = "");
void displayProgramEnd(time_point<system_clock> st);

uint64_t msRandom(); // microseconds since the Unix Epoch, NOT scrambled

constexpr uint64_t dSeed = 0xFE69A87450C4301C; // still my favorite

// make empty C-style strings
char* newChars(int n);

tuple<vector<int>, vector<int>, int> balancedSD(const vector<double> &src, const vector<double> &dst);


class FlowPlanner {
    public:
    explicit FlowPlanner(int ns, int nd, uint64_t s);

    // Sizes the member vectors for an nSrc x nDst
    // problem but does NOT use or generate data.
    FlowPlanner(int ns, int nd);

    ~FlowPlanner();

    // Iteratively improve a flow plan
    // by swapping flows to reduce cost.
    void runSwap(bool verbose = true);

    // given the current plan, compute total cost
    double flowCost();

    // Create a random flow problem, then applies gravity model
    // to get an initial feasible solution (but far from optimal).
    void initGM();

    // Build the initial feasible flow from the current src/dst:
    // flow[i][j] = src[i]*dst[j] / sum(src). Assumes sum(src) == sum(dst).
    void applyGravityModel();

    // Inject a caller-supplied problem (no truncation, no rebalancing) and
    // build the initial feasible flow. Sizes must match nSrc/nDst.
    void setProblem(const vector<double> &s,
                    const vector<double> &d,
                    const vector<vector<double>> &c);

    // If possible, shift flow between edges to reduce total cost
    // while maintaining feasibility.
    // If possible, returns change (negative).
    // If not, returns zero.
    double oneStep();

    int nSrc;
    int nDst;
    uint64_t seed;
    std::mt19937 prng;

    vector<double>  src; // src[i] is the amount available from source i
    vector<double>  dst; // dst[j] is the amount required by destination j
    vector<vector<double>>  flow; // flow[i][j] is the flow from source i to destination j
    vector<vector<double>>  cost; // cost[i][j] is the per-unit cost from source i to destination j

    const double minDecline = 1.0e-6; // min fractional drop (more than round-off error)

    void showProblem(FILE* file);
    void showPlan(FILE* file);
    void checkPlan();

    // Given the src, dst and cost data, setup and run GLPK
    // and copy result back into the 'flow' matrix
    void runGLPK(bool verbose = true);

    void showMatrix(FILE* file, const vector<vector<double>> &m);

    string outputBaseName = "flow_min_v00";
    protected:

private:

};

#endif //FLOWPLANNER_H
// Copyright Ben Paul Wise. All Rights Reserved.

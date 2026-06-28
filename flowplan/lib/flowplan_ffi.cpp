// Copyright Ben Paul Wise. All Rights Reserved.
//
// Implementation of the C-ABI flow-planner entry point. Bridges the flat
// double buffers of the FFI boundary to the FlowPlanner C++ class.
//
#include "flowplan_ffi.h"
#include "flowplanner.h"

#include <vector>


// Run a FlowPlanner
int32_t flowplan_solve(const double *src,  int32_t n_src,
                       const double *dst,  int32_t n_dst,
                       const double *cost,
                       double       *flow_out) {

    const bool verbose = true;
    if (n_src <= 0 || n_dst <= 0) return -1;
    if (src == nullptr || dst == nullptr ||
        cost == nullptr || flow_out == nullptr) return -2;

    std::vector<double> s(src, src + n_src);
    std::vector<double> d(dst, dst + n_dst);

    std::vector<std::vector<double>> c(n_src, std::vector<double>(n_dst));
    for (int i = 0; i < n_src; i++) {
        for (int j = 0; j < n_dst; j++) {
            c[i][j] = cost[static_cast<size_t>(i) * n_dst + j];
        }
    }

    // this just allocates memory space
    FlowPlanner fp(n_src, n_dst);

    //
    fp.setProblem(s, d, c);


    if (verbose) {
        cout << endl << endl << "Starting swap solver\n";
    }
    // starting from gravity model plan, swaps edges to reduce cost.
    fp.runSwap(verbose);
    if (verbose) {
        cout << endl << endl << "Starting GLPK solver\n";
    }
    fp.runGLPK(verbose);
    if (verbose) {
        cout << endl << endl << flush;
    }

    for (int i = 0; i < n_src; i++) {
        for (int j = 0; j < n_dst; j++) {
            flow_out[static_cast<size_t>(i) * n_dst + j] = fp.flow[i][j];
        }
    }

    return 0;
}
// Copyright Ben Paul Wise. All Rights Reserved.

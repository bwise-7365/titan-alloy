// Copyright Ben Paul Wise. All Rights Reserved.
//
// Created by bwise on 6/24/2026.
//

#include <assert.h>
#include <numeric>
#include <random>
#include "utils.h"
#include "bdflowplanner.h"


void BDFP::matchClosest(bool verbose) {
    double cTotal = std::accumulate(cap.begin(), cap.end(), 0.0);
    double rTotal = std::accumulate(rqt.begin(), rqt.end(), 0.0);
    const double myEps = 1e-6;
    assert (rTotal <= cTotal*(1+myEps)); // watch out for round-off


    // get max cost and reset all flows to zero
    double cMax = 0.0;
    flow = vector<vector<double>>(nNodes);
    for (int i=0; i<nNodes; i++) {
        flow[i] = vector<double>(nNodes);
        for (int j=0; j<nNodes; j++) {
            flow[i][j] = 0.0;
            cMax = std::max(cMax, cost[i][j]);
        }
    }
    cMax = 10.0 * cMax;

    const auto dNdx = new int[nNodes];
    for (int j = 0; j < nNodes; j++) {
        dNdx[j] = j;
    }
    std::sort(dNdx, dNdx + nNodes, [&](const int i, const int j) {
        return rqt[i] > rqt[j];
    });

    if (verbose) {
        fprintf(outputLog, "Sorted demands\n");
        for (int i=0; i<nNodes; i++) {
            int i2 = dNdx[i];
            fprintf(outputLog,"%2d  %2d  %8.2f\n",
                i, i2, rqt[i2]);
        }
        fflush(outputLog);
    }

    // these will hold modified quantities
    vector<double> c2 = cap;
    const int numPasses = 4;
    for (int pass=0; pass < numPasses; pass++) {
        vector<double> r2 = rqt;
        for (int i=0; i<nNodes; i++) {
            r2[i] = rqt[i] / numPasses;
        }
        for (int i=0; i<nNodes; i++) {
            int i2 = dNdx[i];
            // we draw from nearest sources until we
            // have enough to satisfy this pass
            while (myEps < r2[i2]) {

                double minCost = cMax;
                int minJ = -1;
                for (int j=0; j<nNodes; j++) {
                    if (0.0 < c2[j]) {
                        double c = cost[j][i2];
                        if (c < minCost) {
                            minCost = c;
                            minJ = j;
                        }
                    }
                }
                assert(0 <= minJ); // there is always enough somewhere
                const double q = std::min(r2[i2], c2[minJ]);

                c2[minJ] = c2[minJ] - q;
                r2[i2] = r2[i2] - q;
                flow[minJ][i2] += q;
            }
        }
    }
}


// Copyright Ben Paul Wise. All Rights Reserved.

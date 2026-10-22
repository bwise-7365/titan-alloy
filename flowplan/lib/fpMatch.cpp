// Copyright Ben Paul Wise. All Rights Reserved.
//
// Created by bwise on 6/24/2026.
//

#include <assert.h>
#include <numeric>
#include <random>
#include "utils.h"
#include "flowplanner.h"


void FlowPlanner::initMatch(int ns, int nd, double sdRatio, uint64_t s) {
    assert (1.0 <= sdRatio);
    double dTotal = 500.0*nd;
    double sTotal = trunc(sdRatio*dTotal); // typically 2x, 10x, or 200x

    seed = s;

    prng.seed(seed);
    std::uniform_real_distribution<double> distribution(0.0, 1.0);

    auto fixVectors = [&](double vTotal, vector<double> &v) {
        double vt = 0.0;
        for (int i = 0; i < v.size(); i++) {
            v[i] = distribution(prng);
            vt += v[i];
        }
        double vRatio = vTotal/vt;
        vt = 0.0;
        for (int i = 0; i < v.size(); i++) {
            v[i] = trunc(v[i] * vRatio);
            vt += v[i];
        }
        v[0] = v[0] + (vTotal - vt);

    };
    fixVectors(dTotal, dst);
    fixVectors(sTotal, src);

    double costSame = 0.01;
    double costMin = 100.0;
    double costMax = 500.0;
    double cNoise = 0.05;
    cost = vector<vector<double>>(nSrc);
    cost.resize(nSrc);
    for (int i = 0; i < nSrc; i++) {
        cost[i] = vector<double>(nDst);
        cost[i].resize(nDst);
        for (int j = 0; j < nDst; j++) {
            const double d = distribution(prng);
            const double c = costMin + d*(costMax - costMin);
            cost[i][j] = trunc(c*(1.0 + cNoise*distribution(prng)));
        }
    }


    flow = vector<vector<double>>(nSrc);
    flow.resize(nSrc);
    for (int i = 0; i < nSrc; i++) {
        flow[i] = vector<double>(nDst);
        flow[i].resize(nDst);
        for (int j = 0; j < nDst; j++) {
            flow[i][j] = 0.0;
        }
    }
}


void FlowPlanner::matchClosest(bool verbose) {
    double sTotal = std::accumulate(src.begin(), src.end(), 0.0);
    double dTotal = std::accumulate(dst.begin(), dst.end(), 0.0);
    double cMax = 0.0;
    for (int i=0; i<nSrc; i++) {
        for (int j=0; j<nDst; j++) {
            cMax = std::max(cMax, cost[i][j]);
        }
    }
    cMax = 10.0 * cMax;

    printf("sTotal = %9.2f\n", sTotal);
    printf("dTotal = %9.2f\n", dTotal);
    assert(dTotal <= sTotal); // the case for which this was designed


    const auto dNdx = new int[nDst];
    for (int j = 0; j < nDst; j++) {
        dNdx[j] = j;
    }
    std::sort(dNdx, dNdx + nDst, [&](int i, int j) {
        return dst[i] > dst[j];
    });

    if (verbose) {
        fprintf(stdout, "Sorted demands\n");
        for (int i=0; i<nDst; i++) {
            int i2 = dNdx[i];
            printf("%2d  %2d  %8.2f\n",
                i, i2, dst[i2]);
        }
        cout << flush;
    }


    // These will hold modified quantities
    vector<double> s2 = src;
    const int numPasses = 4;
    for (int pass=0; pass<numPasses; pass++) {
        vector<double> d2 = dst;
        for (int i=0; i<nDst; i++) {
            d2[i] = dst[i] / numPasses;
        }
        for (int i=0; i<nDst; i++) {
            int i2 = dNdx[i];
            // we draw from nearest sources until we
            // have enough to satisfy this pass
            while (0.0 < d2[i2]) {
                double minCost = cMax;
                int minJ = -1;
                for (int j=0; j<nSrc; j++) {
                    if (0.0 < s2[j]) {
                        double c = cost[j][i2];
                        if (c < minCost) {
                            minCost = c;
                            minJ = j;
                        }
                    }
                }
                assert(0 <= minJ); // there is always enough somewhere
                const double q = std::min(d2[i2], s2[minJ]);

                s2[minJ] = s2[minJ] - q;
                d2[i2] = d2[i2] - q;
                flow[minJ][i2] += q;
            }
        } // end of loop over 'i'
    }

}

// Copyright Ben Paul Wise. All Rights Reserved.

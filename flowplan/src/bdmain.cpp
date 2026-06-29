// Copyright Ben Paul Wise. All Rights Reserved.

#include <iostream>
#include "bdflowplanner.h"


int main() {
    const auto sTime = Utils::displayProgramStart("BD-FlowPlanner", "0.0.1");

    const int nSrc = 25; //5;  // number that are only suppliers
    const int nSD = 15; //3;   // number that are both suppliers and consumers
    const int nDst = 20; //4;  // number that are only consumers
    const int s = 123456;  // Utils::msRandom();  //  prng seed

    printf("Making bidirectional flow planner with %d sources, %d source and consumer, %d consumers \n",
        nSrc, nSD, nDst);

    auto bd0 = BDFP(nSrc, nSD, nDst, 4 * 1000, 4*1000, s);

    fprintf(bd0.outputLog,"\nProblem structure (1-based subscripts)\n");
    bd0.showProblem();

    bd0.matchClosest(true);

    fprintf(bd0.outputLog, "\nMatching plan (with cancellations, 1-based subscripts)\n");
    bd0.showPlan();
    fflush(bd0.outputLog);
    
    bd0.checkPlan();

    bd0.runSwap(true);
    bd0.checkPlan();

    fprintf(bd0.outputLog,"\nSwapped plan (1-based subscripts)\n");
    bd0.showPlan();

    Utils::displayProgramEnd(sTime);
    return 0;
}

// Copyright Ben Paul Wise. All Rights Reserved.
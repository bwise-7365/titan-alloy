// Copyright Ben Paul Wise. All Rights Reserved.

#include <iostream>
#include "bdflowplanner.h"


int main() {

    const auto sTime = Utils::displayProgramStart("BD-FlowPlanner", "0.0.1");

    const int nSrc = 25; //5;  // number that are only suppliers
    const int nSD = 10; //3;   // number that are both suppliers and consumers
    const int nDst = 15; //4;  // number that are only consumers
    const int s =   123456;  // msRandom(); // prng seed

    printf("Making bidirectional flow planner with %d sources, %d source and consumer, %d consumers \n",
        nSrc, nSD, nDst);
    auto bd0 = BDFP(nSrc, nSD, nDst, s);

    cout << "\nProblem structure (1-based subscripts)\n";
    bd0.showProblem(stdout);

    cout << "\nGravity plan (with cancellations, 1-based subscripts)\n";
    bd0.showPlan(stdout);
    
    bd0.checkPlan();

    bd0.runSwap(true);
    bd0.checkPlan();

    cout << "\nSwapped plan (1-based subscripts)\n";
    bd0.showPlan(stdout);

    Utils::displayProgramEnd(sTime);
    return 0;
}

// Copyright Ben Paul Wise. All Rights Reserved.
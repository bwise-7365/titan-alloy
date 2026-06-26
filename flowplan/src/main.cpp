// Copyright Ben Paul Wise. All Rights Reserved.

#include <iostream>
#include "flowplanner.h"

// TIP To <b>Run</b> code, press <shortcut actionId="Run"/> or click the <icon src="AllIcons.Actions.Execute"/> icon in the gutter.

int main() {
    auto sTime = displayProgramStart("FlowPlanner", "0.0.1");
    const int ns = 10; // 25; // 500;
    const int nd =  6; //8; // 100;
    const int s =  123456; // msRandom();
    printf("Making flow planner with %d sources and %d destinations \n",
        ns, nd);
    auto fp0 = FlowPlanner(ns, nd, s);

    if (ns * nd <= 100*50) {
        cout << endl;
        fp0.showProblem();
        cout << endl;

        cout << "Gravity model plan:"<< endl;
        fp0.showPlan();
        fp0.checkPlan();

        cout << endl;
        fp0.runSwap();
        cout << endl;

        cout << "Optimally swapped plan:"<< endl;
        fp0.showPlan();
        fp0.checkPlan();
    }


    fp0.runGLPK();


    displayProgramEnd(sTime);
    return 0;
}

// Copyright Ben Paul Wise. All Rights Reserved.
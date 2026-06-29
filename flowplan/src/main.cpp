// Copyright Ben Paul Wise. All Rights Reserved.

#include <iostream>
#include "utils.h"
#include "flowplanner.h"

// TIP To <b>Run</b> code, press <shortcut actionId="Run"/> or click the <icon src="AllIcons.Actions.Execute"/> icon in the gutter.

int main() {
    const auto sTime = Utils::displayProgramStart("FlowPlanner", "0.0.1");
    const int nd = 10; //8; // 100;
    const int ns = 16; // 25; // 500;
    const int s =  123456; // msRandom();

    // the GLPK value for 16 srcs, 10 dst, seed 123456 should be 915127.8273
    printf("Making flow planner with %d sources and %d destinations \n",
        ns, nd);
    auto fp0 = FlowPlanner(ns, nd, s);

    fp0.initMatch(10, 20, 5.0, s);
    fp0.showProblem(stdout);
    fp0.showPlan(stdout);
    cout << flush;
    fp0.matchClosest(true);
    cout << flush;
    fp0.showPlan(stdout);
    cout << flush;
    cout << flush;

    if (ns * nd <= 100*50) {
        cout << endl;
        fp0.showProblem(stdout);
        cout << endl;

        cout << "Gravity model plan:"<< endl;
        fp0.showPlan(stdout);
        fp0.checkPlan();

        cout << endl;
        fp0.runSwap();
        cout << endl;

        cout << "Optimally swapped plan:"<< endl;
        fp0.showPlan(stdout);
        fp0.checkPlan();
        cout << endl;
        cout << flush;
        cout << flush;
    }

    cout << "Running GLPK, see log file"<<endl<<flush;
    fp0.runGLPK();


    Utils::displayProgramEnd(sTime);
    cout << flush;
    cout << flush;
    return 0;
}

// Copyright Ben Paul Wise. All Rights Reserved.
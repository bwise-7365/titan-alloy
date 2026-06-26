// Copyright Ben Paul Wise. All Rights Reserved.

#include <iostream>
#include "flowplanner.h"

// TIP To <b>Run</b> code, press <shortcut actionId="Run"/> or click the <icon src="AllIcons.Actions.Execute"/> icon in the gutter.

int main() {
    auto sTime = displayProgramStart("FlowPlanner", "0.0.1");
    int ns = 80; // 200;
    int nd = 40; //50;
    int s = 123456; // msRandom()
    printf("Making flow planner with %d sources and %d destinations \n",
        ns, nd);
    FlowPlanner fp0 = FlowPlanner(ns, nd, s);

    fp0.setupLP();


    cout << endl;
    fp0.showProblem();
    cout << endl;

    cout << "Gravity model plan:"<< endl;
    fp0.showPlan();
    fp0.checkPlan();

    cout << endl;
    fp0.run();
    cout << endl;

    cout << "Optimally swapped plan:"<< endl;
    fp0.showPlan();
    fp0.checkPlan();



    displayProgramEnd(sTime);
    return 0;
    // TIP See CLion help at <a href="https://www.jetbrains.com/help/clion/">jetbrains.com/help/clion/</a>. Also, you can try interactive lessons for CLion by selecting 'Help | Learn IDE Features' from the main menu.
}

// Copyright Ben Paul Wise. All Rights Reserved.
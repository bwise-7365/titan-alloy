// Copyright Ben Paul Wise. All Rights Reserved.

#include <iostream>
#include "flowplanner.h"

// TIP To <b>Run</b> code, press <shortcut actionId="Run"/> or click the <icon src="AllIcons.Actions.Execute"/> icon in the gutter.

int main() {
    auto sTime = displayProgramStart("FlowPlanner", "0.0.1");
    int ns = 20; // 200;
    int nd = 12; //50;
    printf("Making flow planner with %d sources and %d destinations \n",
        ns, nd);
    FlowPlanner fp0 = FlowPlanner(ns, nd);
    fp0.run();

    displayProgramEnd(sTime);
    return 0;
    // TIP See CLion help at <a href="https://www.jetbrains.com/help/clion/">jetbrains.com/help/clion/</a>. Also, you can try interactive lessons for CLion by selecting 'Help | Learn IDE Features' from the main menu.
}

// Copyright Ben Paul Wise. All Rights Reserved.
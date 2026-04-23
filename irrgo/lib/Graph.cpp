// Copyright Ben Paul Wise. All Rights Reserved.
#include "Graph.h"

void Graph::addEdge(int a, int b) {
    //printf("Adding edge between %d and %d\n", a, b);
    nodes_[a].neighbors.push_back(b);
    nodes_[b].neighbors.push_back(a);
}
// Copyright Ben Paul Wise. All Rights Reserved.

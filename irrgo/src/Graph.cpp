// Copyright Ben Paul Wise. All Rights Reserved.
#include "Graph.h"

namespace IrrGo {

void Graph::addEdge(int a, int b) {
    nodes_[a].neighbors.push_back(b);
    nodes_[b].neighbors.push_back(a);
}

} // namespace IrrGo
// Copyright Ben Paul Wise. All Rights Reserved.

// Copyright Ben Paul Wise. All Rights Reserved.
#include "Graph.h"

#include <cstddef>
#include <cstdlib>
#include <stdexcept>

namespace IrrGo {

void Graph::addEdge(int a, int b) {
    nodes_[a].neighbors.push_back(b);
    nodes_[b].neighbors.push_back(a);
}

std::vector<int> Graph::neighborhoodSizes(int radius) const {
    if (radius < 0) {
        throw std::invalid_argument("neighborhoodSizes: radius must not be negative");
    }
    std::vector<int> sizes(nodes_.size(), 0);
    for (std::size_t i = 0; i < nodes_.size(); ++i) {
        int count = 0;
        for (const Node& other : nodes_) {
            if (std::abs(other.row - nodes_[i].row) +
                    std::abs(other.col - nodes_[i].col) <= radius) {
                ++count;
            }
        }
        sizes[i] = count;
    }
    return sizes;
}

} // namespace IrrGo
// Copyright Ben Paul Wise. All Rights Reserved.

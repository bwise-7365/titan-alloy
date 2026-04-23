// Copyright Ben Paul Wise. All Rights Reserved.
#pragma once
#include "Node.h"
#include <string>
#include <vector>

namespace IrrGo {

class Graph {
public:
    virtual ~Graph() = default;

    int nodeCount() const { return static_cast<int>(nodes_.size()); }
    const Node& node(int id) const { return nodes_[id]; }
    const std::vector<Node>& nodes() const { return nodes_; }

    virtual std::string asciiRepresentation() const = 0;

protected:
    std::vector<Node> nodes_;
    void addEdge(int a, int b);
};

} // namespace IrrGo
// Copyright Ben Paul Wise. All Rights Reserved.

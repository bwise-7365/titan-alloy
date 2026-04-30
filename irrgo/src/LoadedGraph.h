// Copyright Group W, SPA. All Rights Reserved.
#pragma once
#include "Graph.h"
#include <cstdint>
#include <string>
#include <vector>

namespace IrrGo {

class LoadedGraph : public Graph {
public:
    struct NodeData { std::string label; int row, col; };
    struct EdgeData { std::string labelA, labelB; };

    LoadedGraph(const std::vector<NodeData>& nodes,
                const std::vector<EdgeData>& edges,
                uint64_t seed);

    std::string asciiRepresentation() const override { return {}; }
};

} // namespace IrrGo
// Copyright Group W, SPA. All Rights Reserved.

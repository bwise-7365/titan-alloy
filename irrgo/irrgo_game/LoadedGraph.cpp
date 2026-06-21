// Copyright Ben Paul Wise. All Rights Reserved.
#include "LoadedGraph.h"
#include <unordered_map>

namespace IrrGo {

LoadedGraph::LoadedGraph(const std::vector<NodeData>& nodeData,
                         const std::vector<EdgeData>& edgeData,
                         uint64_t seed)
{
    seed_ = seed;
    nodes_.reserve(nodeData.size());
    std::unordered_map<std::string, int> labelToId;

    for (const auto& nd : nodeData) {
        int id = static_cast<int>(nodes_.size());
        nodes_.emplace_back(id, nd.row, nd.col,
                            nd.col * kColSpacing, nd.row * kRowSpacing);
        labelToId[nodes_.back().label] = id;
    }

    for (const auto& ed : edgeData) {
        auto itA = labelToId.find(ed.labelA);
        auto itB = labelToId.find(ed.labelB);
        if (itA != labelToId.end() && itB != labelToId.end())
            addEdge(itA->second, itB->second);
    }
}

} // namespace IrrGo
// Copyright Ben Paul Wise. All Rights Reserved.

// Copyright Ben Paul Wise. All Rights Reserved.
#pragma once
#include "Graph.h"
#include <cstdint>
#include <utility>
#include <vector>

namespace IrrGo {

class IrregularGraph : public Graph {
public:
    // m*n total nodes, max C incident edges per node, reproducible via seed
    IrregularGraph(int m, int n, int maxDegree, uint64_t seed);

    std::string asciiRepresentation() const override;

private:
    int m_, n_, maxDegree_;
    int baseRows_, baseCols_;
    std::vector<std::pair<int,int>> edges_;
    static int expandGrid(int n);

    static bool segmentsProperlyIntersect(float ax, float ay, float bx, float by,
                                          float cx, float cy, float dx, float dy);
    bool edgesCross(int a, int b, int c, int d) const;
    bool edgeCrossesNode(int a, int b, int nodeId) const;
    double angleDeg(int center, int n1, int n2) const;
    bool canAddEdge(int a, int b) const;
};

} // namespace IrrGo
// Copyright Ben Paul Wise. All Rights Reserved.

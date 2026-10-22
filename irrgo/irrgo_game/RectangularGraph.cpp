// Copyright Ben Paul Wise. All Rights Reserved.
#include "RectangularGraph.h"
#include <sstream>

namespace IrrGo {

RectangularGraph::RectangularGraph(int rows, int cols)
    : rows_(rows), cols_(cols)
{
    nodes_.reserve(rows * cols);
    for (int r = 0; r < rows; ++r)
        for (int c = 0; c < cols; ++c)
            nodes_.emplace_back(r * cols + c, r, c,
                                c * kColSpacing, r * kRowSpacing);

    for (int r = 0; r < rows; ++r)
        for (int c = 0; c < cols; ++c) {
            if (c + 1 < cols) addEdge(nodeId(r, c), nodeId(r, c + 1));
            if (r + 1 < rows) addEdge(nodeId(r, c), nodeId(r + 1, c));
        }
}

std::string RectangularGraph::asciiRepresentation() const {
    std::ostringstream oss;
    for (int r = 0; r < rows_; ++r) {
        for (int c = 0; c < cols_; ++c) {
            oss << '+';
            if (c + 1 < cols_) oss << "---";
        }
        oss << '\n';
        if (r + 1 < rows_) {
            for (int c = 0; c < cols_; ++c) {
                oss << '|';
                if (c + 1 < cols_) oss << "   ";
            }
            oss << '\n';
        }
    }
    return oss.str();
}

} // namespace IrrGo
// Copyright Ben Paul Wise. All Rights Reserved.

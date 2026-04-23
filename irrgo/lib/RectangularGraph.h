// Copyright Ben Paul Wise. All Rights Reserved.
#pragma once
#include "Graph.h"

class RectangularGraph : public Graph {
public:
    RectangularGraph(int rows, int cols);

    int rows() const { return rows_; }
    int cols() const { return cols_; }
    int nodeId(int row, int col) const { return row * cols_ + col; }

    std::string asciiRepresentation() const override;

private:
    int rows_, cols_;
};
// Copyright Ben Paul Wise. All Rights Reserved.

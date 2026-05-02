// Copyright Ben Paul Wise. All Rights Reserved.
#pragma once
#include <cstdint>
#include <string>
#include <vector>

namespace IrrGo {

enum class Color : uint8_t { Empty = 0, Black = 1, White = 2 };

struct Node {
    int id;
    int row, col;       // base-grid coordinates
    std::string label;  // "%02d%02d" of row, col
    float x, y;         // physical position in inches
    std::vector<int> neighbors;

    Node(int id, int row, int col, float x, float y);
};

} // namespace IrrGo
// Copyright Ben Paul Wise. All Rights Reserved.

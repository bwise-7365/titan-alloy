// Copyright Ben Paul Wise. All Rights Reserved.
#pragma once
#include "Node.h"
#include <cstdint>
#include <string>
#include <vector>

namespace IrrGo {

// Physical spacing between grid columns and rows when assigning node display
// coordinates (cells are intentionally slightly non-square for a hand-made look).
// Shared by every Graph subclass that lays out a rectangular lattice.
inline constexpr float kColSpacing = 0.87f;
inline constexpr float kRowSpacing = 0.93f;

class Graph {
public:
    virtual ~Graph() = default;

    int nodeCount() const { return static_cast<int>(nodes_.size()); }
    const Node& node(int id) const { return nodes_[id]; }
    const std::vector<Node>& nodes() const { return nodes_; }
    uint64_t seed() const { return seed_; }

    virtual std::string asciiRepresentation() const = 0;

protected:
    std::vector<Node> nodes_;
    uint64_t          seed_ = 0;
    void addEdge(int a, int b);
};

} // namespace IrrGo
// Copyright Ben Paul Wise. All Rights Reserved.

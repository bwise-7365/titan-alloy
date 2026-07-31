// Copyright Ben Paul Wise. All Rights Reserved.
#pragma once

#include <vector>

// Working storage for bensonPassAliveTerritory (EyeEval.h).
//
// It sits in its own header, depending on nothing, because Game holds one by value and
// EyeEval.h already includes Game.h -- declaring it in EyeEval.h would close that circle.
namespace IrrGo {

// Owned by the caller and refilled on every call. Holding it across calls is the whole
// point: a playout runs Benson once per ply, and returning to the allocator for a dozen
// vectors each time cost more than the algorithm itself. Once the buffers have grown to
// the board, a call allocates nothing.
//
// The variable-length per-region data is packed into one array with a start-offset array
// beside it, rather than a vector of vectors, so a board's worth of regions costs two
// allocations rather than two per region. Region r owns [start[r], start[r + 1]).
struct BensonScratch {
    std::vector<int>  chainOf;           // node -> chain id, or -1 if not the colour
    std::vector<int>  regionOf;          // node -> region id, or -1 if the colour
    std::vector<int>  frontier;          // BFS queue, consumed by read index
    std::vector<int>  regionEmpty;       // empty points of every region, packed
    std::vector<int>  regionEmptyStart;
    std::vector<int>  regionChain;       // boundary chain ids of every region, packed
    std::vector<int>  regionChainStart;
    std::vector<int>  regionVital;       // chains each region is vital TO, packed
    std::vector<int>  regionVitalStart;
    std::vector<int>  vitalCount;        // per chain, surviving regions vital to it
    std::vector<char> chainLives;
    std::vector<char> regionLives;
    std::vector<char> territory;         // the result, node-indexed

    // Working storage is not game state, so a copy starts empty instead of duplicating
    // buffers the copy would immediately overwrite. That is what keeps IrrGo::Game's copy
    // constructor -- and so clone(), once per playout -- from paying for any of this.
    BensonScratch() = default;
    BensonScratch(const BensonScratch&) {}
    BensonScratch& operator=(const BensonScratch&) { return *this; }
    BensonScratch(BensonScratch&&) = default;
    BensonScratch& operator=(BensonScratch&&) = default;
};

}  // namespace IrrGo
// Copyright Ben Paul Wise. All Rights Reserved.

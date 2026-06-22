// Copyright Ben Paul Wise. All Rights Reserved.
#pragma once

#include "AbsGame.h"
#include <cstdint>

namespace AbsGame {
    constexpr unsigned int kWordLen = 64;

    // 1-to-1 quadratic mixing map with no fixed points (RC6-derived).
    uint64_t qTrans(uint64_t s);

    // Rotate the 64-bit word left / right by n bits.
    uint64_t rotl(uint64_t x, unsigned int n);
    uint64_t rotr(uint64_t x, unsigned int n);


    static constexpr uint64_t dSeed = 0xFE69A87450C4301C; // still my favorite

} // namespace AbsGame
// Copyright Ben Paul Wise. All Rights Reserved.

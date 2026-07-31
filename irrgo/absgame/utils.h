// Copyright Ben Paul Wise. All Rights Reserved.
#pragma once

#include "AbsGame.h"
#include <cstdint>

namespace AbsGame {
    // 1-to-1 quadratic mixing map with no fixed points (RC6-derived).
    uint64_t qTrans(uint64_t s);

    static constexpr uint64_t dSeed = 0xFE69A87450C4301C; // still my favorite

} // namespace AbsGame
// Copyright Ben Paul Wise. All Rights Reserved.

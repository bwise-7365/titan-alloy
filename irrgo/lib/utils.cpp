// Copyright Ben Paul Wise. All Rights Reserved.

#include "utils.h"

#include <cstdint>

uint64_t utils::qTrans(uint64_t x)
{
    uint64_t a = 3;
    uint64_t n = 4;
    uint64_t c = 17;
    return (x+a)*(n*x+c);
}

// Copyright Ben Paul Wise. All Rights Reserved.

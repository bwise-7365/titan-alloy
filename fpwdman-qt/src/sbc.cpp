// Copyright Ben Paul Wise. All Rights Reserved.
#include "sbc.h"

namespace SBC {
    W64 rotL(const W64 val, int shift) {
        shift %= 64;
        if (shift < 0) shift += 64;
        if (shift == 0) return val;
        return (val << shift) | (val >> (64 - shift));
    }

    W64 rotR(const W64 val, int shift) {
        shift %= 64;
        if (shift < 0) shift += 64;
        if (shift == 0) return val;
        return (val >> shift) | (val << (64 - shift));
    }

    W64 qTrans(const W64 x) {
        const W64 a = 3;
        const W64 n = 4;
        const W64 c = 17;
        const W64 y = (x+a)*((n*x)+c);
        return y;
    }

    W128 qTrans(const W128 v) {
        const W64 A = v.first;
        const W64 B = v.second;
        const W64 X = qTrans(A);
        const W64 Y = qTrans(B)^X;
        return { X, Y};
    }
} // namespace SBC
// Copyright Ben Paul Wise. All Rights Reserved.

// Copyright Ben Paul Wise. All Rights Reserved.
#ifndef SBC_H
#define SBC_H

#include <cstdint>

namespace SBC {

typedef uint64_t W64;

struct W128 {
    W64 first;
    W64 second;
};

W64 rotL(const W64 val, int shift);
W64 rotR(const W64 val, int shift);
W64 qTrans(const W64 val);
W128 qTrans(const W128 val);

} // namespace SBC

#endif // SBC_H
// Copyright Ben Paul Wise. All Rights Reserved.

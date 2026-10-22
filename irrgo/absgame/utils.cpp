// Copyright Ben Paul Wise. All Rights Reserved.
#include "utils.h"

namespace AbsGame {


    uint64_t qTrans(const uint64_t s) {
        // Derived from the 1-to-1 quadratic mapping 'f' developed by Rivest et al for
        // their RC6 cipher entry in the AES contest.
        //
        // It is true but not obvious that these parameter settings ensure:
        //   (A) the mapping is 1-to-1;
        //   (B) there are no fixed points where x == qTrans(x).
        //
        // The 'f' in RC6 always kept the lowest bit unchanged. Similarly, qTrans
        // always flips the lowest bit, so it is often advisable to rotate afterwards.
        const uint64_t a = 1;  // any positive number
        const uint64_t n = 4;  // any positive even number
        const uint64_t c = 3;  // any odd number

        // Intended as modular arithmetic: the wrap-around rollovers are a feature.
        return (s + a) * ((n * s) + c);
    }

} // namespace AbsGame
// Copyright Ben Paul Wise. All Rights Reserved.

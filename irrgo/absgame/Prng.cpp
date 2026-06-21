// Copyright Ben Paul Wise. All Rights Reserved.
//
// Platform-independent PRNG seed utilities. No global state; pure functions
// except msRandom(), whose only input is the wall clock.

#include "AbsGame.h"
#include "utils.h"

#include <chrono>
#include <cstdint>

namespace AbsGame {

uint64_t msRandom() {
    using std::chrono::duration_cast;
    using std::chrono::microseconds;
    using std::chrono::system_clock;

    const microseconds ms =
        duration_cast<microseconds>(system_clock::now().time_since_epoch());
    uint64_t s2 = static_cast<uint64_t>(ms.count());  // microseconds since the Unix epoch
    s2 = rotr(s2, 3);  // roll some low-order bits up to the top
    return qTrans(s2);
}

uint64_t makeSeed(const uint64_t s) {
    uint64_t s2 = s;
    if (0 == s2) {
        s2 = msRandom();
    }
    return s2;
}

}  // namespace AbsGame
// Copyright Ben Paul Wise. All Rights Reserved.

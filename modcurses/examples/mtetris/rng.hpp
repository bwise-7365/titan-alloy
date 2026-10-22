#pragma once
//
// MTetris - a deterministic pseudo-random number generator.
//
// Determinism is the whole point: the same seed must produce the same
// sequence of pieces and starting columns, so a game can be replayed exactly.
// That rules out std::mt19937 seeded from a device, and it rules out anything
// whose output depends on the standard library implementation - libstdc++ and
// the MSVC STL must agree, so the algorithm is spelled out here.
//
#include <cstdint>
#include <random>

namespace mtetris {

// splitmix64: small, fast, and fully specified by these constants, so Windows
// and Debian produce identical streams from identical seeds.
class Prng {
public:
    explicit Prng(std::uint64_t seed) : seed_(seed), state_(seed) {}

    std::uint64_t next() {
        state_ += 0x9E3779B97F4A7C15ULL;
        std::uint64_t z = state_;
        z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ULL;
        z = (z ^ (z >> 27)) * 0x94D049BB133111EBULL;
        return z ^ (z >> 31);
    }

    // Named to match FTetris' PRNG::uniform(), which returns raw bits.
    std::uint64_t uniform() { return next(); }

    // Unbiased for the small bounds this game uses.
    int below(int n) { return n <= 0 ? 0 : static_cast<int>(next() % static_cast<std::uint64_t>(n)); }

    double uniform01() {
        // 53 significant bits, the most a double can hold exactly.
        return static_cast<double>(next() >> 11) * (1.0 / 9007199254740992.0);
    }

    [[nodiscard]] std::uint64_t seed() const { return seed_; }

    // FTetris' convention, preserved: a seed of 0 means "pick a real random
    // one", and the chosen value is reported so the game can be replayed.
    static std::uint64_t random_seed() {
        std::random_device dev;
        std::uint64_t s = 0;
        for (int i = 0; i < 4; ++i) s = (s << 16) ^ static_cast<std::uint64_t>(dev());
        return s == 0 ? 0x123456789ABCDEFULL : s;  // never hand back 0, it means "random"
    }

private:
    std::uint64_t seed_;
    std::uint64_t state_;
};

}  // namespace mtetris

#include <vector>

#include "doctest.h"
#include "modcurses/widget.hpp"

using namespace modcurses;

namespace {
std::vector<int> dist(const std::vector<SizeReq>& r, int total) {
    return distribute(std::span<const SizeReq>{r}, total);
}
}  // namespace

TEST_CASE("everyone gets their preferred size when it fits exactly") {
    const std::vector<SizeReq> r{SizeReq::fixed(3), SizeReq::fixed(4), SizeReq::fixed(3)};
    CHECK(dist(r, 10) == std::vector<int>{3, 4, 3});
}

TEST_CASE("fixed children never grow, whatever is left over") {
    const std::vector<SizeReq> r{SizeReq::fixed(3), SizeReq::fixed(4)};
    CHECK(dist(r, 100) == std::vector<int>{3, 4});
}

TEST_CASE("leftover space is split by weight") {
    SUBCASE("equal weights") {
        const std::vector<SizeReq> r{SizeReq::expand(1), SizeReq::expand(1)};
        const auto out = dist(r, 10);
        CHECK(out[0] + out[1] == 10);
        CHECK(out[0] == 5);
        CHECK(out[1] == 5);
    }
    SUBCASE("3:1 split") {
        const std::vector<SizeReq> r{SizeReq::expand(3), SizeReq::expand(1)};
        const auto out = dist(r, 12);
        CHECK(out[0] + out[1] == 12);
        CHECK(out[0] > out[1]);
        CHECK(out[0] == 9);
        CHECK(out[1] == 3);
    }
    SUBCASE("a fixed child is paid first, the rest is shared") {
        const std::vector<SizeReq> r{SizeReq::fixed(2), SizeReq::expand(1), SizeReq::expand(1)};
        const auto out = dist(r, 12);
        CHECK(out[0] == 2);
        CHECK(out[1] + out[2] == 10);
    }
}

TEST_CASE("growth stops at max and the remainder goes to whoever can still take it") {
    const std::vector<SizeReq> r{SizeReq{0, 1, 3, 1}, SizeReq::expand(1)};
    const auto out = dist(r, 20);
    CHECK(out[0] == 3);   // capped
    CHECK(out[1] == 17);  // absorbed the rest
}

TEST_CASE("nobody exceeds max even when there is space to burn") {
    const std::vector<SizeReq> r{SizeReq{0, 1, 4, 1}, SizeReq{0, 1, 4, 1}};
    const auto out = dist(r, 100);
    CHECK(out == std::vector<int>{4, 4});
}

TEST_CASE("an overdraft is taken back by weight, down to min") {
    const std::vector<SizeReq> r{SizeReq{2, 10, 100, 1}, SizeReq{2, 10, 100, 1}};
    const auto out = dist(r, 12);
    CHECK(out[0] + out[1] == 12);
    CHECK(out[0] >= 2);
    CHECK(out[1] >= 2);
}

TEST_CASE("shrinking never pushes a child below its minimum") {
    const std::vector<SizeReq> r{SizeReq{5, 10, 100, 1}, SizeReq{1, 10, 100, 1}};
    const auto out = dist(r, 8);
    CHECK(out[0] + out[1] == 8);
    CHECK(out[0] >= 5);
    CHECK(out[1] >= 1);
}

TEST_CASE("when the minimums do not fit, trailing children are hidden") {
    // Three children needing 5 each, in 12 columns: two fit, the third is
    // dropped to 0 rather than everyone being squeezed below their minimum.
    const std::vector<SizeReq> r{SizeReq::fixed(5), SizeReq::fixed(5), SizeReq::fixed(5)};
    const auto out = dist(r, 12);
    CHECK(out[0] == 5);
    CHECK(out[1] == 5);
    CHECK(out[2] == 0);  // hidden for this pass
}

TEST_CASE("zero and negative totals hide everything rather than going negative") {
    const std::vector<SizeReq> r{SizeReq::fixed(3), SizeReq::fixed(3)};
    CHECK(dist(r, 0) == std::vector<int>{0, 0});
    CHECK(dist(r, -5) == std::vector<int>{0, 0});
}

TEST_CASE("no children, no output, no crash") {
    const std::vector<SizeReq> r;
    CHECK(dist(r, 40).empty());
}

TEST_CASE("a max of INT_MAX does not overflow the arithmetic") {
    const std::vector<SizeReq> r{SizeReq::expand(1), SizeReq::expand(1), SizeReq::expand(1)};
    const auto out = dist(r, 1000000);
    CHECK(out[0] + out[1] + out[2] == 1000000);
}

TEST_CASE("distribution always sums to the total when it can") {
    // Awkward numbers: rounding must not lose or invent cells.
    for (int total = 1; total <= 40; ++total) {
        const std::vector<SizeReq> r{SizeReq::expand(1), SizeReq::expand(2), SizeReq::expand(3)};
        const auto out = dist(r, total);
        CHECK(out[0] + out[1] + out[2] == total);
    }
}

TEST_CASE("a weight of zero opts out of both growing and shrinking") {
    const std::vector<SizeReq> r{SizeReq{0, 5, 100, 0}, SizeReq::expand(1)};
    SUBCASE("plenty of room") {
        const auto out = dist(r, 20);
        CHECK(out[0] == 5);
        CHECK(out[1] == 15);
    }
    SUBCASE("tight") {
        const auto out = dist(r, 6);
        CHECK(out[0] == 5);
        CHECK(out[1] == 1);
    }
}

TEST_CASE("a malformed request (max below min) is clamped, not honoured") {
    const std::vector<SizeReq> r{SizeReq{10, 1, 2, 1}};
    const auto out = dist(r, 50);
    CHECK(out[0] >= 10);  // min wins over a nonsensical max
}

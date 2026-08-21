#include "doctest.h"
#include "modcurses/core.hpp"

using namespace modcurses;

TEST_CASE("Rect edges are exclusive on the right and bottom") {
    constexpr Rect r{{2, 3}, {4, 5}};
    static_assert(r.left() == 2 && r.top() == 3);
    static_assert(r.right() == 6 && r.bottom() == 8);
    CHECK(r.contains({2, 3}));
    CHECK(r.contains({5, 7}));
    CHECK_FALSE(r.contains({6, 7}));
    CHECK_FALSE(r.contains({5, 8}));
    CHECK_FALSE(r.contains({1, 3}));
}

TEST_CASE("empty rects contain nothing") {
    CHECK_FALSE(Rect{}.contains({0, 0}));
    CHECK(Rect{{0, 0}, {0, 5}}.empty());
    CHECK(Rect{{0, 0}, {5, 0}}.empty());
    CHECK(Rect{{0, 0}, {-3, 5}}.empty());
}

TEST_CASE("intersect") {
    constexpr Rect a{{0, 0}, {10, 10}};
    constexpr Rect b{{5, 5}, {10, 10}};
    CHECK(a.intersect(b) == Rect{{5, 5}, {5, 5}});
    CHECK(b.intersect(a) == a.intersect(b));

    SUBCASE("disjoint rects give the one canonical empty value") {
        constexpr Rect c{{20, 20}, {2, 2}};
        CHECK(a.intersect(c) == Rect{});
        CHECK(a.intersect(c).empty());
    }
    SUBCASE("touching edges do not overlap") {
        constexpr Rect d{{10, 0}, {5, 10}};
        CHECK(a.intersect(d).empty());
    }
    SUBCASE("containment") { CHECK(a.intersect(Rect{{2, 2}, {3, 3}}) == Rect{{2, 2}, {3, 3}}); }
}

TEST_CASE("geometry is signed - a curses ERR must not become a huge size") {
    // The CPPurses failure: getmaxx() returned -1, it was stored unsigned, and
    // the tree was laid out to 18446744073709551615 cells. Here it stays -1.
    const int err = -1;
    const Size s{err, err};
    CHECK(s.empty());
    CHECK(s.area() == 0);
    CHECK(s.width < 0);
}

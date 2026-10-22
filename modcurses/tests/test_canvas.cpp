#include <string>

#include "doctest.h"
#include "modcurses/render.hpp"
#include "modcurses/utf8.hpp"

using namespace modcurses;

namespace {

std::string row_of(const ScreenBuffer& buf, int y) {
    std::u32string row;
    for (int x = 0; x < buf.size().width; ++x) row.push_back(buf.back_at({x, y}).ch);
    return utf8_encode(row);
}

// A canvas over the whole buffer, as the renderer would build for a root widget.
Canvas full(ScreenBuffer& buf) {
    const Rect all{{0, 0}, buf.size()};
    return Canvas{buf, all, all};
}

}  // namespace

TEST_CASE("coordinates are local to the widget") {
    ScreenBuffer buf{Size{10, 4}};
    const Rect area{{3, 1}, {4, 2}};
    Canvas c{buf, area, Rect{{0, 0}, buf.size()}};

    CHECK(c.size() == Size{4, 2});
    c.put({0, 0}, U'A');
    c.put({3, 1}, U'B');

    CHECK(buf.back_at({3, 1}).ch == U'A');
    CHECK(buf.back_at({6, 2}).ch == U'B');
}

TEST_CASE("writes outside the widget are dropped, not wrapped or clamped") {
    ScreenBuffer buf{Size{10, 4}};
    const Rect area{{3, 1}, {4, 2}};
    Canvas c{buf, area, Rect{{0, 0}, buf.size()}};

    c.put({-1, 0}, U'L');
    c.put({4, 0}, U'R');   // one past the right edge
    c.put({0, -1}, U'U');
    c.put({0, 2}, U'D');   // one past the bottom edge

    CHECK(buf.back_at({2, 1}).ch == U' ');
    CHECK(buf.back_at({7, 1}).ch == U' ');
    CHECK(buf.back_at({3, 0}).ch == U' ');
    CHECK(buf.back_at({3, 3}).ch == U' ');
}

TEST_CASE("an ancestor clip narrows the widget further") {
    ScreenBuffer buf{Size{10, 4}};
    const Rect area{{0, 0}, {10, 4}};
    const Rect clip{{0, 0}, {5, 4}};  // a parent only 5 columns wide
    Canvas c{buf, area, clip};

    c.print({0, 0}, U"ABCDEFGH");
    CHECK(row_of(buf, 0) == "ABCDE     ");
}

TEST_CASE("print clips at the widget edge instead of overflowing") {
    ScreenBuffer buf{Size{8, 1}};
    const Rect area{{2, 0}, {3, 1}};
    Canvas c{buf, area, Rect{{0, 0}, buf.size()}};

    c.print({0, 0}, U"hello");
    CHECK(row_of(buf, 0) == "  hel   ");
}

TEST_CASE("print decodes UTF-8 and carries style") {
    ScreenBuffer buf{Size{6, 1}};
    Canvas c = full(buf);

    const Style s = fg(Color::Cyan).with(Trait::Bold);
    c.print({0, 0}, "héllo", s);

    CHECK(buf.back_at({1, 0}).ch == U'é');  // one cell, not two bytes
    CHECK(buf.back_at({1, 0}).style == s);
    CHECK(row_of(buf, 0) == "héllo ");
}

TEST_CASE("fill covers the widget and only the widget") {
    ScreenBuffer buf{Size{6, 3}};
    const Rect area{{1, 1}, {3, 1}};
    Canvas c{buf, area, Rect{{0, 0}, buf.size()}};

    c.fill(Glyph{U'#', bg(Color::Blue)});
    CHECK(row_of(buf, 0) == "      ");
    CHECK(row_of(buf, 1) == " ###  ");
    CHECK(row_of(buf, 2) == "      ");
    CHECK(buf.back_at({1, 1}).style.bg == Color::Blue);
}

TEST_CASE("draw_box draws the light box-drawing set") {
    // Also a /utf-8 canary: on MSVC without /utf-8 these literals are mangled.
    ScreenBuffer buf{Size{4, 3}};
    Canvas c = full(buf);
    c.draw_box(Rect{{0, 0}, {4, 3}});

    CHECK(row_of(buf, 0) == "┌──┐");
    CHECK(row_of(buf, 1) == "│  │");
    CHECK(row_of(buf, 2) == "└──┘");
}

TEST_CASE("box styles") {
    ScreenBuffer buf{Size{3, 3}};
    Canvas c = full(buf);

    SUBCASE("double") {
        c.draw_box(Rect{{0, 0}, {3, 3}}, {}, BoxStyle::Double);
        CHECK(row_of(buf, 0) == "╔═╗");
        CHECK(row_of(buf, 2) == "╚═╝");
    }
    SUBCASE("rounded") {
        c.draw_box(Rect{{0, 0}, {3, 3}}, {}, BoxStyle::Rounded);
        CHECK(row_of(buf, 0) == "╭─╮");
        CHECK(row_of(buf, 2) == "╰─╯");
    }
    SUBCASE("ascii stays inside ASCII") {
        c.draw_box(Rect{{0, 0}, {3, 3}}, {}, BoxStyle::Ascii);
        CHECK(row_of(buf, 0) == "+-+");
        CHECK(row_of(buf, 1) == "| |");
    }
}

TEST_CASE("degenerate boxes render as the line they actually are") {
    ScreenBuffer buf{Size{4, 3}};
    Canvas c = full(buf);

    SUBCASE("one row") {
        c.draw_box(Rect{{0, 0}, {4, 1}});
        CHECK(row_of(buf, 0) == "────");
    }
    SUBCASE("one column") {
        c.draw_box(Rect{{0, 0}, {1, 3}});
        CHECK(row_of(buf, 0) == "│   ");
        CHECK(row_of(buf, 2) == "│   ");
    }
    SUBCASE("empty draws nothing") {
        c.draw_box(Rect{{0, 0}, {0, 3}});
        CHECK(row_of(buf, 0) == "    ");
    }
}

TEST_CASE("sub() nests coordinates and inherits the clip") {
    ScreenBuffer buf{Size{8, 4}};
    const Rect area{{1, 1}, {6, 3}};
    Canvas outer{buf, area, Rect{{0, 0}, buf.size()}};

    Canvas inner = outer.sub(Rect{{1, 0}, {2, 1}});
    CHECK(inner.size() == Size{2, 1});

    inner.print({0, 0}, U"XYZ");  // third char falls outside the sub-canvas
    CHECK(row_of(buf, 1) == "  XY    ");
}

TEST_CASE("a canvas larger than the screen is clipped by the buffer") {
    ScreenBuffer buf{Size{3, 1}};
    const Rect area{{0, 0}, {100, 100}};
    Canvas c{buf, area, area};

    c.print({0, 0}, U"abcdef");
    CHECK(row_of(buf, 0) == "abc");  // no out-of-bounds write
}

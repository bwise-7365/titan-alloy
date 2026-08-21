#include <string>
#include <vector>

#include "doctest.h"
#include "modcurses/render.hpp"
#include "modcurses/terminal.hpp"

using namespace modcurses;

namespace {

// A TerminalIO that records exactly what the diff asked it to write. The
// write counts are the point: they are the proof that the full-repaint +
// diff strategy really does minimise terminal I/O, which is what makes it
// safe to delete every partial-repaint special case (lesson 6).
struct RunLog final : TerminalIO {
    struct Run {
        Point origin;
        std::u32string text;
        Style style;
    };

    std::vector<Run> runs;
    int flushes = 0;
    Size sz{0, 0};

    Size size() override { return sz; }
    std::optional<Event> poll_event(std::optional<std::chrono::milliseconds>) override {
        return std::nullopt;
    }
    void draw_run(Point origin, std::u32string_view text, Style style) override {
        runs.push_back(Run{origin, std::u32string{text}, style});
    }
    void set_cursor(std::optional<Point>) override {}
    void flush() override { ++flushes; }
    bool define_color(Color, Rgb) override { return false; }
    bool can_define_colors() override { return false; }
    void beep() override {}

    void clear() {
        runs.clear();
        flushes = 0;
    }
    [[nodiscard]] std::size_t cells_written() const {
        std::size_t n = 0;
        for (const auto& r : runs) n += r.text.size();
        return n;
    }
};

}  // namespace

TEST_CASE("the first flush after a resize writes every cell, one run per row") {
    ScreenBuffer buf{Size{4, 2}};
    RunLog log;

    const int runs = buf.flush_to(log);
    CHECK(runs == 2);
    CHECK(log.runs.size() == 2);
    CHECK(log.cells_written() == 8);
    CHECK(log.runs[0].origin == Point{0, 0});
    CHECK(log.runs[0].text == U"    ");
    CHECK(log.runs[1].origin == Point{0, 1});
    CHECK(log.flushes == 1);
}

TEST_CASE("an unchanged frame writes nothing at all") {
    ScreenBuffer buf{Size{8, 3}};
    RunLog log;
    buf.flush_to(log);
    log.clear();

    const int runs = buf.flush_to(log);
    CHECK(runs == 0);
    CHECK(log.runs.empty());
    CHECK(log.flushes == 1);  // still exactly one refresh
}

TEST_CASE("one changed cell writes one run of one cell") {
    ScreenBuffer buf{Size{8, 3}};
    RunLog log;
    buf.flush_to(log);
    log.clear();

    buf.back_at({5, 1}) = Cell{U'X', {}};
    CHECK(buf.flush_to(log) == 1);
    REQUIRE(log.runs.size() == 1);
    CHECK(log.runs[0].origin == Point{5, 1});
    CHECK(log.runs[0].text == U"X");
}

TEST_CASE("adjacent changed cells with one style coalesce into a single run") {
    ScreenBuffer buf{Size{8, 1}};
    RunLog log;
    buf.flush_to(log);
    log.clear();

    const Style s = fg(Color::Yellow);
    for (int x = 2; x < 6; ++x) buf.back_at({x, 0}) = Cell{U'=', s};

    CHECK(buf.flush_to(log) == 1);
    REQUIRE(log.runs.size() == 1);
    CHECK(log.runs[0].origin == Point{2, 0});
    CHECK(log.runs[0].text == U"====");
    CHECK(log.runs[0].style == s);
}

TEST_CASE("a run breaks at a style boundary") {
    ScreenBuffer buf{Size{8, 1}};
    RunLog log;
    buf.flush_to(log);
    log.clear();

    buf.back_at({0, 0}) = Cell{U'a', fg(Color::Red)};
    buf.back_at({1, 0}) = Cell{U'b', fg(Color::Red)};
    buf.back_at({2, 0}) = Cell{U'c', fg(Color::Blue)};

    CHECK(buf.flush_to(log) == 2);
    REQUIRE(log.runs.size() == 2);
    CHECK(log.runs[0].text == U"ab");
    CHECK(log.runs[0].style.fg == Color::Red);
    CHECK(log.runs[1].origin == Point{2, 0});
    CHECK(log.runs[1].text == U"c");
    CHECK(log.runs[1].style.fg == Color::Blue);
}

TEST_CASE("a run breaks across an unchanged cell") {
    ScreenBuffer buf{Size{8, 1}};
    RunLog log;
    buf.flush_to(log);
    log.clear();

    buf.back_at({1, 0}).ch = U'a';
    buf.back_at({3, 0}).ch = U'b';  // cell 2 is untouched

    CHECK(buf.flush_to(log) == 2);
    REQUIRE(log.runs.size() == 2);
    CHECK(log.runs[0].origin == Point{1, 0});
    CHECK(log.runs[1].origin == Point{3, 0});
}

TEST_CASE("runs never span rows") {
    ScreenBuffer buf{Size{3, 2}};
    RunLog log;
    buf.flush_to(log);
    log.clear();

    buf.clear_back(Glyph{U'#', {}});
    CHECK(buf.flush_to(log) == 2);
    for (const auto& r : log.runs) CHECK(r.text.size() == 3);
}

TEST_CASE("writing a cell back to its previous value produces no output") {
    ScreenBuffer buf{Size{4, 1}};
    RunLog log;
    buf.back_at({0, 0}).ch = U'z';
    buf.flush_to(log);
    log.clear();

    buf.back_at({0, 0}).ch = U'q';
    buf.back_at({0, 0}).ch = U'z';  // ...and back again
    CHECK(buf.flush_to(log) == 0);
}

TEST_CASE("style-only changes are still changes") {
    ScreenBuffer buf{Size{4, 1}};
    RunLog log;
    buf.flush_to(log);
    log.clear();

    buf.back_at({0, 0}).style = fg(Color::Green);
    CHECK(buf.flush_to(log) == 1);
    CHECK(log.runs[0].text == U" ");
    CHECK(log.runs[0].style.fg == Color::Green);
}

TEST_CASE("force_full_redraw repaints an otherwise clean screen") {
    ScreenBuffer buf{Size{5, 2}};
    RunLog log;
    buf.flush_to(log);
    log.clear();
    REQUIRE(buf.flush_to(log) == 0);
    log.clear();

    buf.force_full_redraw();
    CHECK(buf.flush_to(log) == 2);
    CHECK(log.cells_written() == 10);
}

TEST_CASE("resize clears both buffers and forces a full repaint") {
    ScreenBuffer buf{Size{4, 1}};
    RunLog log;
    buf.back_at({0, 0}).ch = U'x';
    buf.flush_to(log);
    log.clear();

    buf.resize(Size{6, 2});
    CHECK(buf.size() == Size{6, 2});
    CHECK(buf.back_at({0, 0}).ch == U' ');  // contents dropped, not carried over
    CHECK(buf.flush_to(log) == 2);
    CHECK(log.cells_written() == 12);
}

TEST_CASE("a degenerate size is clamped, not wrapped") {
    // getmaxx() returning ERR must never allocate SIZE_MAX cells.
    ScreenBuffer buf;
    buf.resize(Size{-1, -1});
    CHECK(buf.size() == Size{0, 0});

    RunLog log;
    CHECK(buf.flush_to(log) == 0);
    CHECK_FALSE(buf.in_bounds({0, 0}));
}

TEST_CASE("in_bounds guards every edge") {
    ScreenBuffer buf{Size{3, 2}};
    CHECK(buf.in_bounds({0, 0}));
    CHECK(buf.in_bounds({2, 1}));
    CHECK_FALSE(buf.in_bounds({3, 1}));
    CHECK_FALSE(buf.in_bounds({2, 2}));
    CHECK_FALSE(buf.in_bounds({-1, 0}));
}
